#include "model.hpp"
#include <cassert>
#include <fstream>
#include <unistd.h>
int main(){
 auto root=std::filesystem::temp_directory_path()/("arcade-ui-"+std::to_string(getpid()));auto file=root/"settings.cfg";
 auto settings=arcade::read_settings(file);assert(settings.theme==arcade::Theme::Day);
 settings.theme=arcade::Theme::Night;settings.smooth=true;settings.roms=root/"Games with spaces";settings.core=root/"engine.dylib";
 arcade::write_settings(file,settings);auto read=arcade::read_settings(file);assert(read.theme==arcade::Theme::Night&&read.roms==settings.roms&&read.core==settings.core&&read.smooth);
 arcade::Model m;m.games=arcade::sample_library();assert(m.current()->id=="robby");m.move(1,0);assert(m.current()->id=="gridlee");m.move(0,1);assert(m.current()->id=="targ");m.move(1,0);m.move(1,0);assert(m.selected==5);m.move(0,1);assert(m.selected==5);
 m.toggle_favorite();m.change_page(arcade::Page::Favorites);assert(m.visible().size()==1);m.toggle_favorite();assert(m.visible().empty()&&m.current()==nullptr);m.move(-1,0);assert(m.rail);m.move(0,1);assert(m.page==arcade::Page::Settings);m.move(1,0);assert(!m.rail);
 for(int i=0;i<12;++i)m.move(0,1);assert(m.setting==arcade::settings_category_count-1);m.move(-1,0);assert(m.rail);
 {std::ofstream out(file);out<<"theme \"invalid\"\n";}assert(arcade::read_settings(file).theme==arcade::Theme::Day);
 std::filesystem::remove_all(root);
}
