cmake_minimum_required(VERSION 3.13)

project(interop VERSION 1.0
	DESCRIPTION "Interop Library to exchange Images and Camera Configuration between Applications"
	LANGUAGES CXX)

# project-wide c++ standard settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake related options for this project
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# init git submodules
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/../.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
endif()

# find external dependencies (git submodules in ./external)
# Spout2
set(SKIP_INSTALL_ALL OFF CACHE INTERNAL "")
set(SPOUT_BUILD_CMT OFF CACHE INTERNAL "")
add_subdirectory(external/Spout2)

# ZeroMQ
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/libzmq)
add_subdirectory(external/cppzmq)

# json
set(BUILD_TESTING OFF CACHE INTERNAL "")
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(external/json)

add_library(interop STATIC
    src/interop.cpp
    include/interop.hpp
)
	
# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build paths.
target_include_directories(interop
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# If we have compiler requirements for this library, list them here
target_compile_features(interop
    #PUBLIC cxx_auto_type
    PRIVATE cxx_std_17)

# Depend on a library that we defined in the top-level file
target_link_libraries(interop
	PRIVATE
		Spout_static
		libzmq-static
		cppzmq-static
		nlohmann_json::nlohmann_json
)

# 'make install' to the correct locations (provided by GNUInstallDirs).
include(GNUInstallDirs)
install(TARGETS interop Spout_static #EXPORT MyLibraryConfig
	EXPORT interop-targets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT interop-targets DESTINATION ${PROJECT_NAME})


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