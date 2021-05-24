if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: CORE-17619.
add_subdirectory(cdrom_new)
endif()
add_subdirectory(classpnp)
add_subdirectory(disk_new)
add_subdirectory(ramdisk)
