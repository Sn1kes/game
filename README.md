# What is this?
My first attempts at writing 3d game on C language from scratch.
# Requirements
* Windows 7 or higher.
* Amd64 processor that supports SSE instructions.
* Graphics adapter, that supports directx 11 with feature level 10.0 and WDDM 1.0.
# Build
In order to build the project you will need:
* Mingw-w64 runtime.
* Gcc or clang compiler compatible with C11 and gnu extensions.
* Fxc shader compiler that comes with directx SDK or Visual Studio.

Then do these instructions:
1. Extract Fxc.exe in root directory and compile shaders with shader_compile.bat.
2. Run make in one of the build directories.

You can setup build options manually by modifying makefiles.
