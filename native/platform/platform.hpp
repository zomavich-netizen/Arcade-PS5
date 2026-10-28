#pragma once
#include "platform/types.hpp"
#include "audio_output.hpp"
#include "settings.hpp"
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace arcade::native {
struct Input {
 std::vector<Event> events;
 std::array<std::uint16_t,4> pads{};
 bool quit=false,lost_focus=false,click=false,double_click=false;
 float mouse_x=0,mouse_y=0;
};
class Platform {
public:
 virtual ~Platform()=default;
 virtual void configure(const Settings&){}
 virtual void pulse(unsigned,unsigned=95){}
 virtual void suspend_display(){}
 virtual bool resume_display(std::string&){return true;}
 virtual bool open(std::string& error)=0;
 virtual Input poll()=0;
 virtual bool present()=0;
 virtual void size(int& width,int& height)=0;
};
std::unique_ptr<Platform> make_platform();
std::unique_ptr<AudioOutput> make_audio();
}
