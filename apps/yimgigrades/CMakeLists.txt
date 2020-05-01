add_executable(yimgigrades yimgigrades.cpp)

set_target_properties(yimgigrades PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(yimgigrades PUBLIC ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(yimgigrades yocto yocto_gui yocto_grade)
