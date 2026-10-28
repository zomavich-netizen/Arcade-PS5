#pragma once
#include "platform/types.hpp"
#include <string>
#include "settings.hpp"
#include <vector>
#include <map>
#include <set>
#include <chrono>
namespace arcade::native {
float text_width(const std::string&,float size,int weight=0,bool large_text=false);
// Custom OpenGL UI renderer. PS5 owns its context through EGL/ps5-opengl;
// the host preview supplies a native desktop OpenGL context.
class Renderer {
public:
 bool init(const std::string& asset_directory,std::string& error);
 void close();void begin(int width,int height,Color background);
 void rectangle(Rect,Color,float radius=0);
 void text(float x,float y,const std::string&,float size,Color,int weight=0,float max_width=10000);
 float measure(const std::string&,float size,int weight=0)const;
 void cover(const std::string& id,Rect,float radius=6,float opacity=1,bool contain=false);
 void asset(const std::string& name,Rect,Color tint={1,1,1,1});
 void line(float,float,float,float,float,Color);
 void circle(float,float,float,Color);
 void cover_directory(const std::string&);
 void sidebar_icon(int index,Rect,Color);
 void keycap(const std::string& glyph,float x,float y,float opacity=1);
 void raised(Rect,float opacity=1);
 void button(Rect,const std::string&,const std::string& glyph="",bool selected=false,float opacity=1,float text_size=23);
 void controller_hint(int index,float x,float y,const std::string&,Color ink,Color paper);
 void play(Rect,Color,Color,const std::string& label="Play",bool arrow=true);
 void game(const Frame&,bool smooth);
 void game(const Frame&,const Settings&);
 void wallpaper(const std::string& path);
 void arcade_background(bool day,bool reduce_motion);
 void startup(float progress,bool reduce_motion);
 void release_startup();
 void large_text(bool enabled){large_text_=enabled;}

 void capture(const std::string& path,int width,int height);
private:
 struct CoverTexture {unsigned texture=0;float aspect=1;unsigned long used=0;};
 std::map<std::string,CoverTexture> covers_;std::map<std::string,std::chrono::steady_clock::time_point> cover_probes_;std::string cover_directory_;unsigned long cover_use_=0;
 std::set<std::string> cover_failures_;
 bool cached_cover(const std::string&,Rect,float,float,bool);
 std::string asset_directory_;
 std::map<std::string,unsigned> assets_;
 bool large_text_=false;std::string wallpaper_path_;unsigned wallpaper_=0;
 unsigned program_=0,vao_=0,textures_[3]{},cover_=0,fallback_=0,icons_[3]{},symbols_[3]{},game_=0;
 int loc_axes_=0,loc_rect_=0,loc_uv_=0,loc_color_=0,loc_radius_=0,loc_textured_=0,loc_sampler_=0,loc_rotation_=0,loc_effect_=0,loc_source_=0;
 std::chrono::steady_clock::time_point atmosphere_start_=std::chrono::steady_clock::now();
 int loc_ambient_=0,loc_time_=0;
 unsigned load_asset(const std::string&);
 unsigned image(unsigned char*,int,int);
 void quad(Rect,Color,float radius,unsigned texture=0,Rect uv={0,0,1,1},float angle=0);
 void triangle(float,float,float,Color);
};
}
