// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "gfx/font_metrics.hpp"
#include "gfx/png_image.hpp"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
#ifdef ARCADE_PS5
extern "C" void arcade_startup_note(const char*);
#endif
namespace arcade::native {
namespace {
void image_note(const std::string& message){
#ifdef ARCADE_PS5
 arcade_startup_note(message.c_str());
#else
 std::fprintf(stderr,"%s\n",message.c_str());
#endif
}
std::uint64_t pixel_hash(const unsigned char* data,size_t size){std::uint64_t hash=14695981039346656037ull;for(size_t i=0;i<size;++i){hash^=data[i];hash*=1099511628211ull;}return hash;}

constexpr const char* vertex=R"(#version 330 core
uniform vec4 u_rect;uniform vec4 u_uv;uniform vec2 u_axes;out vec2 v_local;out vec2 v_uv;
void main(){vec2 points[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(0,1),vec2(1,0),vec2(1,1),vec2(0,1));vec2 p=points[gl_VertexID];v_local=p*u_rect.zw;v_uv=u_uv.xy+p*u_uv.zw;vec2 local=v_local-u_rect.zw*.5;vec2 pos=u_rect.xy+u_rect.zw*.5+vec2(local.x*u_axes.x-local.y*u_axes.y,local.x*u_axes.y+local.y*u_axes.x);gl_Position=vec4(pos/vec2(1672,941)*vec2(2,-2)+vec2(-1,1),0,1);}
)";
constexpr const char* fragment=R"(#version 330 core
uniform float u_time;uniform int u_ambient;uniform int u_rotation;uniform int u_effect;uniform vec2 u_source;uniform vec4 u_rect;uniform vec4 u_color;uniform float u_radius;uniform int u_textured;uniform sampler2D u_image;in vec2 v_local;in vec2 v_uv;out vec4 output_color;
void main(){vec2 q=abs(v_local-u_rect.zw*.5)-u_rect.zw*.5+u_radius;float d=length(max(q,0.0))+min(max(q.x,q.y),0.0)-u_radius;float aa=max(fwidth(d),.35);float coverage=1.0-smoothstep(-aa*.5,aa*.5,d);vec4 col=u_color;if(u_textured==1){vec2 uv=v_uv;if(u_rotation==1)uv=vec2(v_uv.y,1.0-v_uv.x);else if(u_rotation==2)uv=vec2(1.0-v_uv.x,1.0-v_uv.y);else if(u_rotation==3)uv=vec2(1.0-v_uv.y,v_uv.x);col*=texture(u_image,uv);if(u_ambient==1){float warm=clamp((col.r-col.b)*2.5,0.,1.);float sweep=pow(.5+.5*sin(uv.x*12.0-u_time*.52),6.0);float pulse=.97+.03*sin(u_time*.75+uv.x*16.0);col.rgb*=pulse;col.rgb+=vec3(.045,.018,.003)*sweep*warm;}if(u_effect>0){float scan=.80+.20*cos(uv.y*u_source.y*6.2831853);col.rgb*=scan;if(u_effect==2){float mask=.94+.06*cos(uv.x*u_source.x*6.2831853);vec2 edge=(uv-.5)*2.0;float vignette=1.0-.14*dot(edge,edge);col.rgb*=mask*vignette;}}}output_color=vec4(col.rgb,col.a*coverage);}
)";
unsigned shader(unsigned kind,const char* source,std::string& error){auto s=glCreateShader(kind);glShaderSource(s,1,&source,nullptr);glCompileShader(s);int ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[2048]{};glGetShaderInfoLog(s,sizeof log,nullptr,log);error=log;glDeleteShader(s);return 0;}return s;}
constexpr Color white{1,1,1,1};
}
bool image_diagnostics_enabled(){
#ifdef ARCADE_PS5
 static const bool enabled=::access("/data/arcade-image-diagnostics",F_OK)==0;return enabled;
#else
 return false;
#endif
}
PngImage load_png_image(const std::string& path){
 PngImage result;int fd=::open(path.c_str(),O_RDONLY);
 if(fd<0){result.error="Cannot open PNG (errno "+std::to_string(errno)+")";return result;}
 std::vector<unsigned char> bytes;unsigned char buffer[32768];
 for(;;){auto n=::read(fd,buffer,sizeof buffer);if(n<0&&errno==EINTR)continue;if(n<0){result.error="Cannot read PNG";break;}if(!n)break;if(bytes.size()+size_t(n)>32u*1024u*1024u){result.error="PNG exceeds 32 MiB";break;}bytes.insert(bytes.end(),buffer,buffer+n);}
 ::close(fd);if(!result.error.empty())return result;
 int channels=0;
 if(bytes.empty()||!stbi_info_from_memory(bytes.data(),int(bytes.size()),&result.width,&result.height,&channels)||result.width<1||result.height<1||result.width>4096||result.height>4096||std::uint64_t(result.width)*result.height>16777216){result.error="Invalid PNG dimensions/header";return result;}
 auto* raw=stbi_load_from_memory(bytes.data(),int(bytes.size()),&result.width,&result.height,&channels,4);
 if(!raw){const char* why=stbi_failure_reason();result.error=why?why:"PNG decode failed";return result;}
 result.pixels.assign(raw,raw+std::size_t(result.width)*result.height*4);stbi_image_free(raw);
 if(image_diagnostics_enabled())image_note("[images] decoded "+path+" "+std::to_string(result.width)+"x"+std::to_string(result.height)+" rgba-hash="+std::to_string(pixel_hash(result.pixels.data(),result.pixels.size())));return result;
}
unsigned Renderer::image(unsigned char* pixels,int w,int h){unsigned t=0;glActiveTexture(GL_TEXTURE0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);// The PS5 driver maps large tiled transfers through scarce flexible memory.
 // Keep each transfer below its 64 KiB malloc/mmap threshold.
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
 int rows=std::max(1,49152/(w*4));unsigned upload_error=glGetError();
 for(int y=0;y<h;y+=rows){glTexSubImage2D(GL_TEXTURE_2D,0,0,y,w,std::min(rows,h-y),GL_RGBA,GL_UNSIGNED_BYTE,pixels+std::size_t(y)*w*4);auto code=glGetError();if(code&&!upload_error)upload_error=code;}
#ifdef ARCADE_PS5
 if(image_diagnostics_enabled()){std::vector<unsigned char> readback(std::size_t(w)*h*4,0xCD);unsigned read_error=0;
 for(int y=0;y<h;y+=rows){int count=std::min(rows,h-y);glGetTextureSubImage(t,0,0,y,0,w,count,1,GL_RGBA,GL_UNSIGNED_BYTE,w*count*4,readback.data()+std::size_t(y)*w*4);auto code=glGetError();if(code&&!read_error)read_error=code;}bool match=std::equal(readback.begin(),readback.end(),pixels);
 image_note("[images] texture="+std::to_string(t)+" size="+std::to_string(w)+"x"+std::to_string(h)+" upload-error="+std::to_string(upload_error)+" readback-error="+std::to_string(read_error)+" pixels-match="+(match?"yes":"NO"));}
 else if(upload_error)image_note("[images] upload GL error="+std::to_string(upload_error));
#else
 if(upload_error)image_note("[images] upload GL error="+std::to_string(upload_error));
#endif
 return t;}
