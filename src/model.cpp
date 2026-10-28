#include "model.hpp"
#include <algorithm>
namespace arcade {
std::vector<size_t> Model::visible()const{std::vector<size_t> v;for(size_t i=0;i<games.size();++i)if(page!=Page::Favorites||favorites.count(games[i].id))v.push_back(i);return v;}
const Game* Model::current()const{auto v=visible();return selected<v.size()?&games[v[selected]]:nullptr;}
void Model::clamp(){auto n=visible().size();selected=n?std::min(selected,n-1):0;}
void Model::change_page(Page p){page_selection[static_cast<int>(page)]=selected;page=p;selected=page_selection[static_cast<int>(p)];setting=0;rail=false;clamp();}
void Model::move(int dx,int dy){
 if(rail){if(dx>0){rail=false;return;}if(dy){int p=std::clamp(static_cast<int>(page)+dy,0,2);change_page(static_cast<Page>(p));rail=true;}return;}
 if(page==Page::Settings){if(dx<0){rail=true;return;}setting=std::clamp(setting+dy,0,settings_category_count-1);return;}
 auto n=visible().size();if(!n){if(dx<0)rail=true;return;}
 int i=static_cast<int>(selected);
 if(dx<0&&i%3==0){rail=true;return;}
 if(dx&&i/3==(i+dx)/3&&i+dx>=0&&i+dx<static_cast<int>(n))i+=dx;
 if(dy&&i+dy*3>=0&&i+dy*3<static_cast<int>(n))i+=dy*3;
 selected=static_cast<size_t>(i);
}
void Model::toggle_favorite(){auto* g=current();if(!g)return;auto id=g->id;if(!favorites.erase(id))favorites.insert(id);clamp();}
}
