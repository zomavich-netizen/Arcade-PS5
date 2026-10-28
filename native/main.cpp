#include "platform/platform.hpp"
#include "gfx/renderer.hpp"
#include "gfx/png_image.hpp"
#include "ui/gallery.hpp"
#include "backend/mame_session.hpp"
#include "settings.hpp"
#include "audio/menu_audio.hpp"
#ifdef ARCADE_HAS_VULKAN
#include "gfx/vulkan_processor.hpp"
#endif
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>
#include <sstream>
#include <iomanip>
#ifdef ARCADE_PS5
extern "C" void arcade_startup_note(const char*);
extern "C" void arcade_log_settings(bool);
#else
static void arcade_startup_note(const char*) {}
static void arcade_log_settings(bool) {}
#endif
#ifndef ARCADE_ROOT
#define ARCADE_ROOT "."
#endif
int main(int argc,char** argv){
 using namespace arcade;using namespace arcade::native;
 arcade_startup_note("main entered");
 try{
#ifdef ARCADE_PS5
  std::filesystem::path assets="/app0/assets",data="/download0";
#else
  std::filesystem::path root=ARCADE_ROOT,assets=root/"assets",data=root/".local/native-user";
#endif
  std::string capture;int settings_capture=-2;bool demo=false,smoke=false,settings_smoke=false;unsigned frame_limit=0;std::string roms,engine,theme;
  for(int i=1;i<argc;++i){std::string arg=argv[i];auto value=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+arg);return std::string(argv[++i]);};if(arg=="--demo")demo=true;else if(arg=="--data")data=value();else if(arg=="--roms")roms=value();else if(arg=="--core")engine=value();else if(arg=="--theme")theme=value();else if(arg=="--capture")capture=value();else if(arg=="--settings")settings_capture=-1;else if(arg=="--settings-category")settings_capture=std::stoi(value());else if(arg=="--settings-smoke")settings_smoke=true;else if(arg=="--play-smoke")smoke=true;else if(arg=="--frames")frame_limit=std::stoul(value());else throw std::runtime_error("Unknown option: "+arg);}
  arcade_startup_note("reading preferences");
  auto preferences=read_settings(data/"settings.cfg");
#ifdef ARCADE_PS5
  // Keep game data outside the application so title updates preserve it.
  preferences.roms=ps5_rom_directory(preferences.roms);
#else
  if(preferences.roms.empty())preferences.roms=root/"roms";
#endif
  if(!roms.empty())preferences.roms=std::filesystem::absolute(roms);if(!engine.empty())preferences.core=std::filesystem::absolute(engine);if(theme=="night")preferences.theme=Theme::Night;else if(theme=="day")preferences.theme=Theme::Day;else if(!theme.empty())throw std::runtime_error("Theme must be day or night");
  arcade_startup_note("opening EGL display and controller");
  auto platform=make_platform();platform->configure(preferences);arcade_log_settings(preferences.immediate_logs);std::string error;if(!platform->open(error))throw std::runtime_error(error);
  arcade_startup_note("initializing renderer");
  Renderer renderer;if(!renderer.init(assets.string(),error))throw std::runtime_error(error);
#ifdef ARCADE_HAS_VULKAN
  VulkanProcessor vulkan;bool vulkan_ready=false,vulkan_failed=false;std::string vulkan_notice;unsigned notice_frames=0;
  auto restore_gallery=[&](){vulkan.close();vulkan_ready=false;if(!platform->resume_display(error)||!renderer.init(assets.string(),error))throw std::runtime_error(error);};