bool Renderer::init(const std::string& assets,std::string& error){
 asset_directory_=assets;auto vs=shader(GL_VERTEX_SHADER,vertex,error);if(!vs)return false;auto fs=shader(GL_FRAGMENT_SHADER,fragment,error);if(!fs){glDeleteShader(vs);return false;}
 program_=glCreateProgram();glAttachShader(program_,vs);glAttachShader(program_,fs);glLinkProgram(program_);glDeleteShader(vs);glDeleteShader(fs);int ok=0;glGetProgramiv(program_,GL_LINK_STATUS,&ok);if(!ok){error="Cannot link native UI shaders";return false;}
 unsigned char pixel[4]={255,255,255,255};fallback_=image(pixel,1,1);
 glGenVertexArrays(1,&vao_);glBindVertexArray(vao_);glUseProgram(program_);
 loc_ambient_=glGetUniformLocation(program_,"u_ambient");loc_time_=glGetUniformLocation(program_,"u_time");
 loc_rotation_=glGetUniformLocation(program_,"u_rotation");loc_effect_=glGetUniformLocation(program_,"u_effect");loc_source_=glGetUniformLocation(program_,"u_source");
 loc_axes_=glGetUniformLocation(program_,"u_axes");loc_rect_=glGetUniformLocation(program_,"u_rect");loc_uv_=glGetUniformLocation(program_,"u_uv");loc_color_=glGetUniformLocation(program_,"u_color");loc_radius_=glGetUniformLocation(program_,"u_radius");loc_textured_=glGetUniformLocation(program_,"u_textured");loc_sampler_=glGetUniformLocation(program_,"u_image");
 for(int i=0;i<3;++i){auto decoded=load_png_image(assets+"/native-font-"+std::to_string(i)+".png");if(!decoded){error="Cannot load native font atlas: "+decoded.error;return false;}textures_[i]=image(decoded.pixels.data(),decoded.width,decoded.height);}
 auto atlas=load_png_image(assets+"/cover-concepts.png");if(!atlas){error="Cannot load cover atlas: "+atlas.error;return false;}int w=atlas.width;auto* data=atlas.pixels.data();cover_=image(data,atlas.width,atlas.height);
 // Reuse the exact approved sidebar silhouettes as tintable alpha textures.
 // This is runtime texture preparation, not a different hand-drawn icon set.
 const int rects[6][4]={{62,210,44,36},{66,285,38,36},{65,359,39,42},{1191,863,36,36},{1329,863,36,36},{1504,863,36,36}};
 for(int i=0;i<6;++i){int rw=rects[i][2],rh=rects[i][3];std::vector<unsigned char> mask(rw*rh*4,255);
  for(int y=0;y<rh;++y)for(int x=0;x<rw;++x){auto* px=data+((rects[i][1]+y)*w+rects[i][0]+x)*4;float alpha=0;
   if(i==0)alpha=std::clamp((float(px[0])-px[1]-2)/164.f,0.f,1.f);
   else if(i<3)alpha=std::clamp((246.f-(px[0]+px[1]+px[2])/3.f)/225.f,0.f,1.f);
   else{float dx=x-rw/2.f,dy=y-rh/2.f;bool inside=dx*dx+dy*dy<11.5f*11.5f;alpha=inside?std::clamp(((px[0]+px[1]+px[2])/3.f-20)/225.f,0.f,1.f):0;}
   if(alpha<.06f)alpha=0;mask[(y*rw+x)*4+3]=static_cast<unsigned char>(alpha*255);
  }
  auto t=image(mask.data(),rw,rh);if(i<3)icons_[i]=t;else symbols_[i-3]=t;
 }
 for(const char* name:{"vault-cabinet.png","vault-grid.png","vault-logo.png"}){if(!image_diagnostics_enabled()&&std::string(name)!="vault-cabinet.png")continue;auto decoded=load_png_image(assets+"/"+name);if(!decoded){error=std::string("Cannot load required UI asset ")+name+": "+decoded.error;return false;}auto texture=image(decoded.pixels.data(),decoded.width,decoded.height);if(!texture){error=std::string("Cannot allocate UI texture ")+name;return false;}assets_[name]=texture;image_note(std::string("[images] loaded ")+name+" texture="+std::to_string(texture));}
 std::printf("[images] cover atlas decoded from memory: %dx%d\n",atlas.width,atlas.height);return true;
}
void Renderer::close(){for(auto& item:assets_)glDeleteTextures(1,&item.second);assets_.clear();for(auto& item:covers_)glDeleteTextures(1,&item.second.texture);covers_.clear();cover_probes_.clear();for(auto t:textures_)glDeleteTextures(1,&t);for(auto t:icons_)glDeleteTextures(1,&t);for(auto t:symbols_)glDeleteTextures(1,&t);if(wallpaper_)glDeleteTextures(1,&wallpaper_);if(fallback_)glDeleteTextures(1,&fallback_);if(cover_)glDeleteTextures(1,&cover_);if(game_)glDeleteTextures(1,&game_);if(vao_)glDeleteVertexArrays(1,&vao_);if(program_)glDeleteProgram(program_);program_=0;vao_=0;fallback_=cover_=game_=wallpaper_=0;std::fill(std::begin(textures_),std::end(textures_),0);std::fill(std::begin(icons_),std::end(icons_),0);std::fill(std::begin(symbols_),std::end(symbols_),0);wallpaper_path_.clear();}
void Renderer::begin(int w,int h,Color bg){glDisable(GL_SCISSOR_TEST);glClearColor(bg.r,bg.g,bg.b,1);glClear(GL_COLOR_BUFFER_BIT);float scale=std::min(w/1672.f,h/941.f);int vw=static_cast<int>(1672*scale),vh=static_cast<int>(941*scale);glViewport((w-vw)/2,(h-vh)/2,vw,vh);glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glBindVertexArray(vao_);glUseProgram(program_);glUniform1i(loc_rotation_,0);glUniform1i(loc_effect_,0);glUniform1i(loc_ambient_,0);}
void Renderer::quad(Rect r,Color c,float radius,unsigned texture,Rect uv,float angle){glUniform2f(loc_axes_,std::cos(angle),std::sin(angle));glUniform4f(loc_rect_,r.x,r.y,r.w,r.h);glUniform4f(loc_uv_,uv.x,uv.y,uv.w,uv.h);glUniform4f(loc_color_,c.r,c.g,c.b,c.a);glUniform1f(loc_radius_,radius);glUniform1i(loc_textured_,texture?1:0);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture?texture:fallback_);glUniform1i(loc_sampler_,0);glDrawArrays(GL_TRIANGLES,0,6);}
void Renderer::rectangle(Rect r,Color c,float radius){quad(r,c,radius);}
float text_width(const std::string& text,float size,int weight,bool large_text){if(large_text&&size<=27)size*=1.16f;float width=0;for(unsigned char c:text)width+=glyph_widths[weight][(c>=32&&c<128?c:'?')-32]*size/48;return width;}
float Renderer::measure(const std::string& text,float size,int weight)const{return text_width(text,size,weight,large_text_);}
void Renderer::text(float x,float y,const std::string& text,float size,Color c,int weight,float max_width){float original_size=size;if(large_text_&&size<=27)size*=1.16f;std::string s=text;if(measure(s,original_size,weight)>max_width){while(!s.empty()&&measure(s+"...",original_size,weight)>max_width)s.pop_back();s+="...";}for(unsigned char ch:s){int i=(ch>=32&&ch<128?ch:'?')-32;quad({x,y-11*size/48,64*size/48,80*size/48},c,0,textures_[weight],{((i%16)*64+.5f)/1024.f,((i/16)*80+.5f)/480.f,63/1024.f,79/480.f});x+=glyph_widths[weight][i]*size/48;}}
void Renderer::cover_directory(const std::string& directory){if(cover_directory_==directory)return;for(auto& item:covers_)glDeleteTextures(1,&item.second.texture);covers_.clear();cover_probes_.clear();cover_directory_=directory;image_note("[images] cover directory="+directory);}
bool Renderer::cached_cover(const std::string& id,Rect dst,float radius,float opacity,bool contain){
 if(cover_directory_.empty()||id.empty()||id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")!=std::string::npos)return false;
 auto found=covers_.find(id);
 if(found==covers_.end()){
  auto now=std::chrono::steady_clock::now();auto probe=cover_probes_.find(id);if(probe!=cover_probes_.end()&&probe->second>now)return false;
  if(cover_probes_.size()>256)cover_probes_.clear();cover_probes_[id]=now+std::chrono::seconds(1);
  auto path=cover_directory_+"/"+id+".png";auto decoded=load_png_image(path);
  if(!decoded||std::uint64_t(decoded.width)*decoded.height>4194304){if(cover_failures_.insert(path).second)image_note("[images] cover unavailable "+path+": "+decoded.error);return false;}
  if(covers_.size()>=32){auto old=std::min_element(covers_.begin(),covers_.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});glDeleteTextures(1,&old->second.texture);covers_.erase(old);}
  auto texture=image(decoded.pixels.data(),decoded.width,decoded.height);found=covers_.emplace(id,CoverTexture{texture,float(decoded.width)/decoded.height,++cover_use_}).first;std::printf("[images] cached cover %s %dx%d texture=%u\n",id.c_str(),decoded.width,decoded.height,texture);
 }
 found->second.used=++cover_use_;Rect uv{0,0,1,1};float target=dst.w/dst.h,source=found->second.aspect;
 if(contain){if(source>target){float h=dst.w/source;dst.y+=(dst.h-h)/2;dst.h=h;}else{float w=dst.h*source;dst.x+=(dst.w-w)/2;dst.w=w;}}else if(source>target){uv.w=target/source;uv.x=(1-uv.w)/2;}else{uv.h=source/target;uv.y=(1-uv.h)/2;}
 quad(dst,{1,1,1,opacity},radius,found->second.texture,uv);return true;
}
void Renderer::cover(const std::string& id,Rect dst,float radius,float opacity,bool contain){if(cached_cover(id,dst,radius,opacity,contain))return;int i=-1;const char* ids[]={"robby","gridlee","circus","looping","targ","alienar"};for(int j=0;j<6;++j)if(id==ids[j])i=j;const Rect crops[]={{345,136,243,307},{624,136,225,307},{885,137,229,307},{345,519,243,314},{624,519,225,314},{885,519,229,314}};if(i<0){rectangle(dst,{.12f,.16f,.2f,opacity},radius);text(dst.x+12,dst.y+dst.h*.65f,id,20,{1,1,1,opacity},1,dst.w-24);return;}auto src=crops[i];if(contain){float aspect=src.w/src.h,target=dst.w/dst.h;if(aspect>target){float h=dst.w/aspect;dst.y+=(dst.h-h)/2;dst.h=h;}else{float w=dst.h*aspect;dst.x+=(dst.w-w)/2;dst.w=w;}}quad(dst,{1,1,1,opacity},radius,cover_,{src.x/1672,src.y/941,src.w/1672,src.h/941});}
void Renderer::asset(const std::string& name,Rect dst,Color tint){auto it=assets_.find(name);if(it==assets_.end()){auto decoded=load_png_image(asset_directory_+"/"+name);if(!decoded){std::printf("[images] %s: %s\n",name.c_str(),decoded.error.c_str());return;}auto texture=image(decoded.pixels.data(),decoded.width,decoded.height);it=assets_.emplace(name,texture).first;}if(it->second)quad(dst,tint,0,it->second);}
void Renderer::line(float x1,float y1,float x2,float y2,float width,Color c){float dx=x2-x1,dy=y2-y1,len=std::sqrt(dx*dx+dy*dy);if(len<.001f){circle(x1,y1,width/2,c);return;}quad({(x1+x2-len)/2,(y1+y2-width)/2,len,width},c,width/2,0,{0,0,1,1},std::atan2(dy,dx));}

