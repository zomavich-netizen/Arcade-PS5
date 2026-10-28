#include "ui/gallery.hpp"
#include <algorithm>
#include "directory.hpp"
#include "core.hpp"
#include "ui/message_dialog.hpp"
#include <fstream>
#include <chrono>
#include <functional>
#ifdef ARCADE_PS5
extern "C" void arcade_startup_note(const char*);
#else
static void arcade_startup_note(const char*) {}
#endif
namespace arcade::native {
namespace {
Color rgb(unsigned v){return {float((v>>16)&255)/255,float((v>>8)&255)/255,float(v&255)/255,1};}
struct Colors {Color bg,ink,muted,line,accent,panel;};
Colors colors(const Settings& s){
 auto c=s.theme==Theme::Day?Colors{rgb(0xfaf7f0),rgb(0x0b1521),rgb(0x717a83),rgb(0xddd9d1),rgb(0xf35419),rgb(0xf0ece4)}:Colors{rgb(0x11161e),rgb(0xfaf7f0),rgb(0xaeb6c2),rgb(0x303946),rgb(0xff8150),rgb(0x1d2531)};
 c.accent=rgb(accent_color(s.accent));
 if(s.high_contrast){c.bg=s.theme==Theme::Day?rgb(0xffffff):rgb(0x05080c);c.panel=c.bg;c.muted=c.ink;c.line=c.ink;}return c;
}
constexpr Color white{1,1,1,1};
constexpr Rect slots[]={{345,136,243,307},{624,136,225,307},{885,136,229,307},{345,519,243,314},{624,519,225,314},{885,519,229,314}};
}
Gallery::Gallery(std::filesystem::path data,Settings settings,bool demo,std::filesystem::path catalogue):data_(std::move(data)),covers_(catalogue,download_cover),metadata_(read_game_metadata(catalogue.empty()?std::filesystem::path{}:catalogue.parent_path()/"mame-metadata.tsv")),demo_(demo){model_.settings=std::move(settings);model_.favorites=read_favorites(profile_data()/"favorites.txt");refresh();vault_init();}
const card::SaveInfo& Gallery::card_save(){
 auto* game=model_.current();if(!game){card_save_={};return card_save_;}
 auto file=Core::state_path(game->path,profile_data());auto now=std::chrono::steady_clock::now();
 if(file!=card_save_path_||now>=card_save_probe_){card_save_=card::inspect_save(file);card_save_path_=file;card_save_probe_=now+std::chrono::seconds(1);}
 return card_save_;
}
bool Gallery::save(){try{write_settings(data_/"settings.cfg",model_.settings);sync_preferences();return true;}catch(const std::exception& e){error(e.what(),"ARC-3001");return false;}}
void Gallery::refresh(bool quiet){next_scan_=std::chrono::steady_clock::now()+std::chrono::seconds(5);try{std::string selected;if(auto* game=model_.current())selected=game->id;if(demo_)model_.games=sample_library();else if(model_.settings.roms.empty())model_.games.clear();else model_.games=scan(model_.settings.roms,metadata_);model_.clamp();auto visible=model_.visible();for(size_t i=0;i<visible.size();++i)if(model_.games[visible[i]].id==selected)model_.selected=i;if(!demo_){if(!model_.settings.roms.empty()){std::error_code ec;std::filesystem::create_directories(CoverCache::directory(model_.settings.roms),ec);}covers_.discover(model_.games);}}catch(const std::exception& e){if(!quiet)error(e.what(),"ARC-3002");}}
void Gallery::toggle_theme(){auto before=model_.settings.theme;model_.settings.theme=before==Theme::Day?Theme::Night:Theme::Day;if(!save())model_.settings.theme=before;}
void Gallery::favorite(){model_.toggle_favorite();try{write_favorites(profile_data()/"favorites.txt",model_.favorites);}catch(const std::exception& e){error(e.what(),"ARC-3001");}}
void Gallery::play(bool start){
 auto* g=model_.current();if(!g||loading_||choosing_start_)return;
 if(demo_){notify("Preview library: choose your game folder in All settings to play.");return;}
 start_requested_=start;load_requested_=false;launch_=*g;
 std::error_code ec;
 if(std::filesystem::is_regular_file(Core::state_path(g->path,profile_data()),ec)){choosing_start_=true;start_choice_=0;feedback_=2;return;}
 begin_launch(false);
}
void Gallery::begin_launch(bool resume){
 if(!resume&&!credits_){if(!choosing_start_)start_requested_=false;feedback_=1;notify("Insert at least one coin to play - press L3");return;}
 choosing_start_=false;load_requested_=resume;if(resume)start_requested_=false;
 loading_=true;launch_at_=std::chrono::steady_clock::now();feedback_=2;
}
std::optional<Game> Gallery::take_launch(){if(!loading_||std::chrono::steady_clock::now()-launch_at_<std::chrono::milliseconds(600))return {};loading_=false;return launch_;}
void Gallery::draw_start_choice(Renderer& r){
 auto c=colors(model_.settings);r.large_text(model_.settings.large_text);
 auto layout=start_choice_layout();auto box=layout.box;float x=box.x,y=box.y;
 auto panel=model_.settings.theme==Theme::Day?rgb(0xfffcf6):rgb(0x1a2029);
 r.rectangle({0,0,1672,941},{0,0,0,.7f});r.rectangle({x,y+12,box.w,box.h},{0,0,0,.4f},24);
 r.rectangle(box,c.line,22);r.rectangle({x+1,y+1,box.w-2,box.h-2},panel,21);
 r.text(x+40,y+33,"START / SAVED PROGRESS",15,c.accent,1);r.text(x+40,y+76,"Start game",38,c.ink,1);
 r.text(x+40,y+134,"Load your last save or start from the beginning.",24,c.muted);
 r.rectangle(layout.game,c.bg,14);
 card::save_icon(r,x+79,layout.game.y+layout.game.h/2,c.accent,c.bg,c.accent,"supported");
 float title_height=layout.title_size*(model_.settings.large_text?1.16f:1)*1.26f;
 for(size_t i=0;i<layout.game_lines.size();++i)r.text(x+118,layout.game.y+17+i*title_height,layout.game_lines[i],layout.title_size,c.ink,1);
 std::string context="Player "+std::to_string(model_.settings.profile+1)+" / Last save: "+card::save_time(card_save());
 r.text(x+118,layout.game.y+layout.game.h-32,context,18,c.muted);
 r.rectangle({x+40,y+layout.divider,1040,1},c.line);
 const char* labels[]={"Load save","New game","Back"};
 const char* confirm=model_.settings.swap_menu?"O":"X";const char* back=model_.settings.swap_menu?"X":"O";
 // Confirm activates the focused option; Square always starts a fresh game.
 const char* glyphs[]={confirm,"[]",back};
 std::string notes[]={"Continue saved progress",std::to_string(credits_)+" credits / L3: insert coin","Return to main hall"};
 for(size_t i=0;i<3;++i){auto button=layout.buttons[i];r.button(button,labels[i],glyphs[i],i==size_t(start_choice_),1,26);
  r.text(button.x+(button.w-r.measure(notes[i],16))/2,y+layout.divider+114,notes[i],16,c.muted);}
}
void Gallery::list_folder(){
 folders_.clear();folder_index_=0;
 arcade_startup_note("listing game folder");arcade_startup_note(folder_.c_str());
 try{for(const auto& item:directory_entries(folder_)){if(item.directory&&!item.path.filename().string().empty()&&item.path.filename().string()[0]!='.')folders_.push_back(item.path);}std::sort(folders_.begin(),folders_.end());}catch(const std::exception&){error("This folder is unavailable. Choose another folder.","ARC-3002");}
}
void Gallery::accept_folder(){if(!directory_available(folder_)){error("This folder is unavailable.","ARC-3002");return;}auto before=model_.settings.roms;model_.settings.roms=folder_;if(!save()){model_.settings.roms=before;return;}demo_=false;browsing_=false;refresh();}
void Gallery::message_scroll(Key key){
 if(!message_.empty())message_pages_=message_layout(message_,model_.settings.large_text).pages;
 if((key==Key::L1||key==Key::Left||key==Key::Up)&&message_page_)--message_page_;
 if((key==Key::R1||key==Key::Right||key==Key::Down)&&message_page_+1<message_pages_)++message_page_;
}
void Gallery::draw_message(Renderer& r,const std::string& message,const std::string& code,bool save_actions){
 auto c=colors(model_.settings);auto layout=message_layout(message,model_.settings.large_text);
 message_pages_=layout.pages;message_page_=std::min(message_page_,layout.pages-1);
 r.rectangle({0,0,1672,941},{0,0,0,.7f});
 auto box=layout.box;r.rectangle({box.x,box.y+12,box.w,box.h},{0,0,0,.35f},24);
 r.rectangle(box,c.line,22);r.rectangle({box.x+1,box.y+1,box.w-2,box.h-2},c.bg,21);
 r.rectangle({box.x+48,box.y+32,4,70},c.accent,2);
 r.text(box.x+68,box.y+32,code.empty()?"ARCADE":"ERROR CODE  /  "+code,20,c.accent,1);
 std::string title=code.empty()?"A quick note":code=="ARC-1001"?"Not enough memory":code=="ARC-1002"?"Game archive unavailable":code=="ARC-1003"?"Arcade engine unavailable":code=="ARC-1004"?"Unable to start this game":code=="ARC-2001"?"Could not save your game":code=="ARC-2002"?"Could not load your save":"Unable to continue";
 r.text(box.x+68,box.y+72,title,38,c.ink,1);
 r.rectangle({box.x+48,box.y+125,1016,1},c.line);
 auto first=message_page_*layout.rows;
 for(size_t i=first;i<std::min(first+layout.rows,layout.lines.size());++i)r.text(layout.body.x,layout.body.y+(i-first)*layout.line_height,layout.lines[i],layout.text_size,c.ink);
 float label_y=box.y+box.h-137;
 if(layout.pages>1){auto page="Page "+std::to_string(message_page_+1)+" of "+std::to_string(layout.pages)+"  /  L1 and R1 to read more";r.text((1672-r.measure(page,19))/2,label_y,page,19,c.muted);}
 auto confirm=model_.settings.swap_menu?"O":"X",back=model_.settings.swap_menu?"X":"O";
 if(save_actions){
  float x=(1672-976.f)/2,y=box.y+box.h-100;
  r.button({x,y,312,64},"Resume",confirm,false,1,22);
  r.button({x+332,y,312,64},"Retry save",back,false,1,22);
  r.button({x+664,y,312,64},"Exit without saving","[]",false,1,20);
 }else{
  if(layout.pages>1){r.button(layout.previous,"Previous","L1",false,message_page_?1:.35f,21);r.button(layout.next,"Next","R1",false,message_page_+1<layout.pages?1:.35f,21);}
  r.button(layout.close,"Close",confirm,true,1,24);
 }
}
void Gallery::input(Key key){
 if(!message_.empty()){
  message_scroll(key);
  bool confirm=model_.settings.swap_menu?key==Key::Circle:key==Key::Cross;
  bool back=model_.settings.swap_menu?key==Key::Cross:key==Key::Circle;
  if(confirm||back)dismiss_message();return;
 }
 if(choosing_start_){
  bool confirm=model_.settings.swap_menu?key==Key::Circle:key==Key::Cross;
  bool back=model_.settings.swap_menu?key==Key::Cross:key==Key::Circle;
  if(back){choosing_start_=false;launch_.reset();start_requested_=false;return;}
  if(key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right){start_choice_=(key==Key::Up||key==Key::Left)?0:1;feedback_=1;}
  if(key==Key::Square){start_choice_=1;begin_launch(false);return;}
  if(key==Key::L3){credits_=std::min(99u,credits_+1);feedback_=2;}
  if(confirm)begin_launch(start_choice_==0);
  return;
 }
 if(vault_input(key))return;
 if(model_.settings.swap_menu){if(key==Key::Cross)key=Key::Circle;else if(key==Key::Circle)key=Key::Cross;}
 feedback_=key==Key::Cross?2:1;

 if(browsing_){
  switch(key){
   case Key::Up:if(folder_index_)--folder_index_;break;
   case Key::Down:if(folder_index_+1<folders_.size())++folder_index_;break;
   case Key::Cross:if(folder_index_<folders_.size()){folder_=folders_[folder_index_];list_folder();}break;
   case Key::Triangle:accept_folder();break;
   case Key::Circle:if(folder_!=folder_.root_path()){folder_=folder_.parent_path();list_folder();}else browsing_=false;break;
   case Key::Options:browsing_=false;break;
   case Key::L1:folder_index_=folder_index_>7?folder_index_-8:0;break;
   case Key::R1:if(!folders_.empty())folder_index_=std::min(folder_index_+8,folders_.size()-1);break;
   default:break;
  }return;
 }
 if(model_.page==Page::Settings){model_.rail=false;if(settings_input(key))return;}
 switch(key){
 case Key::L1:model_.change_page(static_cast<Page>(std::max(0,int(model_.page)-1)));return;
 case Key::R1:model_.change_page(static_cast<Page>(std::min(2,int(model_.page)+1)));return;
 case Key::Options:model_.change_page(Page::Settings);return;
 case Key::Circle:if(model_.page!=Page::Library)model_.change_page(Page::Library);else model_.rail=!model_.rail;return;
 case Key::Left:model_.move(-1,0);return;case Key::Right:model_.move(1,0);return;
 case Key::Up:model_.move(0,-1);return;case Key::Down:model_.move(0,1);return;
 case Key::Triangle:if(model_.page!=Page::Settings)favorite();return;
 case Key::Cross:if(model_.rail){model_.rail=false;return;}play();return;
 default:return;
 }
}
void Gallery::click(float x,float y,bool twice){
 if(!message_.empty()){
  auto layout=message_layout(message_,model_.settings.large_text);
  if(message_contains(layout.close,x,y))dismiss_message();
  else if(layout.pages>1&&message_contains(layout.previous,x,y))message_scroll(Key::L1);
  else if(layout.pages>1&&message_contains(layout.next,x,y))message_scroll(Key::R1);
  return;
 }
 if(choosing_start_){auto layout=start_choice_layout();
  if(message_contains(layout.buttons[0],x,y)){start_choice_=0;begin_launch(true);}
  else if(message_contains(layout.buttons[1],x,y)){start_choice_=1;begin_launch(false);}
  else if(message_contains(layout.buttons[2],x,y))input(model_.settings.swap_menu?Key::Cross:Key::Circle);
  return;
 }

 if(model_.page!=Page::Settings&&!browsing_&&message_.empty()){vault_click(x,y,twice);return;}

 if(browsing_){if(x>=1190&&x<=1590&&y>=790&&y<=860){accept_folder();return;}size_t start=folder_index_/8*8;for(size_t i=start;i<std::min(start+8,folders_.size());++i)if(x>=344&&x<=1632&&y>=218+(i-start)*63&&y<=272+(i-start)*63){folder_index_=i;if(twice){folder_=folders_[i];list_folder();}return;}return;}
 if(model_.page!=Page::Settings&&x<302){for(int i=0;i<3;++i)if(y>=201+i*77&&y<252+i*77){model_.change_page(static_cast<Page>(i));return;}}
 if(model_.page==Page::Settings){
  if(settings_panel_==0){for(int i=0;i<settings_category_count;++i){float tx=48+(i%3)*533,ty=179+(i/3)*151;if(x>=tx&&x<=tx+509&&y>=ty&&y<=ty+131){model_.setting=i;model_.rail=false;settings_panel_=1;settings_row_=0;feedback_=2;return;}}}
  else{auto rows=settings_rows();int first=settings_row_/5*5;for(int i=first;i<std::min(first+5,int(rows.size()));++i){float ty=218+(i-first)*112;if(x>=48&&x<=1624&&y>=ty&&y<=ty+100){settings_row_=i;settings_activate(x<836?-1:1);feedback_=2;return;}}}return;
 }
 auto visible=model_.visible();size_t first=model_.selected/6*6;
 for(size_t i=first;i<std::min(first+6,visible.size());++i){auto r=slots[i-first];if(x>=r.x&&x<=r.x+r.w&&y>=r.y&&y<=r.y+r.h+56){model_.selected=i;model_.rail=false;if(twice)play();return;}}
 if(x>=1189&&x<=1632&&y>=744&&y<=815)play();
}
void Gallery::library(Renderer& r){auto c=colors(model_.settings);auto visible=model_.visible();r.text(342,49,model_.page==Page::Favorites?"Your favorites":"Your arcade",68,c.ink,2);
 if(visible.empty()){r.text(344,345,model_.page==Page::Favorites?"Your favorites live here.":"Your arcade starts here.",36,c.ink,1);r.text(344,405,model_.page==Page::Favorites?"Press Triangle on a game to keep it close.":"Choose a game folder in Settings to get started.",23,c.muted,0,770);return;}
 size_t first=model_.selected/6*6;
 for(size_t i=first;i<std::min(first+6,visible.size());++i){auto& g=model_.games[visible[i]];auto dst=slots[i-first];if(i==model_.selected&&!model_.rail){r.rectangle({dst.x-8,dst.y-8,dst.w+16,dst.h+16},c.accent,10);r.rectangle({dst.x-5,dst.y-5,dst.w+10,dst.h+10},c.bg,7);}r.cover(g.id,dst,5);r.text(dst.x,dst.y+dst.h+10,g.title,21,c.ink,1,dst.w);r.text(dst.x,dst.y+dst.h+38,g.year?std::to_string(g.year):"Unknown",20,c.muted);}
 auto* g=model_.current();if(!g)return;r.cover(g->id,{1189,50,443,532},10);r.text(1189,610,g->title,48,c.ink,1,443);r.text(1190,668,g->maker,25,c.muted,0,443);r.text(1190,700,g->year?std::to_string(g->year):"Unknown",24,c.muted);
 r.play({1189,744,443,70},c.accent,white);r.rectangle({1189,842,443,1},c.line);
 r.controller_hint(model_.settings.swap_menu?3:0,1192,864,"Play",c.ink,c.bg);r.controller_hint(1,1330,864,"Favorite",c.ink,c.bg);r.controller_hint(2,1505,864,"Settings",c.ink,c.bg);
}
std::vector<SettingRow> Gallery::settings_rows()const{
 if(settings_panel_==2)return overlay_rows();if(settings_panel_==3)return mapping_rows();
 if(settings_panel_==4){std::vector<SettingRow> rows{{SettingId::Wallpaper,"None","Use the Gallery background."}};for(size_t i=0;i<wallpapers_.size();++i)rows.push_back({SettingId::Wallpaper,"Wallpaper","PNG image in your wallpapers folder."});return rows;}
 return category_rows(static_cast<SettingsCategory>(model_.setting));
}
bool Gallery::settings_input(Key key){
 if(key==Key::Options){settings_panel_=0;return true;}
 if(key==Key::Circle){if(settings_panel_>1){settings_panel_=1;settings_row_=0;}else if(settings_panel_==1)settings_panel_=0;else model_.change_page(Page::Library);return true;}
 if(settings_panel_==0){int& at=model_.setting;
  if(key==Key::Up){if(at>=3)at-=3;return true;}if(key==Key::Down){if(at+3<settings_category_count)at+=3;return true;}
  if(key==Key::Left){if(at%3>0)--at;return true;}if(key==Key::Right){if(at%3<2&&at+1<settings_category_count)++at;return true;}
  if(key==Key::Cross){settings_panel_=1;settings_row_=0;settings_focus_=0;return true;}return false;
 }
 auto rows=settings_rows();
 if(key==Key::Up||key==Key::Down||key==Key::L1||key==Key::R1){int d=key==Key::Up?-1:key==Key::Down?1:key==Key::L1?-5:5;settings_row_=std::clamp(settings_row_+d,0,std::max(0,int(rows.size())-1));return true;}
 if(key==Key::Left||key==Key::Right||key==Key::Cross){settings_activate(key==Key::Left?-1:1);return true;}return true;
}
void Gallery::settings_activate(int direction){
 auto rows=settings_rows();if(settings_row_<0||settings_row_>=int(rows.size()))return;
 const auto id=rows[settings_row_].id;using I=SettingId;
#ifndef ARCADE_HAS_VULKAN
 if(id==I::Renderer){message_="The Mac preview uses OpenGL. The PS5 build includes the advanced Vulkan renderer.";return;}
#endif
 if(settings_panel_==4){auto before=model_.settings.wallpaper;model_.settings.wallpaper=settings_row_==0?"":wallpapers_[settings_row_-1];if(!save())model_.settings.wallpaper=before;return;}
 if(id==I::OverlayOptions){settings_panel_=2;settings_row_=0;return;}
 if(id==I::Mapping){settings_panel_=3;settings_row_=0;return;}
 if(id==I::Wallpaper){try{auto folder=data_/"wallpapers";std::filesystem::create_directories(folder);wallpapers_.clear();for(const auto& entry:directory_entries(folder))if(entry.regular&&(entry.path.extension()==".png"||entry.path.extension()==".PNG"))wallpapers_.push_back(entry.path.filename().string());std::sort(wallpapers_.begin(),wallpapers_.end());settings_panel_=4;settings_row_=0;}catch(const std::exception& e){error(e.what(),"ARC-3001");}return;}
 if(id==I::GameFolder){folder_=directory_available(model_.settings.roms)?model_.settings.roms:std::filesystem::path(
#ifdef ARCADE_PS5
 "/data"
#else
 "/"
#endif
 );browsing_=true;arcade_startup_note("opening game folder picker");list_folder();return;}
 if(id==I::Rescan){refresh();if(message_.empty())message_="Library refreshed. Found "+std::to_string(model_.games.size())+" arcade games.";return;}
 if(id==I::BackupSaves){backup_saves();return;}
 if(id==I::SaveLocation){message_=(profile_data()/"saves").string();return;}
 if(id==I::LogLocation){message_="PS5 diagnostics: /data/arcade-startup.log";return;}
 if(id==I::Backend){message_="MAME 0.289, native PS5 build. Driver coverage is recorded in BUILD.json beside the installed executable.";return;}
 auto before=model_.settings;if(!adjust_setting(id,model_.settings,direction))return;
 if(!save()){model_.settings=before;return;}
 feedback_=2;
 if(id==I::Profile){model_.favorites=read_favorites(profile_data()/"favorites.txt");model_.clamp();}
}
void Gallery::backup_saves(){
 try{
  auto source=profile_data()/"saves";if(!directory_available(source)){message_="No game saves yet. Play a game before creating a backup.";return;}
  auto stamp=std::chrono::system_clock::now().time_since_epoch().count();auto final=profile_data()/"backups"/("saves-"+std::to_string(stamp));auto partial=final;partial+=".partial";
  unsigned files=0;std::uintmax_t bytes=0;
  std::function<void(const std::filesystem::path&,const std::filesystem::path&,int)> copy;
  copy=[&](const auto& from,const auto& to,int depth){if(depth>16)throw std::runtime_error("Save folder nesting exceeds the backup limit.");std::filesystem::create_directories(to);for(const auto& entry:directory_entries(from)){auto destination=to/entry.path.filename();if(entry.directory)copy(entry.path,destination,depth+1);else if(entry.regular){if(++files>4096)throw std::runtime_error("Save backup has too many files.");std::ifstream in(entry.path,std::ios::binary);std::ofstream out(destination,std::ios::binary);if(!in||!out)throw std::runtime_error("Cannot open a save file for backup.");char buffer[16384];while(in){in.read(buffer,sizeof buffer);auto n=in.gcount();bytes+=n;if(bytes>256u*1024u*1024u)throw std::runtime_error("Save backup exceeds 256 MiB.");out.write(buffer,n);}out.flush();if(!in.eof()||!out)throw std::runtime_error("A save file could not be backed up.");}}};
  copy(source,partial,0);std::filesystem::rename(partial,final);message_="Backed up "+std::to_string(files)+" save files to "+final.string();
 }catch(const std::exception& e){error(e.what(),"ARC-3001");}
}
void Gallery::settings(Renderer& r){
 auto c=colors(model_.settings);r.text(48,49,"Settings",68,c.ink,2);
 if(settings_panel_==0){r.text(50,132,"Fine-tune your arcade",23,c.muted);
  for(int i=0;i<settings_category_count;++i){float x=48+(i%3)*533,y=179+(i/3)*151;bool selected=i==model_.setting&&!model_.rail;auto category=static_cast<SettingsCategory>(i);if(selected)r.rectangle({x-2,y-2,513,135},c.accent,15);r.raised({x,y,509,131});r.text(x+22,y+25,category_name(category),29,white,1,465);r.text(x+22,y+78,category_note(category),20,{.75f,.79f,.85f,1},0,465);}
 }else{
  auto title=settings_panel_==2?"Video / Overlay options":settings_panel_==3?"Controls / Button mapping":settings_panel_==4?"Appearance / Wallpaper":category_name(static_cast<SettingsCategory>(model_.setting));
  r.text(50,143,title,30,c.ink,1);auto rows=settings_rows();settings_row_=std::clamp(settings_row_,0,std::max(0,int(rows.size())-1));int first=settings_row_/5*5;
  float target=218+(settings_row_-first)*112;settings_focus_=model_.settings.reduce_motion||settings_focus_==0?target:settings_focus_+(target-settings_focus_)*.28f;
  r.rectangle({46,settings_focus_-2,1580,104},c.accent,15);
  for(int i=first;i<std::min(first+5,int(rows.size()));++i){float y=218+(i-first)*112;r.rectangle({48,y,1576,100},c.panel,13);r.text(73,y+20,rows[i].label,27,c.ink,1,825);r.text(74,y+65,rows[i].note,19,c.muted,0,1524);std::string value=setting_value(rows[i].id,model_.settings);if(rows[i].id==SettingId::SaveLocation)value=(profile_data()/"saves").string();if(settings_panel_==4)value=i==0?"None":wallpapers_[i-1];float value_size=23;float value_width=std::clamp(r.measure(value,value_size,1)+36,120.f,660.f);r.button({1615-value_width,y+12,value_width,54},value,"",i==settings_row_,1,value_size);}
  if(rows.size()>5){r.rectangle({1644,218,4,548},c.line,2);float thumb=548.f*5/rows.size();r.rectangle({1644,218+(548-thumb)*settings_row_/std::max(1,int(rows.size())-1),4,thumb},c.accent,2);}
  r.text(1500,146,std::to_string(settings_row_+1)+" / "+std::to_string(rows.size()),21,c.muted,0,132);
 }
 r.controller_hint(model_.settings.swap_menu?3:0,48,854,settings_panel_?"Change / Open":"Open",c.ink,c.bg);
 r.controller_hint(model_.settings.swap_menu?0:3,314,854,"Back",c.ink,c.bg);
 if(settings_panel_){r.keycap("<>",910,854);r.text(954,861,"Change",20,c.muted);r.keycap("L1",1180,854);r.keycap("R1",1218,854);r.text(1262,861,"Page",20,c.muted);}
}

void Gallery::browse(Renderer& r){auto c=colors(model_.settings);r.text(344,49,"Choose game folder",52,c.ink,2,1280);r.text(345,135,folder_.string(),22,c.muted,0,1250);size_t first=folder_index_/8*8;
 if(folders_.empty())r.text(370,240,"No subfolders. You can use this folder.",24,c.muted);
 for(size_t i=first;i<std::min(first+8,folders_.size());++i){float y=218+(i-first)*63;r.button({344,y-10,1288,52},folders_[i].filename().string(),"",i==folder_index_,1,24);}
 r.play({1190,790,400,70},c.accent,white,"Use this folder",false);r.controller_hint(model_.settings.swap_menu?3:0,344,864,"Open",c.ink,c.bg);r.controller_hint(model_.settings.swap_menu?0:3,510,864,"Back",c.ink,c.bg);r.controller_hint(1,680,864,"Use folder",c.ink,c.bg);
}
void Gallery::legacy_draw(Renderer& r){if(!demo_&&model_.page!=Page::Settings&&std::chrono::steady_clock::now()>=next_scan_)refresh(true);r.cover_directory(CoverCache::directory(model_.settings.roms).string());r.large_text(model_.settings.large_text);auto c=colors(model_.settings);r.rectangle({0,0,1672,941},c.bg);if(model_.settings.wallpaper.empty())r.arcade_background(model_.settings.theme==Theme::Day,model_.settings.reduce_motion);else r.wallpaper((data_/"wallpapers"/model_.settings.wallpaper).string());if(model_.page==Page::Settings||browsing_){if(browsing_)browse(r);else settings(r);if(!message_.empty())draw_message(r,message_,message_code_);return;}r.rectangle({302,36,1,870},c.line);if(model_.page!=Page::Settings)r.rectangle({1155,28,1,880},c.line);r.text(46,93,"ARCADE",50,c.ink,2);r.rectangle({47,146,109,7},c.accent,2);
 const char* names[]={"Library","Favorites","Settings"};const Rect icons[]={{64,211,40,33},{67,286,35,34},{65,359,39,42}};
 for(int i=0;i<3;++i){bool active=int(model_.page)==i;float y=216+i*77;if(active){r.rectangle({34,y-12,4,49},c.accent,2);if(model_.rail)r.rectangle({48,y-12,229,49},c.panel,8);}r.sidebar_icon(i,icons[i],active?c.accent:c.ink);r.text(136,y,names[i],24,active?c.accent:c.ink,active?1:0);}
 if(browsing_)browse(r);else if(model_.page==Page::Settings)settings(r);else library(r);
 if(!message_.empty())draw_message(r,message_,message_code_);
}
void Gallery::loading(Renderer& r){vault_draw(r);r.rectangle({0,80,1672,776},{0,0,0,.78f});r.text(620,460,"Starting your game...",30,{1,1,1,1},1,600);}
void Gallery::paused(Renderer& r,const Frame& frame,const PauseMenu& menu){
 auto c=colors(model_.settings);r.large_text(model_.settings.large_text);r.game(frame,model_.settings);
 auto layout=paused_layout(menu);auto box=layout.box;float x=box.x,y=box.y;
 auto panel=model_.settings.theme==Theme::Day?rgb(0xfffcf6):rgb(0x1a2029);
 auto green=model_.settings.theme==Theme::Day?rgb(0x2f7150):rgb(0x9bcead),red=model_.settings.theme==Theme::Day?rgb(0xa54343):rgb(0xe89b95);
 r.rectangle({0,0,1672,941},{0,0,0,.7f});r.rectangle({x,y+12,box.w,box.h},{0,0,0,.4f},24);r.rectangle(box,c.line,22);r.rectangle({x+1,y+1,box.w-2,box.h-2},panel,21);
 r.text(x+40,y+33,"PAUSE / SAVE PROGRESS",15,c.accent,1);r.text(x+40,y+76,"Game paused",38,c.ink,1);
 r.text(x+40,y+134,"Save your progress or leave without saving.",24,c.muted);
 auto disk=[&](float dx,float dy){r.rectangle({dx,dy,30,33},c.accent,4);r.rectangle({dx+2,dy+2,26,29},c.bg,2);r.rectangle({dx+7,dy+2,16,11},c.accent,1);r.rectangle({dx+17,dy+4,4,7},c.bg);r.rectangle({dx+7,dy+21,16,10},c.accent,1);r.rectangle({dx+9,dy+23,12,6},c.bg);};
 r.rectangle(layout.game,c.bg,14);disk(x+64,layout.game.y+(layout.game.h-33)/2);
 float title_height=layout.title_size*(model_.settings.large_text?1.16f:1)*1.26f;
 for(size_t i=0;i<layout.game_lines.size();++i)r.text(x+118,layout.game.y+17+i*title_height,layout.game_lines[i],layout.title_size,c.ink,1);
 auto* game=model_.current();std::string context="Player "+std::to_string(model_.settings.profile+1);
 if(game&&game->hardware!="Unknown")context=game->hardware+" / "+context;
 r.text(x+118,layout.game.y+layout.game.h-32,context,18,c.muted);
 r.rectangle({x+40,y+layout.divider,1040,1},c.line);
 const char* labels[]={"Resume","Save","Exit"};const char* glyphs[]={model_.settings.swap_menu?"O":"X","[]",model_.settings.swap_menu?"X":"O"};const char* notes[]={"Return to game","Stay paused","Without saving"};
 for(size_t i=0;i<3;++i){auto button=layout.buttons[i];r.button(button,labels[i],glyphs[i],i==1,1,26);r.text(button.x+(button.w-r.measure(notes[i],16))/2,y+layout.divider+114,notes[i],16,i==2?red:c.muted);}
 auto toast=save_toast_layout(menu,model_.settings.large_text);
 if(toast.visible){auto fb=toast.box;auto color=menu.result()==SaveResult::Success?green:red;
  r.rectangle({fb.x,fb.y+7,fb.w,fb.h},{0,0,0,.4f},14);r.rectangle(fb,c.line,14);r.rectangle({fb.x+1,fb.y+1,fb.w-2,fb.h-2},panel,13);
  r.rectangle({fb.x+1,fb.y+12,3,fb.h-24},color,1.5f);r.circle(fb.x+29,fb.y+31,13,color);
  if(menu.result()==SaveResult::Success){r.line(fb.x+22,fb.y+31,fb.x+27,fb.y+36,2.2f,panel);r.line(fb.x+27,fb.y+36,fb.x+36,fb.y+25,2.2f,panel);}else r.text(fb.x+25,fb.y+19,"!",21,panel,1);
  r.text(fb.x+52,fb.y+18,menu.title(),22,color,1);
  auto first=toast.page*toast.rows;for(size_t i=first;i<std::min(first+toast.rows,toast.lines.size());++i)r.text(fb.x+52,fb.y+58+(i-first)*toast.line_height,toast.lines[i],19,c.ink);
  if(toast.pages>1)r.text(fb.x+52,fb.y+fb.h-28,"Details "+std::to_string(toast.page+1)+" / "+std::to_string(toast.pages),16,c.muted);
 }

}
}
