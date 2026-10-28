// Native host preview only. The production PS5 platform uses EGL/ps5-opengl.
#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#include <AudioToolbox/AudioToolbox.h>
#include "platform/platform.hpp"
#include "libretro.h"
#include <algorithm>
#include <mutex>
#include <vector>
@interface ArcadeWindow : NSWindow
@end
@implementation ArcadeWindow
- (BOOL)canBecomeKeyWindow{return YES;}
@end
namespace arcade::native {
namespace {
class MacPlatform final:public Platform {
 NSWindow* window_=nil;NSOpenGLContext* context_=nil;std::array<bool,128> keys_{};bool was_key_=true;
public:
 ~MacPlatform()override{[NSOpenGLContext clearCurrentContext];[context_ release];[window_ close];}
 bool open(std::string& error)override{
  [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  NSOpenGLPixelFormatAttribute attributes[]={NSOpenGLPFAOpenGLProfile,NSOpenGLProfileVersion4_1Core,NSOpenGLPFADoubleBuffer,NSOpenGLPFAColorSize,24,NSOpenGLPFAAlphaSize,8,0};
  auto* format=[[NSOpenGLPixelFormat alloc]initWithAttributes:attributes];if(!format){error="Cannot create native OpenGL pixel format";return false;}
  context_=[[NSOpenGLContext alloc]initWithFormat:format shareContext:nil];[format release];if(!context_){error="Cannot create native OpenGL context";return false;}
  window_=[[ArcadeWindow alloc]initWithContentRect:NSMakeRect(0,0,1338,753) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
  [window_ setReleasedWhenClosed:NO];[window_ setTitle:@"Arcade — Cover Vault"];[window_ center];[[window_ contentView]setWantsBestResolutionOpenGLSurface:YES];[context_ setView:[window_ contentView]];[context_ makeCurrentContext];
  GLint vsync=1;[context_ setValues:&vsync forParameter:NSOpenGLContextParameterSwapInterval];[window_ makeKeyAndOrderFront:nil];[NSApp finishLaunching];[NSApp activateIgnoringOtherApps:YES];return true;
 }
 Input poll()override{Input input;
  for(;;){auto* e=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];if(!e)break;
   if([e type]==NSEventTypeKeyDown||[e type]==NSEventTypeKeyUp){bool pressed=[e type]==NSEventTypeKeyDown;unsigned code=[e keyCode];if(code<keys_.size())keys_[code]=pressed;
    if(pressed){Key key;bool mapped=true;switch(code){case 36:key=Key::Cross;break;case 53:key=Key::Circle;break;case 49:key=Key::Touchpad;break;case 3:key=Key::Triangle;break;case 1:key=Key::Touchpad;break;case 12:key=Key::Square;break;case 18:key=Key::Options;break;case 23:key=Key::L3;break;case 19:key=Key::L2;break;case 20:key=Key::R2;break;case 123:key=Key::Left;break;case 124:key=Key::Right;break;case 125:key=Key::Down;break;case 126:key=Key::Up;break;case 33:key=Key::L1;break;case 30:key=Key::R1;break;default:mapped=false;break;}if(mapped)input.events.push_back({key});}
   }
   if([e type]==NSEventTypeLeftMouseDown){NSPoint point=[[window_ contentView]convertPoint:[e locationInWindow] fromView:nil];NSRect bounds=[[window_ contentView]bounds];float scale=std::min(float(bounds.size.width)/1672,float(bounds.size.height)/941);input.click=true;input.double_click=[e clickCount]>1;input.mouse_x=(point.x-(bounds.size.width-1672*scale)/2)/scale;input.mouse_y=(bounds.size.height-point.y-(bounds.size.height-941*scale)/2)/scale;}
   [NSApp sendEvent:e];
  }
  if(![window_ isVisible])input.quit=true;bool is_key=[window_ isKeyWindow];input.lost_focus=was_key_&&!is_key;was_key_=is_key;if(!is_key)keys_.fill(false);
  auto bind=[&](int code,unsigned id){if(keys_[code])input.pads[0]|=1u<<id;};
  bind(123,RETRO_DEVICE_ID_JOYPAD_LEFT);bind(124,RETRO_DEVICE_ID_JOYPAD_RIGHT);bind(125,RETRO_DEVICE_ID_JOYPAD_DOWN);bind(126,RETRO_DEVICE_ID_JOYPAD_UP);
  bind(6,RETRO_DEVICE_ID_JOYPAD_B);bind(7,RETRO_DEVICE_ID_JOYPAD_A);bind(0,RETRO_DEVICE_ID_JOYPAD_Y);bind(1,RETRO_DEVICE_ID_JOYPAD_X);bind(23,RETRO_DEVICE_ID_JOYPAD_SELECT);bind(18,RETRO_DEVICE_ID_JOYPAD_START);
  [context_ update];return input;
 }
 bool present()override{[context_ flushBuffer];return true;}
 void size(int& w,int& h)override{auto r=[[window_ contentView]convertRectToBacking:[[window_ contentView]bounds]];w=static_cast<int>(r.size.width);h=static_cast<int>(r.size.height);}
};
class MacAudio final:public AudioOutput {
 AudioQueueRef queue_=nullptr;std::vector<int16_t> ring_;std::size_t read_=0,write_=0;std::mutex mutex_;
 static void callback(void* self,AudioQueueRef queue,AudioQueueBufferRef buffer){auto* a=static_cast<MacAudio*>(self);auto* samples=static_cast<int16_t*>(buffer->mAudioData);{std::lock_guard<std::mutex> lock(a->mutex_);for(int i=0;i<2048;++i){samples[i]=a->read_==a->write_?0:a->ring_[a->read_++%a->ring_.size()];}}buffer->mAudioDataByteSize=4096;AudioQueueEnqueueBuffer(queue,buffer,0,nullptr);}
public:
 ~MacAudio()override{stop();}
 bool configure(unsigned rate)override{stop();ring_.assign(rate*2,0);read_=write_=0;AudioStreamBasicDescription format{};format.mSampleRate=rate;format.mFormatID=kAudioFormatLinearPCM;format.mFormatFlags=kLinearPCMFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;format.mBytesPerPacket=4;format.mFramesPerPacket=1;format.mBytesPerFrame=4;format.mChannelsPerFrame=2;format.mBitsPerChannel=16;
  if(AudioQueueNewOutput(&format,callback,this,nullptr,nullptr,0,&queue_)!=noErr){queue_=nullptr;return false;}for(int i=0;i<3;++i){AudioQueueBufferRef b;AudioQueueAllocateBuffer(queue_,4096,&b);callback(this,queue_,b);}return AudioQueueStart(queue_,nullptr)==noErr;
 }
 void enqueue(const int16_t* samples,std::size_t frames)override{std::lock_guard<std::mutex> lock(mutex_);if(ring_.empty())return;for(std::size_t i=0;i<frames*2;++i){if(write_-read_>=ring_.size())break;ring_[write_++%ring_.size()]=samples[i];}}
 void pause(bool p)override{if(queue_){if(p)AudioQueuePause(queue_);else AudioQueueStart(queue_,nullptr);}}
 void stop()override{if(queue_){AudioQueueStop(queue_,true);AudioQueueDispose(queue_,true);queue_=nullptr;}std::lock_guard<std::mutex> lock(mutex_);ring_.clear();read_=write_=0;}
};
}
std::unique_ptr<Platform> make_platform(){return std::make_unique<MacPlatform>();}
std::unique_ptr<AudioOutput> make_audio(){return std::make_unique<MacAudio>();}
}
