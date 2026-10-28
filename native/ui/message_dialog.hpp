#pragma once
#include "gfx/renderer.hpp"
#include <algorithm>
namespace arcade::native {
// Wrap every word, including long paths, without passing text to ellipsis clipping.
inline std::vector<std::string> message_lines(const std::string& message,float width,float size,bool large,int weight=0){
 std::vector<std::string> lines;std::string line,word;
 auto append=[&](){
  if(word.empty())return;
  if(!line.empty()&&text_width(line+" "+word,size,weight,large)>width){lines.push_back(line);line.clear();}
  if(!line.empty())line+=' ';
  for(char ch:word){if(!line.empty()&&text_width(line+ch,size,weight,large)>width){lines.push_back(line);line.clear();}line+=ch;}
  word.clear();
 };
 for(char ch:message){if(ch=='\n'){append();lines.push_back(line);line.clear();}else if(ch==' '||ch=='\t'||ch=='\r')append();else word+=ch;}
 append();if(!line.empty()||lines.empty())lines.push_back(line);return lines;
}
struct MessageLayout {
 Rect box,body,previous,close,next;std::vector<std::string> lines;
 float text_size=24,line_height=34;size_t rows=0,pages=1;
};
inline MessageLayout message_layout(const std::string& message,bool large){
 MessageLayout layout;layout.line_height=large?39:34;
 layout.lines=message_lines(message,1016,layout.text_size,large);
 const auto maximum=size_t((790-296)/layout.line_height);
 layout.rows=std::min(maximum,std::max(size_t(2),layout.lines.size()));
 layout.pages=(layout.lines.size()+layout.rows-1)/layout.rows;
 float height=std::max(420.f,296+layout.rows*layout.line_height);
 layout.box={(1672-1112.f)/2,(941-height)/2,1112,height};
 layout.body={layout.box.x+48,layout.box.y+154,1016,layout.rows*layout.line_height};
 float y=layout.box.y+height-100;
 if(layout.pages==1)layout.close={716,y,240,64};
 else{layout.previous={452,y,240,64};layout.close={716,y,240,64};layout.next={980,y,240,64};}
 return layout;
}
inline bool message_contains(Rect r,float x,float y){return x>=r.x&&x<=r.x+r.w&&y>=r.y&&y<=r.y+r.h;}
}
