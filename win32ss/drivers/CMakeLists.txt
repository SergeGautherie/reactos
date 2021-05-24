
add_subdirectory(displays)
add_subdirectory(font)
add_subdirectory(miniport)
# win32ss\drivers\videoprt\int10.c depends on x86/amd64...
# Is there x86 BIOS Int 10h on arm/arm64/...?
# D:\a\reactos\reactos\src\win32ss\drivers\videoprt\int10.c(266): error C4013: 'x86BiosAllocateBuffer' undefined; assuming extern returning int
# D:\a\reactos\reactos\src\win32ss\drivers\videoprt\int10.c(299): error C4013: 'x86BiosFreeBuffer' undefined; assuming extern returning int
# D:\a\reactos\reactos\src\win32ss\drivers\videoprt\int10.c(337): error C4013: 'x86BiosReadMemory' undefined; assuming extern returning int
# D:\a\reactos\reactos\src\win32ss\drivers\videoprt\int10.c(373): error C4013: 'x86BiosWriteMemory' undefined; assuming extern returning int
# D:\a\reactos\reactos\src\win32ss\drivers\videoprt\int10.c(400): error C2039: 'Eax': is not a member of '_CONTEXT'
# D:\a\reactos\reactos\build\sdk\include\ddk\ntddk.h(3509): note: see declaration of '_CONTEXT'
# ...
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
add_subdirectory(videoprt)
endif()
add_subdirectory(watchdog)

