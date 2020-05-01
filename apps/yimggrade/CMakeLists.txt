add_executable(yimggrade yimggrade.cpp)

set_target_properties(yimggrade PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(yimggrade PUBLIC ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(yimggrade yocto yocto_grade)
