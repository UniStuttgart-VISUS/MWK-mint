# Mint - Minimal Interop Rendering

Implements a minimal C++ OpenGL Rendering Application and Interop Library to broadcast textures and exchange camera and rendering configurations via Spout2 and ZeroMQ/JSON.

## Getting Started

Clone this project and build with CMake for Visual Studio. Tested only on Windows 10.
CMake should resolve external project dependencies automatically, but is not set up to deploy or install any of the built targets.

### Building

Visual Studio builds the projects `interop` and `rendering`, which are the Interop Library and Rendering Application, respectively. 
`rendering` is used as a test case for the Interop Library.

### Installing / Deployment

TODO: set up CMake to properly export and install library

## Running the tests

TODO: No tests yet
