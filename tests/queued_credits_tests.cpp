#include "queued_credits.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace arcade;constexpr auto coin=1u<<RETRO_DEVICE_ID_JOYPAD_SELECT,start=1u<<RETRO_DEVICE_ID_JOYPAD_START;
 QueuedCredits queue;queue.reset(3,true);unsigned coins=0,starts=0,last_coin=0,first_start=0;std::uint16_t previous=0;
 for(unsigned frame=0;frame<1200;++frame){auto bits=queue.step(frame>0,frame<600?1:0,60);if(frame<600)assert(bits==0&&queue.remaining()==3);if((bits&coin)&&!(previous&coin)){++coins;last_coin=frame;}if((bits&start)&&!(previous&start)){++starts;first_start=frame;assert(coins==3);}previous=bits;}
 assert(coins==3&&starts==1&&first_start>last_coin+20);assert(queue.remaining()==0&&!queue.pending());
 queue.reset(1,false);for(unsigned frame=0;frame<359;++frame)assert(queue.step(true,-1,60)==0);assert(queue.remaining()==1);assert(queue.step(true,-1,60)==coin);for(unsigned frame=0;frame<300;++frame)assert(!(queue.step(true,-1,60)&start));
 queue.reset(2,false);for(unsigned frame=0;frame<700;++frame)assert(queue.step(false,0,60)==0);assert(queue.remaining()==2);for(unsigned frame=0;frame<100;++frame)queue.step(true,1,60);assert(queue.remaining()==2);for(unsigned frame=0;frame<15;++frame)queue.step(true,0,60);assert(queue.remaining()==1);queue.reset();assert(!queue.pending()&&queue.remaining()==0);
 // Neo Geo can unlock before its credit bookkeeping is ready. Ignore the
 // first pulse, then acknowledge subsequent pulses with the hardware meter.
 queue.reset(2,true);unsigned meter=0,pulses=0,start_pulses=0;previous=0;
 for(unsigned frame=0;frame<1500;++frame){auto bits=queue.step(true,frame<120?1:0,60,int(meter));if((bits&coin)&&!(previous&coin)){++pulses;if(frame>=480)++meter;else assert(queue.remaining()==2);}if((bits&start)&&!(previous&start)){++start_pulses;assert(meter==2&&queue.remaining()==0);}previous=bits;}
 assert(pulses==3&&meter==2&&start_pulses==1&&!queue.pending());
 queue.reset(1,false);previous=0;pulses=0;for(unsigned frame=0;frame<2000;++frame){auto bits=queue.step(true,0,60,0);if((bits&coin)&&!(previous&coin))++pulses;previous=bits;}assert(pulses==5&&queue.remaining()==1); // never spend an unacknowledged coin
 std::cout<<"Queued coins survive boot lockout, produce exact pulses, delay Start and retain undelivered credits\n";
}
