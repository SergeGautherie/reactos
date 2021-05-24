
# LINK : fatal error LNK1181: cannot open input file 'libvideoprt.lib'
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
add_subdirectory(dxapi)
endif()
add_subdirectory(dxg)
add_subdirectory(dxgthk)
