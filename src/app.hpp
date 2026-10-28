#pragma once
#include "core.hpp"
#include "draw.hpp"
#include "model.hpp"
#include <array>
namespace arcade {
struct Options {
 std::filesystem::path data,assets,roms,core,capture;
 bool demo=false,night=false,day=false,ui_smoke=false;
 unsigned run_frames=0;
};
class App {
public:
 App(SDL_Window*,SDL_Renderer*,const Options&);
 ~App();
 int run();
private:
 SDL_Renderer* renderer_;Options options_;Draw draw_;Model model_;Core core_;
 std::array<SDL_GameController*,4> pads_{};
 SDL_Texture* frame_=nullptr;unsigned frame_w_=0,frame_h_=0;
 bool running_=true,paused_=false,editing_=false,loading_=false;
 std::string edit_,notice_;unsigned long frames_=0;
 void rescan();void save();void theme(Theme);
 void event(const SDL_Event&);void action(SDL_Keycode);
 void activate();void favorite();void play();void stop();void controllers();void game_input();
 void render();void library();void settings();void footer();void game();void modal();
 void click(float,float,int);
};
}
