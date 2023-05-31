# adapted from https://rix0r.nl/blog/2015/08/13/cmake-guide/

cmake_minimum_required(VERSION 3.13)

project(mint VERSION 1.0
	DESCRIPTION "Minimal Interop Rendering Application"
	LANGUAGES CXX)
	
# include libraries?

option(MINT_BUILD_INTEROP "" ON)
option(MINT_BUILD_RENDERING "" ON)

# add sub-projects

if(MINT_BUILD_INTEROP)
	add_subdirectory(interop)
endif()

if(MINT_BUILD_RENDERING)
	add_subdirectory(rendering)
endif()
