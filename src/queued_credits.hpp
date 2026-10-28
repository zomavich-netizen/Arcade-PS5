#pragma once
#include "libretro.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace arcade {
// Run in emulation updates, not displayed frames. A boot-locked coin input
// keeps the queue intact; games without lockout signals get a startup grace.
class QueuedCredits {
 unsigned remaining_=0,ticks_=0,unlocked_=0,hold_=0,gap_=0;
 bool start_=false,saw_lockout_=false,awaiting_=false;int baseline_=-1;unsigned attempts_=0;std::uint16_t pulse_=0;
public:
 void reset(unsigned count=0,bool start=false){remaining_=count;start_=start;ticks_=unlocked_=hold_=gap_=0;saw_lockout_=false;awaiting_=false;baseline_=-1;attempts_=0;pulse_=0;}
 unsigned remaining()const{return remaining_;}
 bool pending()const{return remaining_||start_||hold_;}
 std::uint16_t step(bool video,int lockout,double fps,int meter=-1){
  fps=fps>1?fps:60;
  if(awaiting_&&meter>=0){if(meter<baseline_)baseline_=meter;else if(meter>baseline_){--remaining_;awaiting_=false;attempts_=0;gap_=unsigned(std::ceil(fps*(remaining_?.2:.5)));}}
  ++ticks_;if(lockout>0){saw_lockout_=true;unlocked_=0;}else if(video)++unlocked_;
  if(hold_){--hold_;return pulse_;}if(gap_){--gap_;return 0;}
  bool ready=video&&lockout<=0&&ticks_>=unsigned(std::ceil(fps*(meter>=0?6:2)))&&(saw_lockout_?unlocked_>=unsigned(std::ceil(fps*.25)):ticks_>=unsigned(std::ceil(fps*6)));
  if(!ready)return 0;
  if(remaining_){
   if(meter>=0){if(attempts_>=5)return 0;baseline_=meter;awaiting_=true;++attempts_;gap_=unsigned(std::ceil(fps*2));}
   else{--remaining_;gap_=unsigned(std::ceil(fps*(remaining_?.2:.5)));}
   pulse_=1u<<RETRO_DEVICE_ID_JOYPAD_SELECT;
  }
  else if(start_){start_=false;pulse_=1u<<RETRO_DEVICE_ID_JOYPAD_START;}
  else return 0;
  hold_=std::max(2u,unsigned(std::ceil(fps*(meter>=0?.2:.08))))-1;return pulse_;
 }
};
}
