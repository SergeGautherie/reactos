
add_subdirectory(arbiter)
add_subdirectory(aux_klib)
add_subdirectory(chew)
add_subdirectory(copysup)
add_subdirectory(csq)
add_subdirectory(hidparser)
add_subdirectory(ip)
add_subdirectory(lwip)
add_subdirectory(ntoskrnl_vista)
add_subdirectory(rdbsslib)
add_subdirectory(rtlver)
add_subdirectory(rxce)
add_subdirectory(sound)
# 137 times:
# ...\sdk\lib\drivers\wdf\shared\inc\private\km\fxglobalskm.h(393): error C3861: 'KeGetCurrentProcessorIndex': identifier not found
# ...\sdk\lib\drivers\wdf\shared\inc\private\km\fxglobalskm.h(412): error C3861: 'KeGetCurrentProcessorIndex': identifier not found
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: CORE-17619.
add_subdirectory(wdf)
endif()