void Renderer::sidebar_icon(int i,Rect r,Color c){quad(r,c,0,icons_[i]);}
void Renderer::circle(float x,float y,float r,Color c){rectangle({x-r,y-r,r*2,r*2},c,r);}
void Renderer::keycap(const std::string& glyph,float x,float y,float opacity){
 auto tone=[opacity](unsigned value){return Color{float((value>>16)&255)/255,float((value>>8)&255)/255,float(value&255)/255,opacity};};

 auto face=tone(0x303b49),socket=tone(0x0b1017);auto symbol=tone(0xd5dfeb);
 if(glyph=="X")symbol=tone(0x9bbdf1);else if(glyph=="[]")symbol=tone(0xe4a8d1);else if(glyph=="/\\")symbol=tone(0xa8d7bd);else if(glyph=="O")symbol=tone(0xeca29c);
 circle(x+15,y+19,16,{0,0,0,.45f*opacity});circle(x+15,y+15,16,socket);circle(x+15,y+14,14,tone(0x596575));circle(x+15,y+15,12,face);line(x+8,y+5,x+21,y+5,1,{1,1,1,.15f*opacity});
 if(glyph=="X"){line(x+10,y+10,x+20,y+20,1.8f,symbol);line(x+20,y+10,x+10,y+20,1.8f,symbol);}
 else if(glyph=="[]"){rectangle({x+9,y+9,12,12},symbol,2);rectangle({x+11,y+11,8,8},face,1);}
 else if(glyph=="/\\"){line(x+15,y+8,x+8,y+21,1.6f,symbol);line(x+8,y+21,x+22,y+21,1.6f,symbol);line(x+22,y+21,x+15,y+8,1.6f,symbol);}
 else if(glyph=="O"){circle(x+15,y+15,7,symbol);circle(x+15,y+15,5,face);}
 else if(glyph=="<>"){line(x+7,y+15,x+23,y+15,1.6f,symbol);line(x+7,y+15,x+11,y+11,1.6f,symbol);line(x+7,y+15,x+11,y+19,1.6f,symbol);line(x+23,y+15,x+19,y+11,1.6f,symbol);line(x+23,y+15,x+19,y+19,1.6f,symbol);}
 else if(glyph=="OPT"){for(int i=0;i<3;++i)line(x+9,y+10+i*5,x+21,y+10+i*5,1.6f,symbol);}
 else if(glyph=="TP"){rectangle({x+7,y+10,16,10},symbol,3);rectangle({x+9,y+12,12,6},face,2);}
 else text(x+(30-measure(glyph,12))/2,y+8,glyph,12,symbol,1);

}
void Renderer::raised(Rect r,float opacity){
 auto tone=[opacity](unsigned v){return Color{float((v>>16)&255)/255,float((v>>8)&255)/255,float(v&255)/255,opacity};};float radius=std::min(20.f,r.h*.32f);
 rectangle({r.x-1,r.y+7,r.w+2,r.h},{0,0,0,.32f*opacity},radius+1);rectangle({r.x,r.y+2,r.w,r.h},tone(0x0b1017),radius);rectangle({r.x,r.y,r.w,r.h-4},tone(0x596575),radius);rectangle({r.x+2,r.y+2,r.w-4,r.h-9},tone(0x303b49),std::max(2.f,radius-2));rectangle({r.x+13,r.y+3,r.w-26,1},{1,1,1,.15f*opacity},.5f);
}
void Renderer::button(Rect r,const std::string& label,const std::string& glyph,bool selected,float opacity,float size){
 if(selected)rectangle({r.x-2,r.y-2,r.w+4,r.h+1},{.96f,.4f,.2f,opacity},std::min(22.f,r.h*.32f+2));raised(r,opacity);
 if(!glyph.empty())keycap(glyph,r.x+12,r.y+(r.h-30)/2-2,opacity);
 float available=r.w-(glyph.empty()?24:92),left=r.x+(glyph.empty()?12:58);text(std::max(left,r.x+(r.w-measure(label,size,1))/2),r.y+(r.h-size)/2-2,label,size,{.975f,.957f,.918f,opacity},1,available);
}
void Renderer::controller_hint(int i,float x,float y,const std::string& label,Color ink,Color){const char* glyphs[]={"X","/\\","[]","O"};keycap(glyphs[std::clamp(i,0,3)],x,y);text(x+44,y+7,label,20,ink);}

