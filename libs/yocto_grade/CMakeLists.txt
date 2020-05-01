add_library(yocto_grade yocto_grade.h yocto_grade.cpp)
set_target_properties(yocto_grade PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(yocto_grade PUBLIC ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(yocto_grade yocto)
