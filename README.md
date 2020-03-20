# Comix
This project is an exercise in C++ for learning to use the SDL libraries.

As the title implies, this is an image viewer program, designed specifically 
for viewing digital comics. It is completely configured to my personal tastes, 
so if you'd like to change anything I'm afraid you must modify the source and
re-compile; However I have plans for a XML-based config file option.

The resource files included in `res` are closely related to the source code, as 
such trying to substitude them with your own is possible but likely to be annoying.

## Dependencies
- SDL2 ver. 2.0.9+
- SDL2_ttf ver. 2.0.15+
- SDL2_image ver. 2.0.4+
- C++17 support

## Compiling
The project does not contain any platform-specific code. After aquiring the 
dependencies listed above simply configure include paths, link the libraries, 
and compile the source files.