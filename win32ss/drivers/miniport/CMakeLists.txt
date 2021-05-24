
add_subdirectory(vbe)
# LINK : fatal error LNK1181: cannot open input file 'libvideoprt.lib'
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
add_subdirectory(vga)
endif()
# LINK : fatal error LNK1181: cannot open input file 'libvideoprt.lib'
add_subdirectory(vga_new)
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
add_subdirectory(vmx_svga)
endif()

if(ARCH STREQUAL "i386")
    add_subdirectory(pc98vid)
    add_subdirectory(xboxvmp)
endif()
