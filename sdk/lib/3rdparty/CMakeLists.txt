
add_subdirectory(adns)
add_subdirectory(cardlib)
# No (explicit) support for MSVC arm/arm64 compiler...
# ...\sdk\lib\3rdparty\freetype\src\truetype\ttinterp.c(1242): error C4101: 't2': unreferenced local variable
# ...\sdk\lib\3rdparty\freetype\src\truetype\ttinterp.c(1242): error C4101: 't': unreferenced local variable
if(NOT (MSVC AND (ARCH STREQUAL "arm" OR ARCH STREQUAL "arm64"))) # FIXME: ...
add_subdirectory(freetype)
endif()
add_subdirectory(libmpg123)
add_subdirectory(libsamplerate)
add_subdirectory(libwine)
add_subdirectory(libwin-iconv)
add_subdirectory(libxml2)
if(MSVC)
    add_subdirectory(stlport)
endif()
add_subdirectory(strmbase)
add_subdirectory(zlib)