void Renderer::triangle(float x,float y,float size,Color c){for(int i=0;i<static_cast<int>(size);++i){float width=size*.78f*(1-std::abs(i-size/2)/(size/2));rectangle({x,y+i,width,1.25f},c,.5f);}}
void Renderer::play(Rect r,Color,Color,const std::string& label,bool arrow){button(r,label,arrow?"X":"",false,1,29);}

void Renderer::wallpaper(const std::string& path){
 if(path!=wallpaper_path_){if(wallpaper_)glDeleteTextures(1,&wallpaper_);wallpaper_=0;wallpaper_path_=path;
  if(!path.empty()){auto decoded=load_png_image(path);if(decoded)wallpaper_=image(decoded.pixels.data(),decoded.width,decoded.height);}}

 if(wallpaper_)quad({0,0,1672,941},{1,1,1,.13f},0,wallpaper_);
}
void Renderer::game(const Frame& frame,bool smooth){Settings settings;settings.smooth=smooth;game(frame,settings);}
void Renderer::game(const Frame& frame,const Settings& settings){
 if(!frame.pixels||!frame.width||!frame.height)return;if(!game_)glGenTextures(1,&game_);glBindTexture(GL_TEXTURE_2D,game_);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,settings.smooth?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,settings.smooth?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,frame.width,frame.height,0,GL_BGRA,GL_UNSIGNED_BYTE,frame.pixels);
 float a=frame.aspect>0?frame.aspect:float(frame.width)/frame.height;if(settings.rotation%2)a=1/a;
 float w=std::min(1672.f,941*a),h=w/a;
 if(settings.scaling==1){float raww=settings.rotation%2?frame.height:frame.width,rawh=settings.rotation%2?frame.width:frame.height;float factor=std::floor(std::min(1672.f/raww,941.f/rawh));if(factor>=1){w=raww*factor;h=rawh*factor;}}
 else if(settings.scaling==2){w=1672;h=941;}
 glUniform1i(loc_rotation_,settings.rotation);glUniform1i(loc_effect_,settings.post_processing<=2?settings.post_processing:0);glUniform2f(loc_source_,frame.width,frame.height);
 quad({(1672-w)/2,(941-h)/2,w,h},white,0,game_);glUniform1i(loc_rotation_,0);glUniform1i(loc_effect_,0);
}

