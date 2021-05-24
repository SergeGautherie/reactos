
add_subdirectory(3rdparty)
# api-ms-win-crt-process-l1-1-0.def : error LNK2001: unresolved external symbol _loaddll
# api-ms-win-crt-process-l1-1-0.def : error LNK2001: unresolved external symbol _unloaddll
# dll\apisets\api-ms-win-crt-process-l1-1-0.lib : fatal error LNK1120: 2 unresolved externals
# +
# api-ms-win-crt-runtime-l1-1-0.def : error LNK2001: unresolved external symbol _getdllprocaddr
# dll\apisets\api-ms-win-crt-runtime-l1-1-0.lib : fatal error LNK1120: 1 unresolved externals
if(NOT ARCH STREQUAL "arm" AND NOT ARCH STREQUAL "arm64") # FIXME: ...
add_subdirectory(apisets)
endif()
add_subdirectory(appcompat)
add_subdirectory(cpl)
add_subdirectory(directx)
add_subdirectory(keyboard)
add_subdirectory(nls)
add_subdirectory(np)
add_subdirectory(ntdll)
add_subdirectory(opengl)
add_subdirectory(shellext)
add_subdirectory(win32)
