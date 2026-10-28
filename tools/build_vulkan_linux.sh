#!/usr/bin/env bash
# Build the pinned PS5 RADV archive in the isolated Linux dependency container.
set -euo pipefail
export LLVM_CONFIG=/usr/bin/llvm-config-18
export PATH=/build/venv/bin:/usr/lib/llvm-18/bin:$PATH
export CC=/usr/bin/clang-18 CXX=/usr/bin/clang++-18
mesa=/build/PS5_Mesa-7b59ef27c1b09b9671bc4153c41940c3155c3af2
vk=/build/PS5_Vulkan-5b5e4fc2d80fdb67a4f61ba9e6a8a424026bb8a5
# Keep the source export separate from the pinned original for the qualified
# ProsperoEden display lifetime and host-memory adaptations.
if [[ -f /build/arcade-adaptation/tools/patch_vulkan_display.py ]]; then
 [[ -d /build/mesa-original ]] || cp -a --reflink=auto "$mesa" /build/mesa-original
 ARCADE_MESA_SOURCE=/build/mesa-original ARCADE_MESA_TARGET="$mesa" python3 /build/arcade-adaptation/tools/patch_vulkan_display.py
fi
if [[ ! -f /build/clc/build.ninja ]]; then
 meson setup /build/clc "$mesa" -Dbuildtype=release -Dmesa-clc=enabled -Dinstall-mesa-clc=true \
 -Dgallium-drivers= -Dvulkan-drivers= -Dplatforms= -Dglx=disabled -Degl=disabled -Dgbm=disabled \
 -Dopengl=false -Dgles1=disabled -Dgles2=disabled -Dllvm=enabled -Dshared-llvm=enabled \
 -Dbuild-tests=false -Dvalgrind=disabled -Dlibunwind=disabled -Dzstd=disabled -Dxmlconfig=disabled -Dtools=
fi
ninja -C /build/clc -j4 src/compiler/clc/mesa_clc src/compiler/spirv/vtn_bindgen2
mkdir -p /build/clc-bin
ln -sf /build/clc/src/compiler/clc/mesa_clc /build/clc-bin/mesa_clc
ln -sf /build/clc/src/compiler/spirv/vtn_bindgen2 /build/clc-bin/vtn_bindgen2
export PATH=/build/clc-bin:$PATH
printf "[constants]\nsdk = '/build/sdk'\n" > /build/cross-constants.ini
if [[ ! -f /build/radv/build.ninja ]]; then
 meson setup /build/radv "$mesa" --cross-file /build/cross-constants.ini --cross-file "$vk/tooling/radv/ps5-cross.ini" \
 -Dvulkan-drivers=amd -Dgallium-drivers= -Dplatforms= -Dradv-winsys=ps5 -Dllvm=disabled -Damd-use-llvm=false \
 -Dvideo-codecs= -Dbuildtype=debugoptimized -Db_ndebug=true -Dglx=disabled -Degl=disabled -Dgbm=disabled \
 -Dopengl=false -Dgles1=disabled -Dgles2=disabled -Dvalgrind=disabled -Dlibunwind=disabled -Dzstd=disabled \
 -Dzlib=enabled --force-fallback-for=zlib -Dexpat=disabled -Dxmlconfig=disabled -Dshader-cache=enabled \
 -Dbuild-tests=false -Dvulkan-layers= -Dtools= -Dmesa-clc=system -Dradv-build-id=7b59ef27c1b09b9671bc4153c41940c3155c3af2
fi
ninja -C /build/radv -j4 src/amd/vulkan/libvulkan_radeon.a
mkdir -p /build/arcade-radv/lib /build/arcade-radv/include
cp /build/radv/src/amd/vulkan/libvulkan_radeon.a /build/arcade-radv/lib/libvulkan_radeon.ps5.a
cp -r "$mesa/include/vulkan" "$mesa/include/vk_video" /build/arcade-radv/include/
cp /build/sdk/target/lib/libps5platform.a /build/arcade-radv/lib/
