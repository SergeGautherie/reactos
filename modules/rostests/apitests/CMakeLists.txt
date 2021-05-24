
PROJECT(ApiTests)

include_directories(include)

add_subdirectory(acpi)
add_subdirectory(advapi32)
add_subdirectory(afd)
add_subdirectory(apphelp)
add_subdirectory(appshim)
add_subdirectory(atl)
add_subdirectory(browseui)
add_subdirectory(cmd)
add_subdirectory(com)
add_subdirectory(comctl32)
if (NOT USE_DUMMY_PSEH)
    add_subdirectory(compiler)
endif()
add_subdirectory(crt)
add_subdirectory(dbghelp)
add_subdirectory(dciman32)
add_subdirectory(dnsapi)
add_subdirectory(fontext)
add_subdirectory(gdi32)
add_subdirectory(gditools)
add_subdirectory(iphlpapi)
# ...\sdk\include\crt\xmmintrin.h(45): error C2719: 'unnamed-parameter': formal parameter with requested alignment of 16 won't be aligned
# ...\sdk\include\crt\xmmintrin.h(46): error C2719: 'a': formal parameter with requested alignment of 16 won't be aligned
# ...\sdk\include\crt\xmmintrin.h(46): error C2719: 'b': formal parameter with requested alignment of 16 won't be aligned
# ...\sdk\include\crt\xmmintrin.h(47): error C2719: 'a': formal parameter with requested alignment of 16 won't be aligned
# ...\sdk\include\crt\xmmintrin.h(47): error C2719: 'b': formal parameter with requested alignment of 16 won't be aligned
# ...\sdk\include\crt\xmmintrin.h(51): error C4163: '_mm_getcsr': not available as an intrinsic function
# ...\sdk\include\crt\xmmintrin.h(53): error C4163: '_mm_setcsr': not available as an intrinsic function
# ...\sdk\include\crt\xmmintrin.h(56): error C4163: '_mm_xor_ps': not available as an intrinsic function
# ...\sdk\include\crt\xmmintrin.h(57): error C4163: '_mm_div_ps': not available as an intrinsic function
# ...\modules\rostests\apitests\kernel32\SetUnhandledExceptionFilter.c(62): fatal error C1189: #error:  Architecture not handled
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
add_subdirectory(kernel32)
endif()
add_subdirectory(loadconfig)
add_subdirectory(localspl)
add_subdirectory(mountmgr)
add_subdirectory(msgina)
add_subdirectory(mspatcha)
add_subdirectory(msvcrt)
add_subdirectory(netapi32)
add_subdirectory(netshell)
# NtSetValueKey.c.obj : error LNK2019: unresolved external symbol __imp_RtlFillMemoryUlong referenced in function func_NtSetValueKey
# NtUnloadDriver.c.obj : error LNK2001: unresolved external symbol __imp_RtlFillMemoryUlong
# modules\rostests\apitests\ntdll\ntdll_apitest.exe : fatal error LNK1120: 1 unresolved externals
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
add_subdirectory(ntdll)
endif()
add_subdirectory(ole32)
add_subdirectory(opengl32)
add_subdirectory(pefile)
add_subdirectory(powrprof)
add_subdirectory(sdk)
add_subdirectory(setupapi)
add_subdirectory(sfc)
add_subdirectory(shell32)
add_subdirectory(shlwapi)
add_subdirectory(spoolss)
add_subdirectory(psapi)
add_subdirectory(user32)
add_subdirectory(user32_dynamic)
add_subdirectory(userenv)
add_subdirectory(utildll)
add_subdirectory(uxtheme)
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    # Lots of asm failures, from ...\win32u\win32u_*\asm\sys-stubs_win32u_*_asm.asm(*), coming from sdk/include/asm/asm.inc.
    if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ...
    add_subdirectory(win32u)
    # Depends on win32u:
    # LINK : fatal error LNK1181: cannot open input file 'libwin32u.lib'
    add_subdirectory(win32nt)
    endif()
endif()
add_subdirectory(winhttp)
add_subdirectory(wininet)
add_subdirectory(winprint)
add_subdirectory(winspool)
add_subdirectory(wlanapi)
add_subdirectory(ws2_32)
add_subdirectory(zipfldr)
