// Approved Cover Vault experience, implemented in the native PS5 renderer.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/gallery.hpp"
#include "ui/vault_footer.hpp"
#include "gfx/vault_layout.hpp"
#include "gfx/vault_vectors.hpp"
#include "control_mapping.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace arcade::native {
namespace {
using Clock=std::chrono::steady_clock;
Color rgb(unsigned v,float a=1){return {float((v>>16)&255)/255,float((v>>8)&255)/255,float(v&255)/255,a};}
Color opacity(Color c,float a){c.a*=a;return c;}
struct Palette{Color bg,ink,dim,line,accent,panel;};
Palette palette(const Settings& s){bool day=s.theme==Theme::Day;return {rgb(day?0xf5f2eb:0x10151c),rgb(day?0x181d24:0xf9f4e9),rgb(day?0x62666c:0xb6b9c3),rgb(day?0xd8d4cd:0x343b45),rgb(0xf46532),rgb(day?0xfffcf6:0x1a2029)};}
constexpr float scene_padding=36,cabinet_height=776-2*scene_padding;
constexpr Rect cabinet{40,80+scene_padding,cabinet_height*858/1350,cabinet_height};
constexpr Rect game_card{1118,cabinet.y,506,cabinet.h};
constexpr Rect play_button=card::play_button,settings_button=card::settings_button;
Rect part(Rect r){float k=cabinet.w/286;return {cabinet.x+r.x*k,cabinet.y+r.y*k,r.w*k,r.h*k};}
bool contains(Rect r,float x,float y){return x>=r.x&&x<=r.x+r.w&&y>=r.y&&y<=r.y+r.h;}
float smooth(float p){p=std::clamp(p,0.f,1.f);return p*p*(3-2*p);}
void wrapped(Renderer& r,float x,float y,const std::string& text,float size,Color color,float width,int max_lines=3,int weight=0){
 std::istringstream input(text);std::string word,line;int row=0;
 while(input>>word){auto next=line.empty()?word:line+" "+word;if(!line.empty()&&r.measure(next,size,weight)>width){r.text(x,y+row*size*1.26f,line,size,color,weight,width);if(++row>=max_lines-1){std::string rest=word,extra;while(input>>extra)rest+=" "+extra;r.text(x,y+row*size*1.26f,rest,size,color,weight,width);return;}line=word;}else line=next;}
 if(!line.empty())r.text(x,y+row*size*1.26f,line,size,color,weight,width);
}

void hint(Renderer& r,const std::string& glyph,float x,float y,const std::string& label,Color ink,Color){r.keycap(glyph,x,y);r.text(x+39,y+7,label,18,ink);}

// Direct geometry preserves the machine/A mark without a sampled image.
void logo(Renderer& r,Rect dst){
 Color ink=rgb(0xf46532),paper=rgb(0x10151c);float k=dst.w/64;
 auto line=[&](float x,float y,float xx,float yy,float w){r.line(dst.x+x*k,dst.y+y*k,dst.x+xx*k,dst.y+yy*k,w*k,ink);};
 auto box=[&](float x,float y,float w,float h,Color c,float radius=0){r.rectangle({dst.x+x*k,dst.y+y*k,w*k,h*k},c,radius*k);};
 box(15,5,34,54,paper,3);
 const float outline[][2]={{18,5},{46,5},{48.1f,5.9f},{49,8},{49,33},{54,41},{50,47},{50,57},{49.4f,58.4f},{48,59},{16,59},{14.6f,58.4f},{14,57},{14,47},{10,41},{15,33},{15,8},{15.9f,5.9f},{18,5}};
 for(size_t i=1;i<sizeof(outline)/sizeof(outline[0]);++i)line(outline[i-1][0],outline[i-1][1],outline[i][0],outline[i][1],3);
 line(25,10,39,10,2);box(19.25f,14.25f,25.5f,21.5f,ink,3.75f);box(20.75f,15.75f,22.5f,18.5f,paper,2.25f);
 // Fill the exact A outline and even-odd holes from arcade-mark.svg.
 using Point=std::pair<float,float>;std::vector<std::vector<Point>> rings={{{21,7},{43,7},{59,57},{45,57},{40.7f,44},{23.3f,44},{19,57},{5,57}},{{28.4f,18},{24.8f,29},{39.2f,29},{35.6f,18}}};
 for(float cx:{27.f,37.f}){std::vector<Point> circle;for(int i=0;i<16;++i){float a=i*6.2831853f/16;circle.emplace_back(cx+2.2f*std::cos(a),36.5f+2.2f*std::sin(a));}rings.push_back(circle);}
 float step=1/(k*.32f);for(float y=7;y<57;y+=step){std::vector<float> hits;float sample=std::min(56.99f,y+step/2);for(const auto& ring:rings)for(size_t i=0,j=ring.size()-1;i<ring.size();j=i++){auto a=ring[j],b=ring[i];if((a.second<=sample&&b.second>sample)||(b.second<=sample&&a.second>sample))hits.push_back(a.first+(sample-a.second)*(b.first-a.first)/(b.second-a.second));}std::sort(hits.begin(),hits.end());for(size_t i=1;i<hits.size();i+=2)box(21.76f+hits[i-1]*.32f,14.76f+y*.32f,(hits[i]-hits[i-1])*.32f,std::min(step,57-y)*.32f,ink);}
 line(14,46,50,46,2);line(24,41,24,44,2);r.circle(dst.x+24*k,dst.y+40*k,2.5f*k,ink);for(float x:{37.f,43.f})for(float y:{40.f,44.f})r.circle(dst.x+x*k,dst.y+y*k,1.5f*k,ink);box(29,50,6,4,ink,1);
}
void apply_game(Settings& out,const Settings& s){out.renderer=s.renderer;out.smooth=s.smooth;out.scaling=s.scaling;out.rotation=s.rotation;out.post_processing=s.post_processing;out.speed=s.speed;out.volume=s.volume;out.mute=s.mute;out.overlay=s.overlay;out.coin_button=s.coin_button;out.start_button=s.start_button;out.action_buttons=s.action_buttons;out.vibration=s.vibration;out.vibration_strength=s.vibration_strength;out.reduce_motion=s.reduce_motion;out.menu_volume=s.menu_volume;}
bool safe_id(const std::string& id){return !id.empty()&&id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")==std::string::npos;}
}

void Gallery::vault_init(){
 // Only the first Cover Vault run adopts the approved Night appearance.
 auto marker=data_/"cover-vault-v1";
 if(!std::filesystem::exists(marker)){model_.settings.theme=Theme::Night;try{write_settings(data_/"settings.cfg",model_.settings);std::ofstream out(marker);out<<"Cover Vault native UI 1\n";}catch(const std::exception&){}}
 sync_preferences();
}
void Gallery::sync_preferences(){
 effective_=model_.settings;auto* game=model_.current();if(!game)return;
 if(loaded_game_!=game->id){loaded_game_=game->id;auto file=data_/"game-settings"/(game->id+".cfg");if(safe_id(game->id)&&!overrides_.count(game->id)&&std::filesystem::exists(file))overrides_[game->id]=read_settings(file);}
 auto found=overrides_.find(game->id);if(found!=overrides_.end())apply_game(effective_,found->second);
}
void Gallery::notify(const std::string& text){toast_=text;toast_until_=Clock::now()+std::chrono::milliseconds(2300);}
void Gallery::return_to_library(){card_save_probe_={};return_at_=Clock::now();boundary_latched_=false;sync_preferences();}
void Gallery::controller(std::uint16_t pad){held_=pad;bool left=pad&(1u<<RETRO_DEVICE_ID_JOYPAD_LEFT),right=pad&(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT);stick_direction_=left?-1:right?1:0;if(!(pad&((1u<<RETRO_DEVICE_ID_JOYPAD_LEFT)|(1u<<RETRO_DEVICE_ID_JOYPAD_RIGHT)|(1u<<RETRO_DEVICE_ID_JOYPAD_UP)|(1u<<RETRO_DEVICE_ID_JOYPAD_DOWN)|(1u<<RETRO_DEVICE_ID_JOYPAD_L)|(1u<<RETRO_DEVICE_ID_JOYPAD_R))))boundary_latched_=false;}
void Gallery::boundary(int direction){if(!boundary_latched_||boundary_direction_!=direction){feedback_=3;boundary_latched_=true;boundary_direction_=direction;}}
void Gallery::vault_select(size_t index){
 auto visible=model_.visible();if(index>=visible.size()||index==model_.selected)return;
 if(auto* g=model_.current())previous_game_=g->id;
 model_.selected=index;model_.rail=false;screen_mix_=model_.settings.reduce_motion?1:0;
 sync_preferences();feedback_=1;
 boundary_latched_=false;
}
void Gallery::vault_move(int delta){
 auto visible=model_.visible();if(visible.empty())return;
 int next=std::clamp(int(model_.selected)+delta,0,int(visible.size())-1);
 if(next==int(model_.selected)){boundary(delta<0?-1:1);notify(next==0?"Beginning of collection":"End of collection");return;}
 vault_select(size_t(next));
}
bool Gallery::vault_input(Key key){
 if(model_.page==Page::Settings||browsing_||!message_.empty())return false;
 if(loading_)return true;
 bool confirm=model_.settings.swap_menu?key==Key::Circle:key==Key::Cross;
 bool back=model_.settings.swap_menu?key==Key::Cross:key==Key::Circle;
 if(sheet_open_){
  if(back||key==Key::Square){sheet_open_=false;feedback_=1;return true;}
  if(key==Key::Options||key==Key::Touchpad){sheet_open_=false;model_.change_page(Page::Settings);return true;}
  if(key==Key::L1||key==Key::R1){sheet_game_=key==Key::L1;sheet_row_=0;feedback_=1;return true;}
  if(key==Key::L2||key==Key::R2){sheet_tab_=(sheet_tab_+(key==Key::L2?2:1))%3;sheet_row_=0;feedback_=1;return true;}
  int count=sheet_tab_==0?4:sheet_tab_==1?6:4;
  if(key==Key::Up||key==Key::Down){sheet_row_=std::clamp(sheet_row_+(key==Key::Up?-1:1),0,count-1);feedback_=1;return true;}
  if(key==Key::Left||key==Key::Right||confirm){sheet_change(key==Key::Left?-1:1);return true;}
  return true;
 }
 if(key==Key::Left){stick_direction_=-1;vault_move(-1);return true;}
 if(key==Key::Right){stick_direction_=1;vault_move(1);return true;}
 if(key==Key::Up||key==Key::Down||key==Key::L1||key==Key::R1)return true;
 if(key==Key::Square){sheet_open_=true;sheet_tab_=sheet_row_=0;sheet_game_=true;feedback_=1;return true;}
 if(key==Key::L3){if(model_.current()){credits_=std::min(99u,credits_+1);coin_at_=Clock::now();feedback_=2;notify("Coin accepted - "+std::to_string(credits_)+" credit"+(credits_==1?"":"s"));}return true;}
 if(key==Key::Options||key==Key::R3){play(true);return true;}
 if(key==Key::Touchpad){model_.change_page(Page::Settings);return true;}
 if(key==Key::Triangle){favorite();feedback_=2;notify("Favorite updated");return true;}
 if(confirm){play();return true;}
 if(back){model_.change_page(model_.page==Page::Favorites?Page::Library:Page::Favorites);poses_.clear();sync_preferences();feedback_=1;return true;}
 return true;
}
void Gallery::sheet_save(const Settings& value){
 if(sheet_game_){auto* game=model_.current();if(!game||!safe_id(game->id))return;try{write_settings(data_/"game-settings"/(game->id+".cfg"),value);overrides_[game->id]=value;sync_preferences();notify("Saved for "+game->title);feedback_=2;}catch(const std::exception& e){error(e.what(),"ARC-3001");}}
 else{auto before=model_.settings;model_.settings=value;if(!save())model_.settings=before;else{notify("Saved for all games");feedback_=2;}}
}
void Gallery::sheet_change(int delta){
 Settings value=sheet_game_?effective_:model_.settings;
 if(sheet_tab_==0){if(sheet_row_==0){
#ifndef ARCADE_HAS_VULKAN
 notify("Vulkan is available in the PS5 build. This Mac preview uses OpenGL.");return;
#else
 adjust_setting(SettingId::Renderer,value,delta);
#endif
 }else if(sheet_row_==1){int filter=value.post_processing?std::min(3,value.post_processing+1):value.smooth?1:0;filter=(filter+delta+4)%4;value.smooth=filter==1;value.post_processing=filter>=2?filter-1:0;}else if(sheet_row_==2)value.scaling=(value.scaling+delta+3)%3;else value.volume=std::clamp(value.volume+delta*10,0,100);}
 else if(sheet_tab_==1){if(sheet_row_==0)value.coin_button=(value.coin_button+delta+4)%4;else if(sheet_row_==1)value.start_button=(value.start_button+delta+3)%3;else value.action_buttons[sheet_row_-2]=(value.action_buttons[sheet_row_-2]+delta+8)%8;}
 else{if(sheet_row_==0)value.menu_volume=value.menu_volume?0:30;else if(sheet_row_==1)value.vibration=!value.vibration;else if(sheet_row_==2){feedback_=2;notify("Controller vibration preview");return;}else value.reduce_motion=!value.reduce_motion;}
 sheet_save(value);
}
void Gallery::vault_header(Renderer& r){auto c=palette(model_.settings);r.rectangle({0,0,1672,80},c.bg);r.rectangle({0,79,1672,1},c.line);logo(r,{48,15,48,48});r.text(108,27,"ARCADE",28,c.ink,2);
 std::string title=model_.page==Page::Favorites?"YOUR FAVORITES":"THE COLLECTION";r.text(836-r.measure(title,22,1)/2,20,title,22,c.ink,1);
 auto visible=model_.visible();std::ostringstream counter;counter<<std::setfill('0')<<std::setw(2)<<(visible.empty()?0:model_.selected+1)<<" / "<<std::setw(2)<<visible.size();auto count=counter.str();r.text(836-r.measure(count,14)/2,52,count,14,c.dim);
 r.circle(1495,40,4,c.accent);r.text(1512,31,"Player "+std::to_string(model_.settings.profile+1),18,c.ink,0,128);
}
void Gallery::vault_grid(Renderer& r){auto c=palette(model_.settings);for(const auto& line:vault::room_lines)r.line(line[0]*1.672f,80+line[1]*776/600,line[2]*1.672f,80+line[3]*776/600,1,opacity(c.dim,line[4]*(model_.settings.high_contrast?.85f:.55f)));}

void Gallery::vault_cabinet(Renderer& r){
 r.rectangle({cabinet.x+28,cabinet.y+cabinet.h-3,cabinet.w-56,20},{0,0,0,.25f},10);r.asset("vault-cabinet.png",cabinet);
 auto screen=part(vault::screen);auto* game=model_.current();
 if(game){r.rectangle(screen,rgb(0x070b10),9);r.cover(game->id,screen,8,.12f,false);
  if(screen_mix_<1&&!previous_game_.empty()){auto before=screen;before.x-=screen_mix_*13;r.cover(previous_game_,before,4,1-screen_mix_,true);}
  auto after=screen;after.x+=(1-screen_mix_)*13;r.cover(game->id,after,4,screen_mix_,true);
  for(float y=screen.y;y<screen.y+screen.h;y+=4)r.rectangle({screen.x,y,screen.w,1},{0,0,0,.1f});
 }
 auto label=part(vault::credit);auto credit=std::string(credits_<10?"0":"")+std::to_string(credits_)+" CREDITS";float credit_size=11*cabinet.w/286;r.text(label.x+label.w-r.measure(credit,credit_size),label.y+6,credit,credit_size,rgb(0xe8bd85),0,label.w+4);
 auto stem=part(vault::stick);float base_x=stem.x+stem.w/2,base_y=stem.y+stem.h;float angle=stick_*.44f,top_x=base_x+std::sin(angle)*stem.h,top_y=base_y-std::cos(angle)*stem.h;
 r.line(base_x,base_y,top_x,top_y,stem.w+1,rgb(0x7e8e9e));r.line(base_x,base_y,top_x,top_y,2,rgb(0xd8e2e9));
 float radius=11*cabinet.w/286;r.circle(top_x+1,top_y+2,radius+1,{0,0,0,.5f});r.circle(top_x,top_y,radius,rgb(0xf46532));r.circle(top_x-3,top_y-4,radius*.55f,rgb(0xff9b62));
 const unsigned bits[]={RETRO_DEVICE_ID_JOYPAD_B,RETRO_DEVICE_ID_JOYPAD_A,RETRO_DEVICE_ID_JOYPAD_Y,RETRO_DEVICE_ID_JOYPAD_X};
 for(int i=0;i<4;++i)if(held_&(1u<<bits[i])){auto b=part(vault::actions[i]);r.circle(b.x+b.w/2,b.y+b.h/2,b.w/2+3,{1,.7f,.4f,.25f});}
 float coin_time=std::chrono::duration<float>(Clock::now()-coin_at_).count();if(coin_time<.5f){auto coin=part(vault::coin);float t=coin_time/.5f;r.circle(coin.x+coin.w/2,coin.y-20+28*t,7*(1-t*.6f),rgb(0xd9b568,1-t));}
}
void Gallery::vault_sheet(Renderer& r){
 auto c=palette(model_.settings);float a=smooth(sheet_mix_);if(a<.01f)return;
 r.rectangle({0,80,1672,776},{0,0,0,.43f*a});float x=1110+(1-a)*90;
 r.rectangle({x,108,520,622},opacity(c.line,a),20);r.rectangle({x+1,109,518,620},opacity(c.panel,a),19);
 r.text(x+28,132,"GAME SETTINGS",15,opacity(c.dim,a),1);auto* game=model_.current();r.text(x+28,165,game?game->title:"All games",29,opacity(c.ink,a),1,448);
 for(int i=0;i<2;++i){float bx=x+28+i*218;bool active=sheet_game_==(i==0);r.button({bx,215,205,43},i==0?"This game":"All games",i==0?"L1":"R1",active,a,17);}
 const char* tabs[]={"Video","Controls","Feedback"};for(int i=0;i<3;++i){float tx=x+28+i*153;r.button({tx,271,140,37},tabs[i],"",sheet_tab_==i,a,18);}
 Settings value=sheet_game_?effective_:model_.settings;std::vector<std::pair<std::string,std::string>> rows;
 if(sheet_tab_==0){const char* filters[]={"Sharp","Smooth","Scanlines","CRT"};int filter=value.post_processing?std::min(3,value.post_processing+1):value.smooth?1:0;rows={{"Game renderer",setting_value(SettingId::Renderer,value)},{"Display filter",filters[filter]},{"Scaling",setting_value(SettingId::Scaling,value)},{"Game volume",std::to_string(value.volume)+"%"}};}
 else if(sheet_tab_==1){rows={{"Insert coin",setting_value(SettingId::CoinButton,value)},{"Start",setting_value(SettingId::StartButton,value)}};const char* labels[]={"Cross","Circle","Square","Triangle"};for(int i=0;i<4;++i)rows.emplace_back(labels[i],"Action "+std::to_string(value.action_buttons[i]+1));}
 else rows={{"Menu audio",value.menu_volume?"On":"Off"},{"Controller vibration",value.vibration?"On":"Off"},{"Test vibration","Press Cross"},{"Reduced motion",value.reduce_motion?"On":"Off"}};
 for(size_t i=0;i<rows.size();++i){float y=337+i*46;if(int(i)==sheet_row_)r.rectangle({x+18,y-8,483,44},opacity(c.bg,a),7);r.text(x+30,y+4,rows[i].first,18,opacity(c.ink,a),0,234);r.button({x+262,y-6,231,36},rows[i].second,"",int(i)==sheet_row_,a,16);r.rectangle({x+28,y+36,462,1},opacity(c.line,a));}
 r.keycap("L2",x+28,628,a);r.keycap("R2",x+66,628,a);r.text(x+108,635,"Tabs",17,opacity(c.dim,a));
 r.keycap("<>",x+246,628,a);r.text(x+290,635,"Change",17,opacity(c.dim,a));
 r.keycap(model_.settings.swap_menu?"X":"O",x+28,674,a);r.text(x+72,681,"Close",17,opacity(c.dim,a));
 r.keycap("OPT",x+246,674,a);r.text(x+290,681,"All settings",17,opacity(c.dim,a),0,200);
}
void Gallery::vault_draw(Renderer& r){
 auto now=Clock::now();float dt=std::clamp(std::chrono::duration<float>(now-vault_tick_).count(),.001f,.1f);vault_tick_=now;clock_+=dt;sync_preferences();auto c=palette(model_.settings);
 r.rectangle({0,0,1672,941},c.bg);if(model_.settings.wallpaper.empty())r.arcade_background(model_.settings.theme==Theme::Day,effective_.reduce_motion);else r.wallpaper((data_/"wallpapers"/model_.settings.wallpaper).string());vault_header(r);
 screen_mix_=std::min(1.f,screen_mix_+dt/.42f);float follow=effective_.reduce_motion?1.f:1-std::exp(-dt*13);stick_+=(stick_direction_-stick_)*(1-std::exp(-dt*22));
 sheet_mix_+=(float(sheet_open_)-sheet_mix_)*(effective_.reduce_motion?1.f:1-std::exp(-dt*18));
 auto visible=model_.visible();

 for(auto& item:poses_)item.second.ta=0;
 // Covers follow a depth rail into the fixed cabinet. The selected cover is
 // occluded by the cabinet as it leaves the rail; it appears only in its screen.
 const float positions[]={284,640,760,865,955,1025},heights[]={450,440,420,400,380,360},scales[]={1,1,.9f,.81f,.73f,.66f};
 auto target=[&](size_t index,int slot){const auto& game=model_.games[visible[index]];float x=positions[slot],y=heights[slot],scale=scales[slot];
  if(slot&&visible.size()==2){x=850;y=440;scale=1.25f;}else if(slot&&visible.size()==3){x=slot==1?710:1000;y=slot==1?440:410;scale=slot==1?1.f:.9f;}else if(slot&&visible.size()==4){x=640+(slot-1)*190;y=440-(slot-1)*22;scale=.85f-(slot-1)*.07f;}
  auto found=poses_.find(game.id);if(found==poses_.end()){CoverPose p;p.id=game.id;p.title=game.title;p.x=x;p.y=y;p.s=scale;p.a=poses_.empty()?1:0;found=poses_.emplace(game.id,p).first;}auto& p=found->second;p.tx=x;p.ty=y;p.ts=scale;p.ta=slot?1:0;
 };
 if(!visible.empty()){target(model_.selected,0);for(size_t offset=1;offset<std::min(size_t(6),visible.size());++offset)target((model_.selected+offset)%visible.size(),int(offset));}
 std::vector<CoverPose*> drawing;
 for(auto it=poses_.begin();it!=poses_.end();){auto& p=it->second;p.x+=(p.tx-p.x)*follow;p.y+=(p.ty-p.y)*follow;p.s+=(p.ts-p.s)*follow;p.a+=(p.ta-p.a)*follow;
  if(p.ta==0&&p.a<.012f&&poses_.size()>8){it=poses_.erase(it);continue;}if(p.a>.012f)drawing.push_back(&p);++it;
 }
 std::sort(drawing.begin(),drawing.end(),[](const auto* a,const auto* b){return a->x>b->x;});
 for(const auto* p:drawing){Rect dst{p->x-130*p->s,p->y-182*p->s,260*p->s,364*p->s};r.rectangle({dst.x+5,dst.y+9,dst.w,dst.h},{0,0,0,.3f*p->a},9);r.cover(p->id,dst,8,p->a);}

 vault_cabinet(r);auto* game=model_.current();
 r.rectangle(game_card,c.line,20);r.rectangle({game_card.x+1,game_card.y+1,game_card.w-2,game_card.h-2},c.panel,19);
 if(game){
  const float x=game_card.x,y=game_card.y;bool day=model_.settings.theme==Theme::Day;auto good=rgb(day?0x2f7150:0x9bcead),warning=rgb(day?0x826117:0xe8c27b),bad=rgb(day?0xa54343:0xe89b95);
  Color status=game->compatibility=="good"?good:game->compatibility=="imperfect"?warning:game->compatibility=="preliminary"?bad:c.dim;
  Rect artwork{x+24,y+24,136,196};r.rectangle(artwork,c.bg,10);
  if(screen_mix_<1&&!previous_game_.empty())r.cover(previous_game_,artwork,10,1-screen_mix_,true);r.cover(game->id,artwork,10,screen_mix_,true);
  r.text(x+184,y+31,"SELECTED GAME",13,c.dim,1);
  auto title=card::display_title(game->title);float title_size=25;auto rows=card::title_lines(r,title,title_size,298);
  while(title_size>18&&rows.size()>3){--title_size;rows=card::title_lines(r,title,title_size,298);}
  for(size_t i=0;i<std::min(size_t(3),rows.size());++i)r.text(x+184,y+62+i*title_size*1.26f,rows[i],title_size,c.ink,2,298);
  float maker_y=rows.size()>2?165:145,status_y=rows.size()>2?215:190;
  auto maker=game->maker+" / "+(game->year?std::to_string(game->year):"Unknown");
  float maker_size=card::fit_text(r,maker,298,19,0);r.text(x+184,y+maker_y,maker,maker_size,c.dim,0,298);
  r.circle(x+189,y+status_y+9,4,status);r.text(x+202,y+status_y,card::compatibility(*game),17,status,1,280);
  r.rectangle({x+24,y+244,458,1},c.line);
  r.text(x+24,y+273,"Hardware",19,c.dim);
  float hardware_size=card::fit_text(r,game->hardware,310,20);r.text(x+482-r.measure(game->hardware,hardware_size,1),y+273,game->hardware,hardware_size,c.ink,1,310);
  constexpr float width=(458-24.f)/3;const char* captions[]={"REGION","PLAYERS","SAVE STATE"};
  const std::string values[]={game->region,card::players(*game),card::save_support(*game)};
  for(int i=0;i<3;++i){float left=x+24+i*(width+12),top=y+331,center=left+width/2;
   r.rectangle({left,top,width,151},c.line,12);r.rectangle({left+1,top+1,width-2,149},c.bg,11);
   auto support=game->save_support=="supported"?good:game->save_support=="unsupported"?warning:c.dim;
   if(i==0)card::globe_icon(r,center,top+37,c.ink,c.bg);else if(i==1)card::players_icon(r,center,top+37,c.ink,c.bg);else card::save_icon(r,center,top+37,c.ink,c.bg,support,game->save_support);
   float size=card::fit_text(r,values[i],width-14,20);r.text(center-r.measure(values[i],size,1)/2,top+77,values[i],size,i==2?support:c.ink,1);
   r.text(center-r.measure(captions[i],12,1)/2,top+115,captions[i],12,c.dim,1);
  }
  const auto& saved=card_save();r.rectangle({x+24,y+510,458,1},c.line);
  r.circle(x+29,y+544,4,saved.available?good:c.dim);r.text(x+45,y+537,"LAST SAVE",13,c.dim,1);
  auto stamp=card::save_time(saved);float stamp_size=card::fit_text(r,stamp,290,20);r.text(x+482-r.measure(stamp,stamp_size,1),y+531,stamp,stamp_size,c.ink,1);
  r.text(x+24,y+570,saved.available?"Continue your saved game":"Start from the beginning",17,c.dim);
  r.button(play_button,saved.available?"Resume":"Play",model_.settings.swap_menu?"O":"X",false,1,card::button_text_size);
  r.button(settings_button,"Settings","[]",false,1,card::button_text_size);
 }else{r.text(1146,152,"YOUR COLLECTION",15,c.dim,1);wrapped(r,1146,212,"Your arcade starts here",34,c.ink,450,3,2);wrapped(r,1146,412,"Choose a game folder in All settings to start your collection.",21,c.dim,450,4);}

 r.rectangle({0,856,1672,85},c.bg);r.rectangle({0,856,1672,1},c.line);
 for(const auto& item:footer_layout(model_.settings.large_text,model_.settings.swap_menu))hint(r,item.glyph,item.bounds.x,item.bounds.y,item.label,c.ink,c.line);

 vault_sheet(r);
 if(!toast_.empty()&&now<toast_until_){float left=cabinet.x+cabinet.w+24,right=game_card.x-24,center=(left+right)/2,width=std::min(right-left,r.measure(toast_,19,1)+48),top=856-scene_padding-42;r.rectangle({center-width/2,top,width,42},c.ink,19);r.text(center-width/2+24,top+12,toast_,19,c.bg,1,width-48);}
 float since_return=std::chrono::duration<float>(now-return_at_).count();if(since_return<.35f&&!effective_.reduce_motion)r.rectangle({0,80,1672,776},{0,0,0,1-smooth(since_return/.35f)});
 if(loading_){float t=std::chrono::duration<float>(now-launch_at_).count();r.rectangle({0,80,1672,776},{0,0,0,smooth((t-.1f)/.5f)});}
}
void Gallery::draw(Renderer& r){
 if(model_.page==Page::Settings||browsing_){legacy_draw(r);return;}
 if(!choosing_start_&&!loading_&&!demo_&&Clock::now()>=next_scan_)refresh(true);
 r.cover_directory(CoverCache::directory(model_.settings.roms).string());r.large_text(model_.settings.large_text);vault_draw(r);
 if(choosing_start_)draw_start_choice(r);
 if(!message_.empty())draw_message(r,message_,message_code_);
}
void Gallery::vault_click(float x,float y,bool twice){
 if(loading_)return;
 if(sheet_open_){if(x<1100){sheet_open_=false;return;}if(y>=215&&y<=258){sheet_game_=x<1350;return;}if(y>=270&&y<=315){sheet_tab_=std::clamp(int((x-1138)/153),0,2);sheet_row_=0;return;}int row=int((y-329)/46);int count=sheet_tab_==0?4:sheet_tab_==1?6:4;if(y>=329&&row>=0&&row<count){sheet_row_=row;sheet_change(x<1400?-1:1);}return;}
 boundary_latched_=false;
 if(contains(part(vault::coin),x,y)){vault_input(Key::L3);return;}
 if(contains(part(vault::start),x,y)){vault_input(Key::Options);return;}
 if(contains(play_button,x,y)){play();return;}
 if(contains(settings_button,x,y)){vault_input(Key::Square);return;}
 for(const auto& item:footer_layout(model_.settings.large_text,model_.settings.swap_menu))if(contains(item.bounds,x,y)){if(item.action==Key::Cross)play();else if(item.action==Key::Circle)vault_input(model_.settings.swap_menu?Key::Cross:Key::Circle);else vault_input(item.action);return;}

 auto visible=model_.visible();std::vector<const CoverPose*> hit;for(const auto& item:poses_)if(item.second.ta>.5f)hit.push_back(&item.second);std::sort(hit.begin(),hit.end(),[](const auto* a,const auto* b){return a->x<b->x;});
 for(const auto* pose:hit){const auto& p=*pose;if(contains({p.x-130*p.s,p.y-182*p.s,260*p.s,364*p.s},x,y)){for(size_t i=0;i<visible.size();++i)if(model_.games[visible[i]].id==p.id){vault_select(i);if(twice)play();return;}}}

}
}
