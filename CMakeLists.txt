# adapted from https://rix0r.nl/blog/2015/08/13/cmake-guide/

cmake_minimum_required(VERSION 3.13)

project(mint VERSION 1.0
	DESCRIPTION "Minimal Interop Rendering Application"
	LANGUAGES CXX)
	
# include libraries?

# add sub-projects
add_subdirectory(interop)
add_subdirectory(rendering)