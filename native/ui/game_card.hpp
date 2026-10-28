#pragma once
#include "gfx/renderer.hpp"
#include "library.hpp"
#include <sys/stat.h>
#include <ctime>
#include <sstream>
#include <cmath>
#include <algorithm>
namespace arcade::native::card {
constexpr Rect play_button{1142,726,223,64},settings_button{1377,726,223,64};
constexpr float button_text_size=24;
struct SaveInfo {bool available=false;std::time_t modified=0;};
inline SaveInfo inspect_save(const std::filesystem::path& file){struct stat info{};if(::stat(file.c_str(),&info)||!S_ISREG(info.st_mode))return {};return {true,info.st_mtime};}
inline std::string save_time(const SaveInfo& save,std::time_t now=std::time(nullptr)){
 if(!save.available)return "No save yet";
 auto* date=std::localtime(&save.modified);if(!date)return "Save found";auto then=*date;
 auto* current=std::localtime(&now);bool today=current&&current->tm_year==then.tm_year&&current->tm_yday==then.tm_yday;
 char buffer[64]{};std::strftime(buffer,sizeof buffer,today?"Today, %H:%M":"%d %b %Y, %H:%M",&then);return buffer;
}
inline std::string display_title(std::string title){auto alt=title.find(" / ");if(alt!=std::string::npos)title.resize(alt);auto variant=title.find(" (");if(variant!=std::string::npos)title.resize(variant);return title;}
inline std::vector<std::string> title_lines(Renderer& r,const std::string& title,float size,float width){
 std::vector<std::string> lines;std::istringstream words(title);std::string word,line;
 while(words>>word){auto next=line.empty()?word:line+" "+word;if(!line.empty()&&r.measure(next,size,2)>width){lines.push_back(line);line=word;}else line=next;}
 if(!line.empty())lines.push_back(line);return lines;
}
inline std::string compatibility(const Game& game){return game.compatibility=="good"?"MAME: Working":game.compatibility=="imperfect"?"MAME: Imperfect":game.compatibility=="preliminary"?"MAME: Not working":"MAME: Not rated";}
inline std::string players(const Game& game){return game.players>1?"1-"+std::to_string(game.players):game.players?"1":"Unknown";}
inline std::string save_support(const Game& game){return game.save_support=="supported"?"Supported":game.save_support=="unsupported"?"Unsupported":"Unknown";}
inline float fit_text(Renderer& r,const std::string& text,float width,float preferred,int weight=1){while(preferred>14&&r.measure(text,preferred,weight)>width)preferred-=1;return preferred;}
inline void globe_icon(Renderer& r,float cx,float cy,Color ink,Color paper){
 r.circle(cx,cy,15,ink);r.circle(cx,cy,13,paper);constexpr float pi=3.14159265f;
 for(int i=1;i<=32;++i){float a=(i-1)*2*pi/32,b=i*2*pi/32;r.line(cx+6*std::cos(a),cy+13*std::sin(a),cx+6*std::cos(b),cy+13*std::sin(b),1.5f,ink);}
 r.line(cx-13,cy,cx+13,cy,1.5f,ink);r.line(cx-11,cy-7,cx+11,cy-7,1.3f,ink);r.line(cx-11,cy+7,cx+11,cy+7,1.3f,ink);
}
inline void players_icon(Renderer& r,float cx,float cy,Color ink,Color paper){
 for(float dx:{-11.f,11.f}){r.circle(cx+dx,cy-7,4,ink);r.rectangle({cx+dx-6,cy,12,11},ink,5);r.rectangle({cx+dx-4,cy+2,8,9},paper,3);}
 r.rectangle({cx-9,cy-1,18,16},paper,6);r.circle(cx,cy-11,5,ink);r.rectangle({cx-7,cy-2,14,16},ink,6);r.rectangle({cx-5,cy,10,12},paper,4);
}
inline void save_icon(Renderer& r,float cx,float cy,Color ink,Color paper,Color state,const std::string& support){
 r.rectangle({cx-13,cy-15,26,29},ink,3);r.rectangle({cx-11,cy-13,22,25},paper,2);
 r.rectangle({cx-7,cy-13,14,9},ink,1);r.rectangle({cx+2,cy-11,3,5},paper);
 r.rectangle({cx-7,cy+2,14,10},ink,1);r.rectangle({cx-5,cy+4,10,6},paper);
 r.circle(cx+12,cy+10,10,paper);r.circle(cx+12,cy+10,8,state);
 if(support=="supported"){r.line(cx+8,cy+10,cx+11,cy+13,1.8f,paper);r.line(cx+11,cy+13,cx+17,cy+7,1.8f,paper);}
 else if(support=="unsupported"){r.line(cx+9,cy+7,cx+15,cy+13,1.8f,paper);r.line(cx+15,cy+7,cx+9,cy+13,1.8f,paper);}
 else r.text(cx+8,cy+4,"?",13,paper,1);
}
}
