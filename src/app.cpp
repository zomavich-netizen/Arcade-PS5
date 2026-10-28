#include "app.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace arcade {
namespace {
constexpr float grid_x=320,grid_y=153,card_w=210,card_h=278,col_step=245,row_step=351;

}
App::App(SDL_Window*,SDL_Renderer* r,const Options& o):renderer_(r),options_(o),draw_(r,o.assets){
 model_.settings=read_settings(o.data/"settings.cfg");
 if(!o.roms.empty())model_.settings.roms=std::filesystem::absolute(o.roms);
 if(!o.core.empty())model_.settings.core=std::filesystem::absolute(o.core);
 if(o.day)model_.settings.theme=Theme::Day;if(o.night)model_.settings.theme=Theme::Night;
 model_.favorites=read_favorites(o.data/"favorites.txt");
 if(o.ui_smoke){model_.favorites.clear();model_.settings.theme=Theme::Day;}
 rescan();controllers();
}
App::~App(){core_.stop();SDL_DestroyTexture(frame_);for(auto* p:pads_)if(p)SDL_GameControllerClose(p);}
void App::save(){try{write_settings(options_.data/"settings.cfg",model_.settings);}catch(const std::exception& e){notice_=e.what();}}
void App::theme(Theme t){model_.settings.theme=t;save();}
void App::rescan(){
 try {
  if(options_.demo)model_.games=sample_library();
  else if(model_.settings.roms.empty())model_.games.clear();
  else model_.games=scan(model_.settings.roms);
  model_.clamp();
 }catch(const std::exception& e){model_.games.clear();notice_=e.what();model_.clamp();}
}
void App::favorite(){
 if(model_.page==Page::Settings)return;model_.toggle_favorite();
 try{write_favorites(options_.data/"favorites.txt",model_.favorites);}catch(const std::exception& e){notice_=e.what();}
}
void App::play(){
 auto* g=model_.current();if(!g)return;
 if(options_.demo){notice_="This is a design preview. Open the app without --demo to play your games.";return;}
 if(model_.settings.core.empty()||!std::filesystem::is_regular_file(model_.settings.core)){notice_="The arcade engine is unavailable. Check the app installation.";return;}
 loading_=true;render();SDL_RenderPresent(renderer_);
 if(!core_.load(model_.settings.core,g->path,options_.data))notice_=core_.error();
 else {paused_=false;notice_.clear();core_.buttons.fill(0);}
 loading_=false;
}
void App::stop(){core_.stop();paused_=false;SDL_DestroyTexture(frame_);frame_=nullptr;frame_w_=frame_h_=0;}
void App::activate(){
 if(model_.rail){model_.rail=false;return;}
 if(model_.page!=Page::Settings){play();return;}
 switch(model_.setting){
 case 0:theme(model_.settings.theme==Theme::Day?Theme::Night:Theme::Day);break;
 case 1:editing_=true;edit_=model_.settings.roms.string();SDL_StartTextInput();break;
 case 2:rescan();if(notice_.empty())notice_="Your library is up to date.";break;
 case 3:model_.settings.smooth=!model_.settings.smooth;save();break;
 }
}
void App::action(SDL_Keycode key){
 if(editing_){
  if(key==SDLK_ESCAPE){editing_=false;notice_.clear();SDL_StopTextInput();return;}
  if(key==SDLK_BACKSPACE){if(!edit_.empty()){do{auto ch=static_cast<unsigned char>(edit_.back());edit_.pop_back();if((ch&0xc0)!=0x80)break;}while(!edit_.empty());}return;}
  if(key==SDLK_RETURN){if(!std::filesystem::is_directory(edit_)){notice_="Choose an existing game folder.";return;}model_.settings.roms=std::filesystem::absolute(edit_);editing_=false;SDL_StopTextInput();options_.demo=false;notice_.clear();save();rescan();return;}return;
 }
 if(!notice_.empty()){if(key==SDLK_RETURN||key==SDLK_ESCAPE||key==SDLK_SPACE)notice_.clear();return;}
 if(core_.playing()){
  if(key==SDLK_ESCAPE){paused_=!paused_;core_.buttons.fill(0);return;}
  if(paused_&&key==SDLK_RETURN){paused_=false;return;}
  if(paused_&&key==SDLK_l){stop();return;}return;
 }
 switch(key){
 case SDLK_LEFT:model_.move(-1,0);break;case SDLK_RIGHT:model_.move(1,0);break;
 case SDLK_UP:model_.move(0,-1);break;case SDLK_DOWN:model_.move(0,1);break;
 case SDLK_RETURN:case SDLK_SPACE:activate();break;
 case SDLK_f:favorite();break;
 case SDLK_t:theme(model_.settings.theme==Theme::Day?Theme::Night:Theme::Day);break;
 case SDLK_TAB:model_.change_page(static_cast<Page>((static_cast<int>(model_.page)+1)%3));break;
 case SDLK_s:model_.change_page(Page::Settings);break;
 case SDLK_ESCAPE:if(model_.page!=Page::Library)model_.change_page(Page::Library);else model_.rail=!model_.rail;break;
 default:break;
 }
}
void App::controllers(){
 // Rebuild on hotplug, so disconnected devices never retain pressed buttons.
 for(auto*& p:pads_){if(p)SDL_GameControllerClose(p);p=nullptr;}
 unsigned port=0;for(int i=0;i<SDL_NumJoysticks()&&port<4;++i)if(SDL_IsGameController(i)){auto* p=SDL_GameControllerOpen(i);if(p)pads_[port++]=p;}
 core_.buttons.fill(0);
}
void App::event(const SDL_Event& e){
 if(e.type==SDL_QUIT){running_=false;return;}
 if(e.type==SDL_CONTROLLERDEVICEADDED||e.type==SDL_CONTROLLERDEVICEREMOVED){controllers();return;}
 if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_LOST&&core_.playing()){paused_=true;core_.buttons.fill(0);return;}
 if(e.type==SDL_TEXTINPUT&&editing_){edit_+=e.text.text;return;}
 if(e.type==SDL_KEYDOWN&&!e.key.repeat){action(e.key.keysym.sym);return;}
 if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT){float x,y;SDL_RenderWindowToLogical(renderer_,e.button.x,e.button.y,&x,&y);click(x,y,e.button.clicks);return;}
 if(e.type==SDL_MOUSEWHEEL&&!core_.playing())model_.move(0,e.wheel.y<0?1:-1);
 if(e.type==SDL_CONTROLLERBUTTONDOWN){
  auto b=e.cbutton.button;
  if(core_.playing()){
   if(b==SDL_CONTROLLER_BUTTON_TOUCHPAD)action(SDLK_ESCAPE);
   else if(paused_&&b==SDL_CONTROLLER_BUTTON_A)action(SDLK_RETURN);
   else if(paused_&&b==SDL_CONTROLLER_BUTTON_B)action(SDLK_l);
   return;
  }
  switch(b){
  case SDL_CONTROLLER_BUTTON_DPAD_UP:action(SDLK_UP);break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:action(SDLK_DOWN);break;
  case SDL_CONTROLLER_BUTTON_DPAD_LEFT:action(SDLK_LEFT);break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:action(SDLK_RIGHT);break;
  case SDL_CONTROLLER_BUTTON_A:action(SDLK_RETURN);break;case SDL_CONTROLLER_BUTTON_B:action(SDLK_ESCAPE);break;
  case SDL_CONTROLLER_BUTTON_Y:action(SDLK_f);break;case SDL_CONTROLLER_BUTTON_START:action(SDLK_s);break;
  case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:if(!editing_)model_.change_page(static_cast<Page>(std::max(0,static_cast<int>(model_.page)-1)));break;
  case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:if(!editing_)model_.change_page(static_cast<Page>(std::min(2,static_cast<int>(model_.page)+1)));break;
  default:break;
  }
 }
}
void App::click(float x,float y,int clicks){
 if(editing_)return;
 if(!notice_.empty()){notice_.clear();return;}
 if(core_.playing()){if(paused_){if(y>=475&&y<540)paused_=false;else if(y>=555&&y<620)stop();}return;}
 if(x<265){for(int i=0;i<3;++i)if(y>=207+i*78&&y<=267+i*78){model_.change_page(static_cast<Page>(i));return;}return;}
 if(model_.page==Page::Library&&model_.visible().empty()&&x>=359&&x<=609&&y>=505&&y<=571){model_.change_page(Page::Settings);return;}
 if(model_.page==Page::Settings){
  if(y>=181&&y<280){model_.setting=0;model_.rail=false;if(x>1000)theme(Theme::Night);else if(x>810)theme(Theme::Day);else activate();}
  else if(y>=320&&y<420){model_.setting=1;model_.rail=false;activate();}
  else if(y>=456&&y<556){model_.setting=2;model_.rail=false;activate();}
  else if(y>=592&&y<692){model_.setting=3;model_.rail=false;activate();}return;
 }
 if(x>=1122&&x<=1554&&y>=718&&y<=786){play();return;}
 if(x>=1500&&y>=556&&y<=619){favorite();return;}
 auto visible=model_.visible();size_t first=(model_.selected/6)*6;
 for(size_t i=first;i<std::min(first+6,visible.size());++i){int j=static_cast<int>(i-first);float cx=grid_x+(j%3)*col_step,cy=grid_y+(j/3)*row_step;if(x>=cx&&x<=cx+card_w&&y>=cy&&y<=cy+card_h+58){model_.selected=i;model_.rail=false;if(clicks>1)play();return;}}
}
void App::game_input(){
 core_.buttons.fill(0);if(paused_)return;
 const auto* k=SDL_GetKeyboardState(nullptr);
 auto key=[&](SDL_Scancode sc,unsigned id){if(k[sc])core_.buttons[0]|=uint16_t(1u<<id);};
 key(SDL_SCANCODE_UP,RETRO_DEVICE_ID_JOYPAD_UP);key(SDL_SCANCODE_DOWN,RETRO_DEVICE_ID_JOYPAD_DOWN);key(SDL_SCANCODE_LEFT,RETRO_DEVICE_ID_JOYPAD_LEFT);key(SDL_SCANCODE_RIGHT,RETRO_DEVICE_ID_JOYPAD_RIGHT);
 key(SDL_SCANCODE_Z,RETRO_DEVICE_ID_JOYPAD_B);key(SDL_SCANCODE_X,RETRO_DEVICE_ID_JOYPAD_A);key(SDL_SCANCODE_A,RETRO_DEVICE_ID_JOYPAD_Y);key(SDL_SCANCODE_S,RETRO_DEVICE_ID_JOYPAD_X);
 key(SDL_SCANCODE_Q,RETRO_DEVICE_ID_JOYPAD_L);key(SDL_SCANCODE_W,RETRO_DEVICE_ID_JOYPAD_R);key(SDL_SCANCODE_5,RETRO_DEVICE_ID_JOYPAD_SELECT);key(SDL_SCANCODE_1,RETRO_DEVICE_ID_JOYPAD_START);
 const std::pair<SDL_GameControllerButton,unsigned> bindings[]={
 {SDL_CONTROLLER_BUTTON_DPAD_UP,RETRO_DEVICE_ID_JOYPAD_UP},{SDL_CONTROLLER_BUTTON_DPAD_DOWN,RETRO_DEVICE_ID_JOYPAD_DOWN},{SDL_CONTROLLER_BUTTON_DPAD_LEFT,RETRO_DEVICE_ID_JOYPAD_LEFT},{SDL_CONTROLLER_BUTTON_DPAD_RIGHT,RETRO_DEVICE_ID_JOYPAD_RIGHT},
 {SDL_CONTROLLER_BUTTON_A,RETRO_DEVICE_ID_JOYPAD_B},{SDL_CONTROLLER_BUTTON_B,RETRO_DEVICE_ID_JOYPAD_A},{SDL_CONTROLLER_BUTTON_X,RETRO_DEVICE_ID_JOYPAD_Y},{SDL_CONTROLLER_BUTTON_Y,RETRO_DEVICE_ID_JOYPAD_X},
 {SDL_CONTROLLER_BUTTON_LEFTSHOULDER,RETRO_DEVICE_ID_JOYPAD_L},{SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,RETRO_DEVICE_ID_JOYPAD_R},{SDL_CONTROLLER_BUTTON_BACK,RETRO_DEVICE_ID_JOYPAD_SELECT},{SDL_CONTROLLER_BUTTON_START,RETRO_DEVICE_ID_JOYPAD_START}};
 for(unsigned p=0;p<4;++p)if(pads_[p]){
  for(auto [button,id]:bindings)if(SDL_GameControllerGetButton(pads_[p],button))core_.buttons[p]|=uint16_t(1u<<id);
  // Touchpad opens the pause menu; Options starts and Share inserts a coin.
  auto ax=SDL_GameControllerGetAxis(pads_[p],SDL_CONTROLLER_AXIS_LEFTX),ay=SDL_GameControllerGetAxis(pads_[p],SDL_CONTROLLER_AXIS_LEFTY);
  if(ax<-16000)core_.buttons[p]|=1<<RETRO_DEVICE_ID_JOYPAD_LEFT;if(ax>16000)core_.buttons[p]|=1<<RETRO_DEVICE_ID_JOYPAD_RIGHT;
  if(ay<-16000)core_.buttons[p]|=1<<RETRO_DEVICE_ID_JOYPAD_UP;if(ay>16000)core_.buttons[p]|=1<<RETRO_DEVICE_ID_JOYPAD_DOWN;
 }
}
void App::library(){
 auto p=palette(model_.settings.theme);auto v=model_.visible();
 draw_.text(320,40,model_.page==Page::Favorites?"Your favorites":"Your arcade",60,p.text,true);
 std::string count=std::to_string(v.size())+(v.size()==1?" game":" games");draw_.text(322,113,count,19,p.muted);
 if(options_.demo)draw_.text(780,113,"DESIGN PREVIEW",16,p.muted,true);
 if(v.empty()){
  draw_.text(357,330,model_.page==Page::Favorites?"Keep your favorites close.":"Your arcade starts here.",39,p.text,true);
  draw_.wrap(359,399,model_.page==Page::Favorites?"Press Triangle on a game to add it to your favorites.":"Choose your game folder in Settings. Your arcade games will appear here.",25,p.muted,610);
  if(model_.page==Page::Library){draw_.button(359,505,270,66,"Open Settings",{244,82,24,255},p.on_accent);}
  return;
 }
 size_t first=(model_.selected/6)*6;
 for(size_t i=first;i<std::min(first+6,v.size());++i){auto& g=model_.games[v[i]];int j=static_cast<int>(i-first);float x=grid_x+(j%3)*col_step,y=grid_y+(j/3)*row_step;bool focused=i==model_.selected&&!model_.rail;
  if(focused){draw_.rounded(x-8,y-8,card_w+16,card_h+16,13,p.accent);draw_.rounded(x-5,y-5,card_w+10,card_h+10,10,p.bg);}
  draw_.cover(g.id,x,y,card_w,card_h,p.panel);
  draw_.text(x,y+card_h+10,g.title,24,p.text,true,card_w-5);
  draw_.text(x,y+card_h+39,g.year?std::to_string(g.year):"Arcade",20,p.muted);
  if(model_.favorites.count(g.id))draw_.icon(1,x+card_w-26,y+card_h+39,p.accent,p.bg);
 }
 auto* g=model_.current();if(!g)return;
 draw_.cover(g->id,1124,46,426,493,p.panel);
 draw_.text(1124,557,g->title,43,p.text,true,375);draw_.text(1126,614,g->maker,26,p.muted,false,400);
 draw_.text(1126,649,g->year?std::to_string(g->year):"Arcade",24,p.muted);
 draw_.icon(1,1510,572,model_.favorites.count(g->id)?p.accent:p.muted,p.bg);
 draw_.button(1124,718,426,68,loading_?"Starting...":"Play",{244,82,24,255},{255,255,255,255},!loading_);
 draw_.rect(1124,815,426,1,p.line);
 draw_.hint('x',1125,837,"Play",p.text,p.bg,19);draw_.hint('t',1245,837,"Favorite",p.text,p.bg,19);draw_.hint('o',1410,837,"Settings",p.text,p.bg,19);
 if(v.size()>6)draw_.text(955,835,std::to_string(first/6+1)+" / "+std::to_string((v.size()+5)/6),18,p.muted);
}
void App::settings(){
 auto p=palette(model_.settings.theme);draw_.text(320,40,"Make it yours",60,p.text,true);draw_.text(322,113,"Just the essentials.",23,p.muted);
 const int tops[]={181,320,456,592};
 for(int i=0;i<4;++i){if(model_.setting==i&&!model_.rail){draw_.rounded(318,float(tops[i])-2,1222,104,15,p.accent);}draw_.rounded(320,float(tops[i]),1218,100,13,p.panel);}
 draw_.text(347,193,"Appearance",30,p.text,true);draw_.text(348,232,"Choose your light.",22,p.muted);
 for(int i=0;i<2;++i){bool on=(model_.settings.theme==Theme::Night)==bool(i);draw_.button(832+i*210,201,190,60,i?"Night":"Day",on?SDL_Color{244,82,24,255}:p.bg,on?p.on_accent:p.text);}
 draw_.text(347,332,"Game folder",30,p.text,true);draw_.text(348,371,model_.settings.roms.empty()?"Choose where your games live":model_.settings.roms.string(),22,p.muted,false,930);draw_.button(1325,340,182,60,"Edit",p.bg,p.text);
 draw_.text(347,468,"Refresh library",30,p.text,true);draw_.text(348,507,"Find games added to your folder.",22,p.muted);draw_.button(1325,476,182,60,"Refresh",p.bg,p.text);
 draw_.text(347,604,"Picture",30,p.text,true);draw_.text(348,643,"Original proportions, always.",22,p.muted);draw_.text(1250,622,model_.settings.smooth?"Smooth":"Sharp pixels",25,p.accent,true);
 draw_.text(322,754,"Day and Night keep the same layout and controls.",24,p.muted);
 draw_.text(322,792,"Your choices are saved automatically.",22,p.muted);
}
void App::footer(){auto p=palette(model_.settings.theme);draw_.text(320,861,"L1 / R1   Browse sections",19,p.muted);draw_.hint('b',830,853,"Back",p.text,p.bg,18);}

