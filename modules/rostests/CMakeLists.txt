
start_module_group(rostests)

add_definitions(-DHAVE_APITEST)

if(MSVC_IDE)
    add_definitions(-DWINETEST_MSVC_IDE_FORMATTING)
endif()

add_subdirectory(apitests)
#add_subdirectory(dibtests)
add_subdirectory(drivers)
#add_subdirectory(dxtest)
# 22 times:
# ...\kmtests\include\kmt_test_kernel.h(60): error C4013: '__readeflags' undefined; assuming extern returning int
# ++
# ...\kmtests\kmtest_drv\structs.c(17): error C2118: negative subscript
# and some more.
# +
# ...\kmtests\ntos_ex\ExInterlocked.c(265): error C4189: 'pSpinLock': local variable is initialized but not referenced
# +
# ...\kmtests\ntos_ke\KeIrql.c(39): error C2065: 'CMCI_LEVEL': undeclared identifier
# and 2 more.
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") # FIXME: ROSTESTS-363.
add_subdirectory(kmtests)
endif()
#add_subdirectory(regtests)
add_subdirectory(rosautotest)
add_subdirectory(tests)
add_subdirectory(win32)
add_subdirectory(winetests)


## Extra files needed for the various tests can be added into the "testdata" subdirectory.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/testdata/)
    file(GLOB_RECURSE TESTDATA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/testdata/ ${CMAKE_CURRENT_SOURCE_DIR}/testdata/*)
    foreach(item ${TESTDATA_FILES})
        add_rostests_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/testdata/${item} SUBDIR testdata)
    endforeach(item)
endif()

end_module_group()
