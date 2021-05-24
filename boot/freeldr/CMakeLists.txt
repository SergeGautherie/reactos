
if(ARCH STREQUAL "i386") # CORE-17585
add_subdirectory(bootsect)
endif()
add_subdirectory(fdebug)
if(ARCH STREQUAL "i386") # CORE-17585
add_subdirectory(freeldr)
endif()
add_subdirectory(install)
