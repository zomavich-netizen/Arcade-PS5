#pragma once
#include "settings.hpp"
#include <SDL.h>
#include <filesystem>
#include <string>
namespace arcade {
struct Palette {SDL_Color bg,panel,text,muted,line,accent,on_accent,focus;};
Palette palette(Theme theme);
class Draw {
public:
 explicit Draw(SDL_Renderer* renderer,const std::filesystem::path& assets);
 ~Draw();
 Draw(const Draw&)=delete;Draw& operator=(const Draw&)=delete;
 void rect(float x,float y,float w,float h,SDL_Color c);
 void rounded(float x,float y,float w,float h,float radius,SDL_Color color);
 void button(float x,float y,float w,float h,const std::string& label,SDL_Color background,SDL_Color foreground,bool play=false);
 void hint(char symbol,float x,float y,const std::string& label,SDL_Color ink,SDL_Color paper,float size=21);
 void outline(float x,float y,float w,float h,SDL_Color c,int thickness=2);
 void text(float x,float y,std::string value,float size,SDL_Color color,bool bold=false,float max_width=10000);
 float measure(const std::string& value,float size,bool bold=false)const;
 void wrap(float x,float y,const std::string& value,float size,SDL_Color color,float width,int lines=4);
 void cover(const std::string& id,float x,float y,float w,float h,SDL_Color color);
 void icon(int kind,float x,float y,SDL_Color color,SDL_Color paper);
 void capture(const std::filesystem::path& path);
private:
 void stroke(float,float,float,float,float,SDL_Color);
 void rounded_texture(SDL_Texture*,const SDL_Rect&,float,float,float,float,float,SDL_Color);
 SDL_Renderer* renderer_;SDL_Texture* font_[2]{};SDL_Texture* covers_=nullptr;
};
}
