#include "draw.hpp"
#include "font_widths.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma clang diagnostic pop
namespace arcade {
namespace {
SDL_Color color(unsigned v){return {Uint8(v>>16),Uint8(v>>8),Uint8(v),255};}
SDL_Texture* bitmap(SDL_Renderer* r,const std::filesystem::path& path){auto* s=SDL_LoadBMP(path.c_str());if(!s)throw std::runtime_error("Cannot load font atlas: "+std::string(SDL_GetError()));auto* t=SDL_CreateTextureFromSurface(r,s);SDL_FreeSurface(s);if(!t)throw std::runtime_error(SDL_GetError());SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(t,SDL_ScaleModeLinear);return t;}
}
Palette palette(Theme t){return t==Theme::Day?
 Palette{color(0xfaf7f0),color(0xf0ece3),color(0x101a25),color(0x68717c),color(0xdedbd3),color(0xe94c0a),color(0xffffff),color(0xffe1d2)}:
 Palette{color(0x11161e),color(0x1c2430),color(0xf4f0e8),color(0xa8b2c0),color(0x303b49),color(0xff824f),color(0xffffff),color(0x36251f)};
}
Draw::Draw(SDL_Renderer* r,const std::filesystem::path& assets):renderer_(r){
 font_[0]=bitmap(r,assets/"font-regular.bmp");font_[1]=bitmap(r,assets/"font-bold.bmp");
 int w,h,n;auto* bytes=stbi_load((assets/"cover-concepts.png").c_str(),&w,&h,&n,4);
 if(bytes){auto* s=SDL_CreateRGBSurfaceWithFormatFrom(bytes,w,h,32,w*4,SDL_PIXELFORMAT_RGBA32);if(s){covers_=SDL_CreateTextureFromSurface(r,s);SDL_FreeSurface(s);}stbi_image_free(bytes);if(covers_)SDL_SetTextureScaleMode(covers_,SDL_ScaleModeLinear);}
}
Draw::~Draw(){for(auto* f:font_)SDL_DestroyTexture(f);SDL_DestroyTexture(covers_);}
void Draw::rect(float x,float y,float w,float h,SDL_Color c){SDL_SetRenderDrawColor(renderer_,c.r,c.g,c.b,c.a);SDL_FRect r{x,y,w,h};SDL_RenderFillRectF(renderer_,&r);}
void Draw::rounded_texture(SDL_Texture* texture,const SDL_Rect& source,float x,float y,float w,float h,float radius,SDL_Color color){
 radius=std::min(radius,std::min(w,h)/2);int tw=1,th=1;if(texture)SDL_QueryTexture(texture,nullptr,nullptr,&tw,&th);
 std::vector<SDL_Vertex> vertices;std::vector<int> indices;
 auto point=[&](float px,float py){SDL_FPoint uv{(source.x+(px-x)/w*source.w)/tw,(source.y+(py-y)/h*source.h)/th};vertices.push_back({{px,py},color,uv});};
 point(x+w/2,y+h/2);
 const float centers[4][2]={{x+w-radius,y+radius},{x+w-radius,y+h-radius},{x+radius,y+h-radius},{x+radius,y+radius}};
 for(int corner=0;corner<4;++corner)for(int step=0;step<=12;++step){float a=(-.5f+corner*.5f+step/24.f)*3.14159265f;point(centers[corner][0]+std::cos(a)*radius,centers[corner][1]+std::sin(a)*radius);}
 for(int i=1;i<static_cast<int>(vertices.size());++i){indices.push_back(0);indices.push_back(i);indices.push_back(i+1==static_cast<int>(vertices.size())?1:i+1);}
 SDL_RenderGeometry(renderer_,texture,vertices.data(),static_cast<int>(vertices.size()),indices.data(),static_cast<int>(indices.size()));
}
void Draw::rounded(float x,float y,float w,float h,float radius,SDL_Color color){rounded_texture(nullptr,{0,0,1,1},x,y,w,h,radius,color);}
void Draw::button(float x,float y,float w,float h,const std::string& label,SDL_Color bg,SDL_Color fg,bool play){
 rounded(x,y,w,h,13,bg);
 float text_width=measure(label,30,true),group=text_width+(play?46:0),left=x+(w-group)/2;
 if(play){
  SDL_Vertex v[]={{{left,y+h/2-14},fg,{}},{{left,y+h/2+14},fg,{}},{{left+23,y+h/2},fg,{}}};
  const int indices[]={0,1,2};SDL_RenderGeometry(renderer_,nullptr,v,3,indices,3);left+=46;
 }
 text(left,y+(h-30)/2-2,label,30,fg,true);
}
void Draw::stroke(float ax,float ay,float bx,float by,float width,SDL_Color c){
 float length=std::hypot(bx-ax,by-ay);if(length<=0)return;float nx=-(by-ay)/length*width/2,ny=(bx-ax)/length*width/2;
 SDL_Vertex v[]={{{ax+nx,ay+ny},c,{}},{{bx+nx,by+ny},c,{}},{{bx-nx,by-ny},c,{}},{{ax-nx,ay-ny},c,{}}};const int indices[]={0,1,2,0,2,3};SDL_RenderGeometry(renderer_,nullptr,v,4,indices,6);
}
void Draw::hint(char symbol,float x,float y,const std::string& label,SDL_Color ink,SDL_Color paper,float size){
 float d=size+11,r=d/2;rounded(x,y,d,d,r,ink);
 SDL_SetRenderDrawColor(renderer_,paper.r,paper.g,paper.b,paper.a);
 auto line=[&](float ax,float ay,float bx,float by){stroke(x+ax,y+ay,x+bx,y+by,1.8f,paper);};
 if(symbol=='x'){line(d*.29f,d*.29f,d*.71f,d*.71f);line(d*.71f,d*.29f,d*.29f,d*.71f);}
 else if(symbol=='t'){line(r,d*.23f,d*.77f,d*.72f);line(d*.77f,d*.72f,d*.23f,d*.72f);line(d*.23f,d*.72f,r,d*.23f);}
 else if(symbol=='b'){SDL_FPoint pts[41];for(int i=0;i<=40;++i){float a=i*6.2831853f/40;pts[i]={x+r+std::cos(a)*d*.25f,y+r+std::sin(a)*d*.25f};}for(int i=0;i<40;++i)stroke(pts[i].x,pts[i].y,pts[i+1].x,pts[i+1].y,1.8f,paper);}
 else if(symbol=='s'){outline(x+d*.28f,y+d*.28f,d*.44f,d*.44f,paper);}
 else {for(int i=0;i<3;++i)line(d*.26f,d*.31f+i*d*.18f,d*.74f,d*.31f+i*d*.18f);}
 text(x+d+14,y+(d-size)/2-1,label,size,ink);
}
void Draw::outline(float x,float y,float w,float h,SDL_Color c,int thick){for(int i=0;i<thick;++i){SDL_SetRenderDrawColor(renderer_,c.r,c.g,c.b,c.a);SDL_FRect r{x-i,y-i,w+i*2,h+i*2};SDL_RenderDrawRectF(renderer_,&r);}}
float Draw::measure(const std::string& s,float size,bool bold)const{float w=0;auto* widths=bold?font_bold_widths:font_regular_widths;for(unsigned char c:s)w+=widths[(c>=32&&c<128?c:'?')-32]*size/48.f;return w;}
void Draw::text(float x,float y,std::string s,float size,SDL_Color c,bool bold,float max){
 if(measure(s,size,bold)>max){while(!s.empty()&&measure(s+"...",size,bold)>max)s.pop_back();s+="...";}
 auto* f=font_[bold?1:0];auto* widths=bold?font_bold_widths:font_regular_widths;SDL_SetTextureColorMod(f,c.r,c.g,c.b);SDL_SetTextureAlphaMod(f,c.a);
 for(unsigned char code:s){unsigned i=(code>=32&&code<128?code:'?')-32;SDL_Rect src{int(i%16)*64,int(i/16)*72,64,72};SDL_FRect dst{x,y-size*.17f,64*size/48.f,72*size/48.f};SDL_RenderCopyF(renderer_,f,&src,&dst);x+=widths[i]*size/48.f;}
}
void Draw::wrap(float x,float y,const std::string& s,float size,SDL_Color color,float width,int max_lines){std::istringstream words(s);std::string line,word;int n=0;while(words>>word){if(!line.empty()&&measure(line+" "+word,size)>width){text(x,y+n*size*1.4f,line,size,color,false,width);line.clear();if(++n>=max_lines)return;}if(!line.empty())line+=' ';line+=word;}if(n<max_lines)text(x,y+n*size*1.4f,line,size,color,false,width);}
void Draw::cover(const std::string& id,float x,float y,float w,float h,SDL_Color c){
 int i=-1;const char* ids[]={"robby","gridlee","circus","looping","targ","alienar"};for(int j=0;j<6;++j)if(id==ids[j])i=j;
 // Original concept art from the approved Gallery proposal; only the art regions are sampled.
 const SDL_Rect sources[]={{348,140,234,299},{626,140,221,299},{887,140,221,299},{349,523,233,307},{626,523,221,307},{887,523,221,307}};
 if(covers_&&i>=0){rounded_texture(covers_,sources[i],x,y,w,h,7,{255,255,255,255});return;}
 rect(x,y,w,h,c);SDL_Color ink{255,255,255,80};for(int j=0;j<7;++j)outline(x+w*.1f+j*5,y+h*.1f+j*9,w*.7f,h*.6f,ink);text(x+18,y+h*.68f,id,25,{255,255,255,255},true,w-36);
}
void Draw::icon(int kind,float x,float y,SDL_Color c,SDL_Color paper){
 if(kind==0){
  rounded(x,y+4,29,19,7,c);rounded(x-1,y+12,10,14,4,c);rounded(x+20,y+12,10,14,4,c);
  rounded(x+5,y+12,10,3,1,paper);rounded(x+8.5f,y+8.5f,3,10,1,paper);
  rounded(x+21,y+10,3,3,1.5f,paper);rounded(x+24,y+14,3,3,1.5f,paper);
 }else if(kind==1){
  SDL_Vertex v[11];v[0]={{x+14,y+14},c,{}};
  for(int i=0;i<10;++i){float a=-1.5708f+i*.628319f,r=i%2?6.5f:14;v[i+1]={{x+14+std::cos(a)*r,y+14+std::sin(a)*r},c,{}};}
  int indices[30];for(int i=0;i<10;++i){indices[i*3]=0;indices[i*3+1]=i+1;indices[i*3+2]=i==9?1:i+2;}SDL_RenderGeometry(renderer_,nullptr,v,11,indices,30);
 }else{
  rounded(x+3,y+3,23,23,11.5f,c);for(int i=0;i<8;++i){float a=i*.785398f;rounded(x+11+std::cos(a)*12,y+11+std::sin(a)*12,7,7,1,c);}rounded(x+10,y+10,9,9,4.5f,paper);
 }
}
void Draw::capture(const std::filesystem::path& path){
 int w,h;SDL_GetRendererOutputSize(renderer_,&w,&h);auto* s=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_ARGB8888);if(!s)throw std::runtime_error(SDL_GetError());
 if(SDL_RenderReadPixels(renderer_,nullptr,s->format->format,s->pixels,s->pitch)<0){SDL_FreeSurface(s);throw std::runtime_error(SDL_GetError());}
 int result=SDL_SaveBMP(s,path.c_str());SDL_FreeSurface(s);if(result)throw std::runtime_error(SDL_GetError());
}
}
