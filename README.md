# NCurses Music Player Client (Plus Plus)

## ncmpcpp – featureful ncurses based MPD client inspired by ncmpc

### Project status

The project is officially in maintenance mode. I (Andrzej Rybczak) still use it
daily, but it's feature complete for me and there is very limited time I have
for tending to the issue tracker and open pull requests.

No new, substantial features should be expected (at least from me). However, if
there are any serious bugs or the project outright stops compiling because of
new, incompatible versions of dependencies, it will be fixed.

### Main features:
* tag editor
* playlist editor
* easy to use search engine
* media library
* music visualizer
* ability to fetch artist info from last.fm
* new display mode
* alternative user interface
* ability to browse and add files from outside of MPD music directory
…and a lot more minor functions.

### Dependencies:
* [boost](https://www.boost.org/)
* [ncurses](https://invisible-island.net/ncurses/announce.html)
* [readline](https://tiswww.case.edu/php/chet/readline/rltop.html)
* [curl](https://curl.se), for fetching lyrics and last.fm data
#### Optional libraries
* [fftw](http://www.fftw.org), for frequency spectrum music visualization mode
* [taglib](https://taglib.org/), for tag editing

### Known issues:
* No full support for handling encodings other than UTF-8.

### Installation:
The simplest way to compile this package is:

  1. `cd` to the directory containing the package's source code.

  For the next two commands, `csh` users will need to prefix them with
  `sh `.

  2. Run `autoreconf -fiv` to generate the `configure` script.

  3. Run `./configure` to configure the package for your system.  This
     will take a while.  While running, it prints some messages
     telling which features it is checking for.

  4. Run `make` to compile the package.

  5. Type `make install` to install the programs and any data files
     and documentation.

  6. You can remove the program binaries and object files from the
     source code directory by typing `make clean`.

Detailed intallation instructions can be found in the `INSTALL` file. 

### Optional features:
Optional features can be enable by specifying them during configure. For
example, to enable visualizer run `./configure --enable-visualizer`. 

Additional details can be found in the INSTALL file. 