void Renderer::capture(const std::string& path,int w,int h){
 std::vector<unsigned char> bytes(std::size_t(w)*h*4),rgb(std::size_t(w)*h*3);unsigned error=0;int rows=std::max(1,49152/(w*4));
 for(int y=0;y<h;y+=rows){glReadPixels(0,y,w,std::min(rows,h-y),GL_RGBA,GL_UNSIGNED_BYTE,bytes.data()+std::size_t(y)*w*4);auto code=glGetError();if(code&&!error)error=code;}
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto src=bytes.data()+(std::size_t(h-1-y)*w+x)*4;auto dst=rgb.data()+(std::size_t(y)*w+x)*3;std::copy(src,src+3,dst);}
 int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0666);if(fd<0){image_note("[images] capture open failed errno="+std::to_string(errno));return;}
 auto header="P6\n"+std::to_string(w)+" "+std::to_string(h)+"\n255\n";
 auto write_all=[&](const unsigned char* data,size_t size){while(size){auto n=::write(fd,data,size);if(n<0&&errno==EINTR)continue;if(n<=0)return false;data+=n;size-=size_t(n);}return true;};
 bool ok=write_all(reinterpret_cast<const unsigned char*>(header.data()),header.size())&&write_all(rgb.data(),rgb.size());::close(fd);
 image_note("[images] capture "+path+" "+std::to_string(w)+"x"+std::to_string(h)+" saved="+(ok?"yes":"NO")+" GL-error="+std::to_string(error));
}
}