#endif
  arcade_startup_note("loading gallery");
  Gallery gallery(data,preferences,demo,assets/"mame-artwork.tsv");if(settings_capture>=-1)gallery.show_settings(settings_capture);auto audio=make_audio();MenuAudio menu_audio;
  if(!menu_audio.load(assets/"arcade-menu.wav"))arcade_startup_note("Menu soundtrack unavailable; UI sounds remain available");
  if(!smoke&&!settings_smoke){
   bool intro_audio=audio->configure(48000);menu_audio.reset_clock();int iw,ih;platform->size(iw,ih);renderer.begin(iw,ih,{0,0,0,1});renderer.startup(0,preferences.reduce_motion);platform->present();
   auto intro_begin=std::chrono::steady_clock::now();auto intro_next=intro_begin;bool skip=false;
   while(!skip){auto input=platform->poll();if(input.quit){audio->stop();renderer.close();return 0;}for(auto event:input.events)if(event.key==Key::Cross||event.key==Key::Circle||event.key==Key::Options)skip=true;
    float progress=std::chrono::duration<float>(std::chrono::steady_clock::now()-intro_begin).count()/2.f;if(progress>=1)break;
    platform->size(iw,ih);renderer.begin(iw,ih,{0,0,0,1});renderer.startup(progress,preferences.reduce_motion);if(intro_audio)menu_audio.pump(*audio,preferences);platform->present();
    intro_next+=std::chrono::microseconds(16667);std::this_thread::sleep_until(intro_next);
   }
   renderer.release_startup();audio->stop();arcade_startup_note("CRT startup splash completed");
  }
  MameSession session(audio.get());bool done=false;PauseMenu pause_menu;unsigned long run_frame=0;auto next=std::chrono::steady_clock::now();auto emulation_time=next,statistics_time=next;double emulation_budget=1,shown_fps=0,shown_speed=100,shown_ms=0;unsigned display_frames=0,slow_menu_notes=0;unsigned long emulated_frames=0;bool menu_audio_ready=false;auto stop_game=[&](){gallery.refund_credits(session.pending_credits());session.stop();};
  arcade_startup_note(image_diagnostics_enabled()?"image diagnostics enabled explicitly":"image diagnostics disabled; no automatic capture or texture readback");
  arcade_startup_note("entering frame loop");
  for(unsigned frame=0;!done;++frame){auto frame_started=std::chrono::steady_clock::now();auto input=platform->poll();if(input.quit)break;
   if(settings_smoke){
    const Key sequence[]={
     Key::Touchpad,Key::Left,Key::Right,Key::Cross,Key::Down,Key::Right,Key::Left,Key::Circle,
     Key::Right,Key::Cross,Key::Right,Key::Left,Key::Circle,
     Key::Down,Key::Cross,Key::Cross,Key::Cross,Key::Circle,
     Key::Left,Key::Cross,Key::Down,Key::Down,Key::Cross,Key::Right,Key::Left,Key::Circle,Key::Circle,Key::Circle,
     Key::Square,Key::R2,Key::Down,Key::Right,Key::Left,Key::Circle,
     Key::Touchpad,Key::Down,Key::Down,Key::Right,Key::Cross,Key::Cross,Key::Options,Key::Circle,Key::Circle};
    if(frame<sizeof(sequence)/sizeof(sequence[0]))input.events.push_back({sequence[frame]});
    else {if(gallery.page()!=Page::Library||gallery.preferences().output!=0||gallery.preferences().large_text||gallery.preferences().action_buttons[0]!=0||!gallery.current())throw std::runtime_error("Settings navigation or folder selection regression");std::cout<<"Native settings: categories, paging, adjustments, mapping, accessibility, persistence and folder selection passed\n";done=true;}
   }
   if(smoke){
    if(frame==2)input.events.push_back({Key::L3});if(frame==3)input.events.push_back({Key::Cross});
    if(frame==4&&gallery.choosing_start()){input.events.push_back({Key::Down});input.events.push_back({Key::Cross});}
    if(frame==185){if(!session.active()||session.frames()<100)throw std::runtime_error("MAME never produced gameplay frames");input.events.push_back({Key::Touchpad});run_frame=session.frames();}
    if(frame==188){if(session.frames()!=run_frame)throw std::runtime_error("Pause did not stop game frames");input.events.push_back({Key::Square});}
    if(frame==190){if(!session.active()||!pause_menu.paused()||pause_menu.result()!=SaveResult::Success||session.frames()!=run_frame)throw std::runtime_error("Save did not preserve the paused game");input.events.push_back({Key::Circle});}
    if(frame==193){if(session.active())throw std::runtime_error("Exit did not close MAME");std::cout<<"Native frontend: MAME load, 100+ frames, pause, independent Save and Exit passed\n";done=true;}
   }
   gallery.controller(input.pads[0]);
   if(session.active()){
    if(input.lost_focus&&gallery.preferences().pause_on_focus)pause_menu.open(session);
    for(auto event:input.events){
     if(event.key==Key::Touchpad){if(pause_menu.paused())pause_menu.apply(PauseAction::Resume,session,stop_game);else pause_menu.open(session);}
     else if(pause_menu.paused()){
      pause_menu.scroll(event.key,gallery.paused_layout(pause_menu).pages);
      if(pause_menu.apply(PauseMenu::action(event.key,gallery.preferences().swap_menu),session,stop_game)){gallery.return_to_library();break;}
     }
    }
    if(input.click&&pause_menu.paused()){
     auto layout=gallery.paused_layout(pause_menu);
     if(layout.pages>1&&message_contains(layout.previous,input.mouse_x,input.mouse_y))pause_menu.scroll(Key::L1,layout.pages);
     else if(layout.pages>1&&message_contains(layout.next,input.mouse_x,input.mouse_y))pause_menu.scroll(Key::R1,layout.pages);
     else if(pause_menu.apply(layout.hit(input.mouse_x,input.mouse_y),session,stop_game))gallery.return_to_library();
    }
   }else{for(auto event:input.events)gallery.input(event.key);if(input.click)gallery.click(input.mouse_x,input.mouse_y,input.double_click);}
   const auto& prefs=gallery.runtime_preferences();session.preferences(prefs);arcade_log_settings(prefs.immediate_logs);
   if(!session.active()){
    if(!menu_audio_ready){menu_audio_ready=audio->configure(48000);menu_audio.reset_clock();arcade_startup_note("Main-hall audio configured; soundtrack is excluded from gameplay");}
    int cue=gallery.take_feedback();const auto& feedback=gallery.feedback_preferences();
    if(cue==3){auto* game=gallery.current();std::string note="Collection boundary game="+std::string(game?game->id:"none")+" vibration="+std::to_string(feedback.vibration)+" strength="+std::to_string(feedback.vibration_strength);arcade_startup_note(note.c_str());}
    if(menu_audio_ready)menu_audio.pump(*audio,feedback,cue);
    if((cue==2||cue==3)&&feedback.vibration)platform->pulse(cue==3?std::min(35,feedback.vibration_strength):feedback.vibration_strength);
   }else menu_audio_ready=false;
   int w=0,h=0;
   if(auto game=gallery.take_launch()){
    platform->size(w,h);renderer.begin(w,h,{0,0,0,1});gallery.loading(renderer);platform->present();
#ifdef ARCADE_HAS_VULKAN
    vulkan_failed=false;vulkan_notice.clear();
#endif
    if(!session.start(*game,gallery.preferences().core,gallery.profile_data()))gallery.error(game->title+"\n\n"+session.error(),session.error_code());
    else if(gallery.load_requested()){if(!session.load_state()){auto message=session.error(),code=session.error_code();session.stop();gallery.error(game->title+"\n\n"+message,code);}}
    else session.queue_credits(gallery.take_credits(),gallery.take_start());
    gallery.finish_launch();menu_audio_ready=false;emulation_time=std::chrono::steady_clock::now();emulation_budget=1;emulated_frames=0;
   }
   if(session.active()){
    if(!pause_menu.paused()){if(smoke)session.run(input.pads);else{auto tick=std::chrono::steady_clock::now();emulation_budget=std::min(8.,emulation_budget+std::chrono::duration<double>(tick-emulation_time).count()*session.fps()*speed_multiplier(prefs.speed));emulation_time=tick;while(emulation_budget>=1&&session.active()){session.run(input.pads);emulation_budget-=1;if(session.finished())stop_game();}}if(session.finished())stop_game();}
    else{emulation_time=std::chrono::steady_clock::now();emulation_budget=1;}
   }
   bool native_vulkan=false;
#ifdef ARCADE_HAS_VULKAN
   bool want_vulkan=session.active()&&session.frame().ready()&&!pause_menu.paused()&&prefs.renderer==1&&!vulkan_failed;
   if(want_vulkan&&!vulkan.display_open()){
    if(!vulkan_ready)vulkan_ready=vulkan.open(error,assets.string());
    if(vulkan_ready){renderer.close();platform->suspend_display();if(!vulkan.open_display(prefs,error)){restore_gallery();vulkan_failed=true;}}
    else vulkan_failed=true;
    if(vulkan_failed){vulkan_notice="Vulkan unavailable; using OpenGL. "+error;notice_frames=420;arcade_startup_note(vulkan_notice.c_str());}
    else arcade_startup_note("Direct Vulkan display opened; OpenGL context is closed");
   }
   if(!want_vulkan&&vulkan.display_open()){restore_gallery();arcade_startup_note("Returned from Vulkan to OpenGL Gallery");}
   native_vulkan=vulkan.display_open();
#endif
   if(!native_vulkan){platform->size(w,h);renderer.begin(w,h,prefs.theme==Theme::Day?Color{.98f,.969f,.941f,1}:Color{.067f,.086f,.118f,1});if(session.active()){if(pause_menu.paused())gallery.paused(renderer,session.frame(),pause_menu);else if(!session.frame().ready())gallery.loading(renderer);else renderer.game(session.frame(),prefs);}else gallery.draw(renderer);}
#ifdef ARCADE_HAS_VULKAN
   else{Frame processed;if(!vulkan.process(session.frame(),prefs,processed,error)){vulkan_failed=true;vulkan_notice="Vulkan frame failed; using OpenGL. "+error;notice_frames=420;arcade_startup_note(vulkan_notice.c_str());continue;}}
#endif
   if(session.active()&&prefs.overlay&&!pause_menu.paused()){
    ++display_frames;auto tick=std::chrono::steady_clock::now();double elapsed=std::chrono::duration<double>(tick-statistics_time).count();shown_ms=shown_ms*.9+std::chrono::duration<double,std::milli>(tick-frame_started).count()*.1;
    if(elapsed>=.5){shown_fps=display_frames/elapsed;auto produced=session.frames()>=emulated_frames?session.frames()-emulated_frames:0;shown_speed=100*produced/(elapsed*session.fps());display_frames=0;emulated_frames=session.frames();statistics_time=tick;}
    std::ostringstream line;line<<std::fixed<<std::setprecision(1);if(prefs.hud_backend)line<<(native_vulkan?"MAME / Vulkan   ":"MAME / OpenGL   ");if(prefs.hud_fps)line<<shown_fps<<" FPS   ";if(prefs.hud_ms)line<<shown_ms<<" ms   ";if(prefs.hud_speed)line<<shown_speed<<"%   ";if(prefs.hud_resolution)line<<session.frame().width<<"x"<<session.frame().height;
    auto text=line.str();
#ifdef ARCADE_HAS_VULKAN
    if(native_vulkan)vulkan.overlay(text,prefs);else
#endif
    if(!text.empty()){float size=20+prefs.hud_size*4,width=std::min(1608.f,renderer.measure(text,size,1)+32),height=size+32,x=prefs.hud_corner%2?1640-width:32,y=prefs.hud_corner>=2?909-height:32;renderer.rectangle({x,y,width,height},{.03f,.04f,.06f,.92f},12);unsigned accent=accent_color(prefs.accent);Color ink=prefs.hud_colors?Color{((accent>>16)&255)/255.f,((accent>>8)&255)/255.f,(accent&255)/255.f,1}:Color{1,1,1,1};renderer.text(x+16,y+17,text,size,ink,1,width-32);}
   }else{statistics_time=std::chrono::steady_clock::now();display_frames=0;emulated_frames=session.frames();}
#ifdef ARCADE_HAS_VULKAN
   if(!native_vulkan&&notice_frames){--notice_frames;renderer.rectangle({32,32,1608,74},{.05f,.06f,.09f,.95f},12);renderer.text(52,54,vulkan_notice,22,{1,1,1,1},0,1560);}
#endif
#ifdef ARCADE_PS5
   if(image_diagnostics_enabled()&&frame==90&&!session.active())renderer.capture("/data/arcade-ui-diagnostic.ppm",w,h);
#endif
   if(!native_vulkan&&!capture.empty()&&frame==45){renderer.capture(capture,w,h);done=true;}
#ifdef ARCADE_HAS_VULKAN
   if(native_vulkan){if(!vulkan.present(error)){vulkan_failed=true;vulkan_notice="Vulkan presentation failed; using OpenGL. "+error;notice_frames=420;arcade_startup_note(vulkan_notice.c_str());}}
   else
#endif
   platform->present();if(frame==0)arcade_startup_note("first frame presented");if(frame_limit&&frame>=frame_limit)done=true;
   if(!session.active()&&slow_menu_notes<8){double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-frame_started).count();if(ms>100){++slow_menu_notes;auto note="menu frame "+std::to_string(frame)+" took "+std::to_string(ms)+" ms";arcade_startup_note(note.c_str());}}
   double fps=preferences.refresh?120.:60.;next+=std::chrono::microseconds(static_cast<long long>(1000000/fps));auto now=std::chrono::steady_clock::now();if(next>now)std::this_thread::sleep_until(next);else next=now;
  }
  session.stop();
#ifdef ARCADE_HAS_VULKAN
  if(vulkan.display_open())vulkan.close();else renderer.close();
#else
  renderer.close();
#endif
  return 0;
 }catch(const std::exception& e){arcade_startup_note(e.what());std::cerr<<"Arcade native: "<<e.what()<<'\n';return 1;}
}
