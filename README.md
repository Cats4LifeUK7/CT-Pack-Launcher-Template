# CT Pack Launcher Template
This is template code so people can create there own Custom Track Pack Launcher!

## Features

- Updater
- Launch
- Exit
- Settings/Options

## Credits

- Cats4Life - Channel/Template
- r00t1024 - Making Riivolution Buildable
- Aaron "AerialX", tueidj, Tempus, megazig - Riivolution

## Dependencies

- [devkitPPC and devkitARM](https://devkitpro.org/wiki/Getting_Started)
	- `dkp-pacman -S wii-dev ppc-portlibs wii-portlibs devkitARM`
- ppc-libogg, ppc-libvorbisidec and ppc-freetype
	- `dkp-pacman -S ppc-libogg ppc-libvorbisidec ppc-freetype`
- a build environment for your host with GNU Make, some coreutils, and a C++ compiler
	- on Windows, devkitPro supplies and requires this via [MSYS2](https://www.msys2.org/)
- Python 3.x and python-yaml or pyyaml (only for rawksd)
- `curl`, `unzip`, and i386 multilibs (for dollz3)
- `zip` (only for packaging the build result)


## Building

With the required dependencies installed, just run `make launcher` in the project's directory. Also make sure to use launcher.dol instead of the other files when it is finished.
