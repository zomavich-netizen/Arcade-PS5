#pragma once
#include "settings.hpp"
#include "libretro.h"
#include <cstdint>
namespace arcade {
inline std::uint16_t map_controls(std::uint16_t physical,const Settings& settings){
 constexpr unsigned face[]={RETRO_DEVICE_ID_JOYPAD_B,RETRO_DEVICE_ID_JOYPAD_A,RETRO_DEVICE_ID_JOYPAD_Y,RETRO_DEVICE_ID_JOYPAD_X};
 constexpr unsigned targets[]={RETRO_DEVICE_ID_JOYPAD_B,RETRO_DEVICE_ID_JOYPAD_A,RETRO_DEVICE_ID_JOYPAD_Y,RETRO_DEVICE_ID_JOYPAD_X,RETRO_DEVICE_ID_JOYPAD_L,RETRO_DEVICE_ID_JOYPAD_R,RETRO_DEVICE_ID_JOYPAD_L2,RETRO_DEVICE_ID_JOYPAD_R2};
 auto held=[physical](unsigned id){return physical&(1u<<id);};
 unsigned out=physical;for(auto bit:face)out&=~(1u<<bit);
 out&=~((1u<<RETRO_DEVICE_ID_JOYPAD_SELECT)|(1u<<RETRO_DEVICE_ID_JOYPAD_START)|(1u<<RETRO_DEVICE_ID_JOYPAD_L3)|(1u<<RETRO_DEVICE_ID_JOYPAD_R3));
 for(unsigned i=0;i<4;++i)if(held(face[i]))out|=1u<<targets[settings.action_buttons[i]];
 bool coin=settings.coin_button==0?(held(RETRO_DEVICE_ID_JOYPAD_L3)||held(RETRO_DEVICE_ID_JOYPAD_SELECT)):settings.coin_button==1?held(RETRO_DEVICE_ID_JOYPAD_L3):settings.coin_button==2?held(RETRO_DEVICE_ID_JOYPAD_SELECT):held(RETRO_DEVICE_ID_JOYPAD_L);
 bool start=settings.start_button==0?(held(RETRO_DEVICE_ID_JOYPAD_START)||held(RETRO_DEVICE_ID_JOYPAD_R3)):settings.start_button==1?held(RETRO_DEVICE_ID_JOYPAD_START):held(RETRO_DEVICE_ID_JOYPAD_R3);
 if(coin)out|=1u<<RETRO_DEVICE_ID_JOYPAD_SELECT;if(start)out|=1u<<RETRO_DEVICE_ID_JOYPAD_START;return static_cast<std::uint16_t>(out);
}
}
