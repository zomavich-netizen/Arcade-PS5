#pragma once
#include "library.hpp"
#include "settings.hpp"
#include <array>
namespace arcade {
enum class Page { Library, Favorites, Settings };
class Model {
public:
 Settings settings;
 Page page=Page::Library;
 bool rail=false;
 size_t selected=0;
 std::array<size_t,3> page_selection{};
 int setting=0;
 std::vector<Game> games;
 std::set<std::string> favorites;
 std::vector<size_t> visible()const;
 const Game* current()const;
 void move(int dx,int dy);
 void change_page(Page);
 void toggle_favorite();
 void clamp();
};
}
