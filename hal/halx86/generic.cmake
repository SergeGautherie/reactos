
list(APPEND HAL_GENERIC_SOURCE
    generic/beep.c
    generic/cmos.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/halinit.c
    generic/memory.c
    generic/misc.c
    generic/pic.c
    generic/reboot.c
    generic/sysinfo.c
    generic/usage.c)

if(ARCH STREQUAL "i386")
    list(APPEND HAL_GENERIC_SOURCE
        generic/bios.c
        generic/portio.c)

    list(APPEND HAL_GENERIC_ASM_SOURCE
        generic/v86.S)
endif()

add_asm_files(lib_hal_generic_asm ${HAL_GENERIC_ASM_SOURCE})
if(STACK_PROTECTOR)
    if(OPTIMIZE STREQUAL "2")
    # if(NOT OPTIMIZE STREQUAL "1")
        # Checking whether that helps 'bt'...
        replace_compile_flags("-O1" "-O0")
        # add_compile_flags("-O0")
        # add_target_compile_flags(freeldr_common "-O0")
    endif()
endif()
add_object_library(lib_hal_generic ${HAL_GENERIC_SOURCE} ${lib_hal_generic_asm})
add_dependencies(lib_hal_generic asm)
