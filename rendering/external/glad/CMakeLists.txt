# from https://github.com/Dav1dde/glad/issues/186

cmake_minimum_required(VERSION 3.0)
project(Glad)

add_library(glad STATIC include/glad/glad.h src/glad.c)
target_include_directories(glad PUBLIC include/)

install(TARGETS glad #EXPORT MyLibraryConfig
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Then in your main CMakeLists.txt file put:
#   add_subdirectory(glad/)
#   target_link_libraries(<your target> PRIVATE glad)