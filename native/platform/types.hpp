#pragma once
#include <cstdint>
namespace arcade::native {
struct Color {float r,g,b,a=1;};
struct Rect {float x,y,w,h;};
enum class Key { Cross,Circle,Square,Triangle,Options,L1,R1,Up,Down,Left,Right,Touchpad,L3,R3,L2,R2 };
struct Event {Key key;};
struct Frame {const std::uint32_t* pixels=nullptr;unsigned width=0,height=0;float aspect=0;bool ready()const noexcept{return pixels!=nullptr&&width!=0&&height!=0;}};
}
