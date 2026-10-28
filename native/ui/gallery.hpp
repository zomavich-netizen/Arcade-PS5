#pragma once
#include "gfx/renderer.hpp"
#include "model.hpp"
#include "settings_menu.hpp"
#include <optional>
#include <algorithm>
#include "cover_art.hpp"
#include "ui/game_card.hpp"
#include "ui/pause_menu.hpp"
#include <chrono>
namespace arcade::native {
class Gallery {
public:
 Gallery(std::filesystem::path data,Settings settings,bool demo=false,std::filesystem::path catalogue={});
 void input(Key);
 void click(float x,float y,bool double_click=false);
 void draw(Renderer&);
 void loading(Renderer&);
 PauseLayout start_choice_layout()const{return paused_layout(PauseMenu{});}
 void paused(Renderer&,const Frame&,const PauseMenu&);
 PauseLayout paused_layout(const PauseMenu& menu)const{auto* game=model_.current();return pause_layout(menu,game?card::display_title(game->title):"Game",model_.settings.large_text);}
 std::optional<Game> take_launch();
 void error(const std::string& message,const std::string& code="ARC-0001"){message_=message;message_code_=code;message_page_=0;}
 bool message_open()const{return !message_.empty();}
 size_t message_page()const{return message_page_;}
 void message_scroll(Key);

 void toggle_theme();
 const Settings& preferences()const{return model_.settings;}
 const Settings& runtime_preferences()const{return effective_;}
 const Settings& feedback_preferences()const{return sheet_open_?effective_:model_.settings;}
 void controller(std::uint16_t);
 unsigned take_credits(){auto n=credits_;credits_=0;return n;}
 bool take_start(){auto value=start_requested_;start_requested_=false;return value;}
 void return_to_library();
 unsigned credits()const{return credits_;}
 void refund_credits(unsigned count){credits_=std::min(99u,credits_+count);}
 bool choosing_start()const{return choosing_start_;}
 bool load_requested()const{return load_requested_;}
 bool launching()const{return loading_;}
 bool game_sheet_open()const{return sheet_open_;}
 std::filesystem::path profile_data()const{return profile_directory(data_,model_.settings.profile);}
 int take_feedback(){int value=feedback_;feedback_=0;return value;}
 void show_settings(int category=-1){model_.change_page(Page::Settings);if(category>=0){model_.setting=category;settings_panel_=1;settings_row_=0;}}
 Page page()const{return model_.page;}
 const Game* current()const{return model_.current();}
 void finish_launch(){loading_=false;return_to_library();}
private:
 Model model_;std::filesystem::path data_;CoverCache covers_;GameMetadata metadata_;std::chrono::steady_clock::time_point next_scan_{};bool demo_=false,loading_=false;
 bool browsing_=false;std::filesystem::path folder_;std::vector<std::filesystem::path> folders_;size_t folder_index_=0;
 void draw_message(Renderer&,const std::string&,const std::string&,bool save_actions=false);
 void dismiss_message(){message_.clear();message_code_.clear();message_page_=0;}
 std::string message_code_;size_t message_page_=0,message_pages_=1;
 std::string message_;std::optional<Game> launch_;
 int settings_panel_=0,settings_row_=0,feedback_=0;float settings_focus_=0;
 std::vector<std::string> wallpapers_;
 bool choosing_start_=false,load_requested_=false;int start_choice_=0;
 void begin_launch(bool resume);void draw_start_choice(Renderer&);
 void refresh(bool quiet=false);bool save();void favorite();void play(bool start=false);
 bool settings_input(Key);void settings_activate(int);std::vector<SettingRow> settings_rows()const;void backup_saves();
 void library(Renderer&);void settings(Renderer&);void browse(Renderer&);void list_folder();void accept_folder();
 struct CoverPose {std::string id,title;float x=0,y=0,a=0,s=1,tx=0,ty=0,ta=0,ts=1;};
 std::map<std::string,CoverPose> poses_;
 Settings effective_;
 std::map<std::string,Settings> overrides_;
 std::string loaded_game_,previous_game_,toast_;
 bool sheet_open_=false,sheet_game_=true,start_requested_=false,boundary_latched_=false;
 int sheet_tab_=0,sheet_row_=0,stick_direction_=0,boundary_direction_=0;
 unsigned credits_=0;std::uint16_t held_=0;
 float clock_=0,stick_=0,screen_mix_=1,sheet_mix_=0;
 std::chrono::steady_clock::time_point vault_tick_=std::chrono::steady_clock::now(),launch_at_{},toast_until_{},coin_at_{},return_at_=std::chrono::steady_clock::now();
 card::SaveInfo card_save_;std::filesystem::path card_save_path_;std::chrono::steady_clock::time_point card_save_probe_{};
 const card::SaveInfo& card_save();
 void vault_init();void vault_draw(Renderer&);void legacy_draw(Renderer&);
 bool vault_input(Key);void vault_click(float,float,bool);
 void vault_move(int);void vault_select(size_t);void sync_preferences();
 void vault_header(Renderer&);void vault_grid(Renderer&);void vault_cabinet(Renderer&);void vault_sheet(Renderer&);
 void sheet_change(int);void sheet_save(const Settings&);void notify(const std::string&);void boundary(int);
};
}
