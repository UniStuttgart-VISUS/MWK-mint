cmake_minimum_required(VERSION 3.13)

project(interop VERSION 1.0
	DESCRIPTION "Interop Library to exchange Images and Camera Configuration between Application"
	LANGUAGES CXX)

add_library(interop
    src/interop.cpp)

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(interop PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

# If we have compiler requirements for this library, list them here
target_compile_features(interop
    #PUBLIC cxx_auto_type
    PRIVATE cxx_std_14)

# Depend on a library that we defined in the top-level file
target_link_libraries(interop
    #-Spout2
    #-OSC
	#-ZeroMQ
	#-RapidJSON
	""
	)

# 'make install' to the correct locations (provided by GNUInstallDirs).
# install(TARGETS interop EXPORT MyLibraryConfig
#     ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
# install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
#install(EXPORT MyLibraryConfig DESTINATION share/MyLibrary/cmake)

# This makes the project importable from the build directory
#export(TARGETS interop FILE MyLibraryConfig.cmake)

# Every library has unit tests, of course
#add_executable(testlib
#    test/testlib.cpp)
#target_link_libraries(testlib
#    interop)
#add_test(testlib testlib)