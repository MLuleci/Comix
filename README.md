# ImageViewer

## Compiling
The project must be linked to the SDL and SDL_image libraries and include directories. 
The following DLL files must be in the same directory as the source files:
* libjpeg-9.dll
* libpng16-16.dll
* libfreetype-6.dll
* SDL2.dll
* SDL2_image.dll
* SDL2_ttf.dll
* zlib1.dll

## Config options
The program supports config files with options the user can edit as seen below:

Option      | Purpose                                                       | Possible values       | Defaults
------------|---------------------------------------------------------------|-----------------------|-----------------------
`KeepZoom`  | If `true` the zoom will not reset when the image is changed   | `true` or `false`     | `false`
`ReadRight` | If `true` the right arrow key advances to next page           | `true` or `false`     | `true`
`Background`| Changes the background color behind the image                 | `(r, g, b)`           | `(255, 255, 255)`
`Mode`      | Window starts as `full`screen, `max`imized, or as `last` size | `full`, `max`, `last` | `max`
`LastSize`  | If `Mode` is set as `last`, size & position are stored here   | `(x, y, w, h)`        | `(centre, centre, 640, 480)`
`Borderless`| Need it say more?                                             | `true` or `false`     | `false`
`FontSize`  | Font size (height in pixels) for the info bar                 | `0-inf`               | `12`

The config file must be placed in the same directory as the executable and named `.config`, if no file is found defaults are used.
The lines must be formatted like `OPTION:VALUE\n`, wrongly formatted lines are ignored and defaults used instead.
The program uses the first font file it finds inside the `res` directory within the same directory as the executable. Font files must be '.ttf' format.