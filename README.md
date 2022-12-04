# My fork of NCurses Music Player Client (Plus Plus)

The main ncmpcpp project merges things very slowly, so this fork implements a few things I want to use:
* my fix for [crashes when playing long filenames like streams](https://github.com/ncmpcpp/ncmpcpp/issues/380)
* [lyrics fetcher from tags](https://github.com/ncmpcpp/ncmpcpp/pull/482)

Project page - https://rybczak.net/ncmpcpp/

## ncmpcpp – featureful ncurses based MPD client inspired by ncmpc

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

* boost library [https://www.boost.org/]
* ncurses library [http://www.gnu.org/software/ncurses/ncurses.html]
* readline library [https://tiswww.case.edu/php/chet/readline/rltop.html]
* curl library (optional, required for fetching lyrics and last.fm data) [https://curl.haxx.se/]
* fftw library (optional, required for frequency spectrum music visualization mode) [http://www.fftw.org/]
* tag library (optional, required for tag editing) [https://taglib.org/]

### Known issues:
* No full support for handling encodings other than UTF-8.

### Installation:

The simplest way to compile this package is:

  1. `cd` to the directory containing the package's source code.

  For the next two commands, `csh` users will need to prefix them with
  `sh `.

  2. Run `./autogen.sh` to generate the `configure` script.

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