void App::game(){
 draw_.rect(0,0,1600,900,{0,0,0,255});
 auto w=core_.width(),h=core_.height();if(!w||!h)return;
 if(!frame_||frame_w_!=w||frame_h_!=h){SDL_DestroyTexture(frame_);frame_=SDL_CreateTexture(renderer_,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);frame_w_=w;frame_h_=h;}
 if(!frame_)return;SDL_SetTextureScaleMode(frame_,model_.settings.smooth?SDL_ScaleModeLinear:SDL_ScaleModeNearest);
 SDL_UpdateTexture(frame_,nullptr,core_.pixels().data(),w*4);
 float aspect=core_.aspect()>0?core_.aspect():float(w)/h;
 float fw=std::min(1600.f,900.f*aspect),fh=fw/aspect;SDL_FRect dst{(1600-fw)/2,(900-fh)/2,fw,fh};SDL_RenderCopyF(renderer_,frame_,nullptr,&dst);
 if(paused_){auto p=palette(model_.settings.theme);draw_.rect(0,0,1600,900,{0,0,0,170});draw_.rect(490,282,620,390,p.bg);draw_.text(542,316,"Take a breather",43,p.text,true);draw_.text(542,382,"Your game is paused.",25,p.muted);
 draw_.button(540,475,520,65,"Resume",{244,82,24,255},p.on_accent,true);
 draw_.button(540,555,520,65,"Return to library",p.panel,p.text);}
}
void App::modal(){auto p=palette(model_.settings.theme);draw_.rect(0,0,1600,900,{0,0,0,150});draw_.rect(300,280,1000,350,p.bg);
 if(editing_){draw_.text(348,311,"Your game folder",40,p.text,true);draw_.text(348,375,"Enter the folder path, then press Enter.",24,p.muted);draw_.rect(348,429,903,63,p.panel);draw_.text(365,442,edit_+"|",25,p.text,false,865);draw_.text(348,510,notice_,21,p.accent,false,900);draw_.text(348,560,"Enter  Save     Esc  Cancel",22,p.muted);}
 else{draw_.text(348,311,"A quick note",40,p.text,true);draw_.wrap(348,385,notice_,27,p.text,880,4);draw_.text(348,557,"X / Enter  Continue",23,p.accent,true);}
}
void App::render(){
 auto p=palette(model_.settings.theme);draw_.rect(0,0,1600,900,p.bg);
 if(core_.playing()){game();if(!notice_.empty())modal();return;}
 draw_.rect(270,35,1,808,p.line);draw_.rect(1084,35,1,808,p.line);
 draw_.text(43,68,"ARCADE",51,p.text,true);draw_.rect(45,135,194,6,p.accent);
 const char* titles[]={"Library","Favorites","Settings"};
 for(int i=0;i<3;++i){float y=213+i*78;bool active=static_cast<int>(model_.page)==i;
  if(active&&model_.rail)draw_.rect(27,y-8,219,58,p.focus);
  if(active)draw_.rect(27,y-5,3,47,p.accent);
  draw_.icon(i,48,y+6,active?p.accent:p.muted,p.bg);draw_.text(96,y,titles[i],29,active?p.accent:p.text,active);
 }
 draw_.text(46,815,model_.settings.theme==Theme::Day?"DAY THEME":"NIGHT THEME",16,p.muted,true);
 if(model_.page==Page::Settings){draw_.rect(1080,30,10,820,p.bg);settings();}else library();footer();if(editing_||!notice_.empty())modal();
 if(loading_){draw_.rect(0,0,1600,900,{0,0,0,150});draw_.rect(570,377,460,140,p.bg);draw_.text(635,420,"Starting your game...",30,p.text,true);}
}
int App::run(){
 using Clock=std::chrono::steady_clock;auto next=Clock::now();
 for(;running_;++frames_){
  SDL_Event e;while(SDL_PollEvent(&e))event(e);
  if(options_.ui_smoke){
   auto press=[&](SDL_Keycode key){SDL_Event ev{};ev.type=SDL_KEYDOWN;ev.key.keysym.sym=key;SDL_PushEvent(&ev);};
   if(frames_==2)press(SDLK_t);if(frames_==4)press(SDLK_f);if(frames_==6)press(SDLK_TAB);if(frames_==8)press(SDLK_s);if(frames_==10)press(SDLK_RETURN);if(frames_==12)press(SDLK_ESCAPE);
   if(frames_==14){auto persisted=read_settings(options_.data/"settings.cfg");auto favorites=read_favorites(options_.data/"favorites.txt");if(persisted.theme!=model_.settings.theme||favorites.count("robby")!=1||model_.page!=Page::Library)throw std::runtime_error("UI smoke flow failed");std::cout<<"UI smoke: keyboard events, favorites, settings and theme persistence passed\n";running_=false;}
  }
  if(core_.playing()&&!paused_){game_input();core_.run();if(core_.shutdown())stop();}
  render();
  if(!options_.capture.empty()&&frames_==2){draw_.capture(options_.capture);running_=false;}
  SDL_RenderPresent(renderer_);
  if(options_.run_frames&&frames_>=options_.run_frames)running_=false;
  auto rate=core_.playing()&&!paused_?core_.fps():60.;next+=std::chrono::microseconds(static_cast<long long>(1000000/rate));
  if(Clock::now()<next)std::this_thread::sleep_until(next);else next=Clock::now();
 }
 return 0;
}
}
