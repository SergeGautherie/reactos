
## Experimental 16-bit ASM compilation support
include(asm16.cmake)

add_subdirectory(config)
if(ARCH STREQUAL "i386") # CORE-17585
add_subdirectory(dos)
# endif()
add_subdirectory(ntvdm)
endif()
add_subdirectory(pifmgr)
if(ARCH STREQUAL "i386")
add_subdirectory(samples)
endif()
add_subdirectory(vdmdbg)
add_subdirectory(wow16)
