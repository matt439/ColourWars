# The x64-asan preset's vcpkg triplet: stock x64-windows, except that a port
# vcpkg builds as a static library is compiled with /fsanitize=address. Its
# objects join the game's link, where the STL's container annotations must
# agree (LNK2038 otherwise). DLL ports keep the stock flags; only their import
# libraries are linked. See docs/code-hygiene.md, the x64-asan preset.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

if(PORT STREQUAL "lodepng")
    set(VCPKG_C_FLAGS "/fsanitize=address")
    set(VCPKG_CXX_FLAGS "/fsanitize=address")
endif()
