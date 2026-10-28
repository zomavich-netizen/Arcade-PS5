#pragma once
#include <string>
#include <vector>
namespace arcade::native {
struct PngImage {
 int width=0,height=0;
 std::vector<unsigned char> pixels;
 std::string error;
 explicit operator bool()const{return !pixels.empty();}
};
// Read through native descriptors, then decode from memory. This avoids the
// payload/native stdio seek and pushback boundary for chunked/metadata PNGs.
bool image_diagnostics_enabled();
PngImage load_png_image(const std::string& path);
}
