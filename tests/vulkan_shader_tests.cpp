#include "gfx/vulkan_processor.hpp"
#include <cassert>
#include <vector>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
int main(){
 using namespace arcade;using namespace arcade::native;
 VulkanProcessor gpu;std::string error;if(!gpu.open(error)){std::cerr<<error<<'\n';return 1;}
 Settings settings;std::vector<uint32_t> pixels(16);for(unsigned i=0;i<16;++i)pixels[i]=0xff000000|((i*13)<<16)|((i*7)<<8)|(i*11);
 Frame input{pixels.data(),4,4,1},output;
 assert(gpu.process(input,settings,output,error));assert(output.width==4&&output.height==4);assert(std::vector<uint32_t>(output.pixels,output.pixels+16)==pixels);
 settings.rotation=2;assert(gpu.process(input,settings,output,error));for(unsigned i=0;i<16;++i)assert(output.pixels[i]==pixels[15-i]);
 settings.rotation=0;
 for(int effect=1;effect<=5;++effect){settings.post_processing=effect;assert(gpu.process(input,settings,output,error));bool changed=false;for(unsigned i=0;i<16;++i){changed|=output.pixels[i]!=pixels[i];assert((output.pixels[i]&0xff000000)==0xff000000);}assert(changed);}
 assert(!gpu.open_display(settings,error));assert(!gpu.display_open());
 assert(!gpu.process({},settings,output,error));settings.post_processing=0;assert(gpu.process(input,settings,output,error));gpu.close();
 std::cout<<"Actual Vulkan compute: exact passthrough, rotation, all five effects and invalid-frame recovery passed\n";
}
