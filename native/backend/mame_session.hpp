#pragma once
#include "core.hpp"
#include "queued_credits.hpp"
#include "library.hpp"
#include "control_mapping.hpp"
#include "platform/types.hpp"
#include <cstdio>
#ifdef ARCADE_PS5
extern "C" void arcade_startup_note(const char*);
#endif
namespace arcade::native {
// The launcher requests a game; this adapter owns all libretro/MAME lifetime.
// No UI or frontend platform framework is imported here.
class MameSession {
public:
 explicit MameSession(AudioOutput* output=nullptr):core_(output){}
 bool start(const Game& game,const std::filesystem::path& engine,const std::filesystem::path& data){return core_.load(engine,game.path,data);}
 bool save_state(){return core_.save_state();}
 bool load_state(){return core_.load_state();}
 void stop(){core_.stop();credits_.reset();}
 void queue_credits(unsigned count,bool start){credits_.reset(count,start);waiting_reported_=false;previous_coin_pulse_=false;
#ifdef ARCADE_PS5
 char note[96];std::snprintf(note,sizeof note,"[credits] Queued %u launcher coin(s)",count);arcade_startup_note(note);
#endif
 }
 unsigned pending_credits()const{return credits_.remaining();}
 void pause(bool paused){core_.pause_audio(paused);}
 void preferences(const Settings& s){settings_=s;core_.audio_speed(speed_multiplier(s.speed));core_.audio_settings(s.volume,s.mute);core_.log_settings(s.detailed_logging,s.immediate_logs);}
 void run(const std::array<std::uint16_t,4>& pads){for(unsigned i=0;i<4;++i)core_.buttons[i]=map_controls(pads[i],settings_);[[maybe_unused]] auto remaining=credits_.remaining();int lockout=core_.coin_lockout();int meter=core_.coin_counter();
#ifdef ARCADE_PS5
 if(remaining&&lockout>0&&!waiting_reported_){arcade_startup_note("[credits] Waiting for the machine to unlock its coin input");waiting_reported_=true;}
#endif
 auto queued=credits_.step(core_.width()>0&&core_.height()>0,lockout,core_.fps(),meter);core_.buttons[0]|=queued;
#ifdef ARCADE_PS5
 if((queued&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT))&&!previous_coin_pulse_){char note[128];std::snprintf(note,sizeof note,"[credits] Sent coin pulse; machine meter before pulse=%d",meter);arcade_startup_note(note);}
 if(credits_.remaining()<remaining){char note[96];std::snprintf(note,sizeof note,"[credits] Coin accepted or delivered; remaining=%u",credits_.remaining());arcade_startup_note(note);}
#endif
 previous_coin_pulse_=queued&(1u<<RETRO_DEVICE_ID_JOYPAD_SELECT);core_.run();}
 bool active()const{return core_.playing();}bool finished()const{return core_.shutdown();}
 const std::string& error_code()const{return core_.error_code();}
 const std::string& error()const{return core_.error();}
 Frame frame()const{return {core_.pixels().data(),core_.width(),core_.height(),core_.aspect()};}
 double fps()const{return core_.fps();}
 unsigned long frames()const{return core_.video_frames;}
private:Core core_;QueuedCredits credits_;bool waiting_reported_=false,previous_coin_pulse_=false;Settings settings_;
};
}
