add_subdirectory(yocto)
add_subdirectory(yocto_grade)

if(YOCTO_OPENGL)
add_subdirectory(yocto_gui)
endif(YOCTO_OPENGL)