namespace arcade::native {
unsigned Renderer::load_asset(const std::string& name){
 auto found=assets_.find(name);if(found!=assets_.end())return found->second;
 auto decoded=load_png_image(asset_directory_+"/"+name);if(!decoded){image_note("[images] optional asset unavailable "+name+": "+decoded.error);assets_[name]=0;return 0;}
 auto texture=image(decoded.pixels.data(),decoded.width,decoded.height);assets_[name]=texture;image_note("[images] loaded "+name+" "+std::to_string(decoded.width)+"x"+std::to_string(decoded.height));return texture;
}
void Renderer::arcade_background(bool day,bool reduce_motion){
 auto texture=load_asset("arcade-room-4k.png");if(!texture)return;
 float time=std::chrono::duration<float>(std::chrono::steady_clock::now()-atmosphere_start_).count();
 glUniform1i(loc_ambient_,reduce_motion?0:1);glUniform1f(loc_time_,time);quad({0,0,1672,941},white,0,texture);glUniform1i(loc_ambient_,0);
 rectangle({0,0,1672,941},day?Color{.96f,.949f,.922f,.87f}:Color{.02f,.027f,.04f,.66f});
 if(!reduce_motion&&!day){for(int i=0;i<9;++i){float x=1672*std::fmod(i*.137f+time*.0012f,1.f),y=100+730*std::fmod(i*.173f+time*.003f,1.f);circle(x,y,1.2f,{.96f,.4f,.2f,.12f});}}
}
void Renderer::startup(float progress,bool reduce_motion){
 auto texture=load_asset("arcade-splash-4k.png");if(texture)quad({0,0,1672,941},white,0,texture);else{text(620,375,"ARCADE",64,white,2);text(708,465,"by zomavich",23,white);}
 float value=std::clamp(progress,0.f,1.f);if(!reduce_motion){float opacity=std::clamp(value*6.f,0.f,1.f);rectangle({0,0,1672,941},{0,0,0,1-opacity});}
 rectangle({666,874,340,7},{.04f,.05f,.065f,.9f},3);rectangle({666,874,340*value,7},{.956f,.396f,.196f,1},3);
}
void Renderer::release_startup(){auto found=assets_.find("arcade-splash-4k.png");if(found!=assets_.end()){if(found->second)glDeleteTextures(1,&found->second);assets_.erase(found);}}
}
