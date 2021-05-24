
add_subdirectory(beep)
if(NOT ARCH STREQUAL "arm") # FIXME: CORE-17617.
add_subdirectory(bootvid)
endif()
add_subdirectory(condrv)

if(_WINKD_)
    if(GDB)
        add_subdirectory(kdgdb)
    else()
        add_subdirectory(kdcom)
        if(NOT ARCH STREQUAL "arm")
            add_subdirectory(kdvm)
        endif()
    endif()
else()
    add_subdirectory(kdrosdbg)
endif()

if(NOT ARCH STREQUAL "arm")
add_subdirectory(nmidebug)
endif()
add_subdirectory(null)
