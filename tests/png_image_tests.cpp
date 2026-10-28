#include "gfx/png_image.hpp"
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
using arcade::native::load_png_image;
int main(int argc,char** argv){
 assert(argc==2);std::filesystem::path assets=argv[1];
 // These production files exercise multi-IDAT chunks, RGB conversion,
 // transparent RGBA and a large ancillary metadata chunk before IDAT.
 struct Fixture{const char* name;int w,h;std::uint64_t hash;};
 const Fixture files[]={{"vault-cabinet.png",858,1350,0x8708a6c0f0860932ull},{"vault-logo.png",1024,1024,0x47c5c877710da79eull},{"cover-concepts.png",1672,941,0xfc5e516bdc3027feull},{"vault-grid.png",1672,776,0x0224eb085526eeb2ull}};
 for(auto f:files){auto decoded=load_png_image((assets/f.name).string());assert(decoded);assert(decoded.width==f.w&&decoded.height==f.h);assert(decoded.pixels.size()==std::size_t(f.w)*f.h*4);bool colour=false,visible=false;for(size_t i=0;i<decoded.pixels.size();i+=4){colour|=decoded.pixels[i]!=decoded.pixels[i+1];visible|=decoded.pixels[i+3]!=0;}std::uint64_t hash=14695981039346656037ull;for(auto byte:decoded.pixels){hash^=byte;hash*=1099511628211ull;}assert(hash==f.hash);assert(visible);if(std::string(f.name)!="vault-grid.png")assert(colour);}
 for(auto name:{"arcade-room-4k.png","arcade-splash-4k.png"}){auto image=load_png_image((assets/name).string());assert(image&&image.width==3840&&image.height==2160&&image.pixels.size()==3840u*2160u*4u);}
 assert(!load_png_image((assets/"missing-image.png").string()));
 auto malformed=std::filesystem::temp_directory_path()/"arcade-invalid-png.bin";{std::ofstream out(malformed);out<<"not a PNG";}assert(!load_png_image(malformed.string()));std::filesystem::remove(malformed);
}
