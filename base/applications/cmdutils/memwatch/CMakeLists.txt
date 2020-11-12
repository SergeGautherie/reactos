
add_executable(memwatch memwatch.cpp)
set_module_type(memwatch win32cui)
target_link_libraries(memwatch getopt cppstl)
set_target_cpp_properties(memwatch WITH_EXCEPTIONS)
add_importlibs(memwatch msvcrt kernel32 user32 ntdll)
add_cd_file(TARGET memwatch DESTINATION reactos/system32 FOR all)
