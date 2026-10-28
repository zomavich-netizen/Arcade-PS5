// SPDX-License-Identifier: GPL-3.0-or-later
#include "prosperoeden/radio_input.h"
#include <cassert>
#include <cstdint>
#include <cstring>
static unsigned char state[120];
static int next_count=1,state_reads=0;
extern "C" {
int scePadInit(){return 0;}
int scePadOpen(int,int,int,const void*){return 7;}
int scePadClose(int){return 0;}
int scePadSetVibration(int,const void*){return 0;}
int scePadSetVibrationMode(int,int){return 0;}
int scePadRead(int,void* output,int){if(next_count==1)std::memcpy(output,state,sizeof state);return next_count;}
int scePadReadState(int,void*){++state_reads;return -1;}
int scePadGetHandle(int,int,int){return -1;}
int sceUserServiceInitialize(void*){return 0;}
int sceUserServiceGetInitialUser(int* user){*user=101;return 0;}
int sceUserServiceGetLoginUserIdList(void* list){int users[4]={101,-1,-1,-1};std::memcpy(list,users,sizeof users);return 0;}
int sceUserServiceTerminate(){return 0;}
}
int main(){
 assert(radio_input_init());
 state[4]=state[5]=128;state[76]=1;
 std::uint32_t buttons=0x4000|0x80|0x2;std::memcpy(state,&buttons,4);
 radio_input_poll();unsigned char sample[120]{};
 assert(arcade_radio_input_sample(0,sample,sizeof sample));
 assert(std::memcmp(sample,state,sizeof state)==0);
 assert(radio_input_pressed(RADIO_INPUT_CROSS));
 assert(radio_input_controllers()&1);
 assert(state_reads==0); // Primary gameplay does not depend on a separate ReadState call.
 assert(!arcade_radio_input_sample(1,sample,sizeof sample));
 assert(!arcade_radio_input_sample(0,sample,119));
 next_count=0;radio_input_poll();assert(arcade_radio_input_sample(0,sample,sizeof sample));
 next_count=1;buttons=0;std::memcpy(state,&buttons,4);radio_input_poll();
 assert(arcade_radio_input_sample(0,sample,sizeof sample));
 assert(!radio_input_pressed(RADIO_INPUT_CROSS));
 std::memcpy(&buttons,sample,4);assert(buttons==0);
 next_count=-1;radio_input_poll();assert(!arcade_radio_input_sample(0,sample,sizeof sample));
 radio_input_shutdown();
}
