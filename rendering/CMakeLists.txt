cmake_minimum_required(VERSION 3.13)

project(rendering VERSION 1.0
	DESCRIPTION "Minimal Rendering Example using Interop"
	LANGUAGES CXX)

# project-wide c++ standard settings
set(CMAKE_CXX_STANDARD 14)
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
# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)
# OpenGL
find_package(OpenGL REQUIRED)

# GLAD
add_subdirectory(external/glad/)

# GLM
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(GLM_TEST_ENABLE_CXX_14 ON CACHE BOOL "" FORCE)
add_subdirectory(external/glm/)

#add_library(interop STATIC IMPORTED ../interop)

set(LINK_LIBS_EXTERNAL interop glad glm glfw ${OPENGL_glu_LIBRARY})
set(INCLUDE_LIBS_EXTERNAL ${OPENGL_INCLUDE_DIR})
	
# build render app
add_executable(rendering src/rendering.cpp)
target_include_directories(rendering PRIVATE ${INCLUDE_LIBS_EXTERNAL})
target_link_libraries(rendering PRIVATE ${LINK_LIBS_EXTERNAL})