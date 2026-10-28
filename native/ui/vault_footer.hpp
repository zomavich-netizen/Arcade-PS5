#pragma once
#include "gfx/renderer.hpp"
#include <array>
namespace arcade::native {
struct FooterHint {const char* glyph;const char* label;Key action;Rect bounds;};
inline std::array<FooterHint,6> footer_layout(bool large_text=false,bool swap_menu=false){
 std::array<FooterHint,6> hints{{{"L3","Insert coin",Key::L3,{}},{swap_menu?"O":"X","Play",Key::Cross,{}},{"[]","Settings",Key::Square,{}},{"/\\","Favorite",Key::Triangle,{}},{swap_menu?"X":"O","Library / Favorites",Key::Circle,{}},{"TP","All settings",Key::Touchpad,{}}}};
 float x=48;for(size_t i=0;i<hints.size();++i){float width=39+text_width(hints[i].label,18,0,large_text);hints[i].bounds={i==5?1624-width:x,882,width,35};if(i<5)x+=width+32;}return hints;
}
}
