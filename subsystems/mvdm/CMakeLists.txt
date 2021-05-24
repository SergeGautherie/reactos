
if(NOT MSVC OR (ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")) # LocalOnly SYNC: Only used by dos.
## Experimental 16-bit ASM compilation support
include(asm16.cmake)
endif()

add_subdirectory(config)
if(NOT MSVC OR (ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")) # FIXME: CORE-10452
add_subdirectory(dos)
# Depends on dos:
# ...\ntvdm\dos\dem.c(768): fatal error C1083: Cannot open include file: 'command_com.h': No such file or directory
add_subdirectory(ntvdm)
endif()
add_subdirectory(pifmgr)
if(ARCH STREQUAL "i386")
add_subdirectory(samples)
endif()
add_subdirectory(vdmdbg)
add_subdirectory(wow16)
