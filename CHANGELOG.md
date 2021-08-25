# ncmpcpp-0.10 (????-??-??)
* Add the configuration option `mpd_password`.
* Separate chunks of lyrics with a double newline.
* Fix separator between albums with the same name, to check for album artist
  instead of artist.

# ncmpcpp-0.9.2 (2021-01-24)
* Revert suppression of output of all external commands as that makes e.g album
  art addons no longer work.
* Gracefully handle failures when asking for a password.

# ncmpcpp-0.9.1 (2020-12-23)
* Add support for fetching lyrics from musixmatch.com.
* Fix intermittent failures of the Genius fetcher.
* Fix crash on startup with Visualizer as the initial screen.
* Fix crash on startup with Browser as the initial screen.
* Show the Visualizer immediately if it's the initial screen.
* Draw a separator between albums with the same name, but a different artist.
* Suppress output of all external commands.
* Consider mouse support when pausing and unpausing curses interface.
* Reduce CPU usage of the frequency spectrum visualizer.
* Enable Link Time Optimization by default.
* Enable full sorting of items in the local browser if it's not.
* Bind `Play` to `Backspace` by default.
* Fix fetching information about artists from last.fm.

# ncmpcpp-0.9 (2020-12-20)
* Fix various Mopidy specific bugs.
* Add `run_external_console_command` action for running terminal applications.
* Pass lyrics filename in single quotes to shell.
* Add support for fetching lyrics from zeneszoveg.hu.
* Add `Play` action for starting playback in stopped state.
* Properly expand `~` in mpd_host when preceded by a password.
* Add support for `message_delay_time` equal to 0.
* Add support for album only view in the Media Library.
* Add `random_exclude_pattern` option to a configuration file.
* Add `connected_message_on_startup` option to a configuration file.
* Fix compilation with GCC 10.
* Remove Lyricwiki fetcher as the site closed down.
* Remove Lyricsmania fetcher as it no longer works.
* Fix Genius and Tekstowo fetchers.
* Add dedicated `Load` action for loading playlists.
* Add `media_library_hide_album_dates` option to a configuration file.
* Improve look of the frequency spectrum visualizer.
* Add `visualizer_spectrum_smooth_look`, `visualizer_spectrum_dft_size`,
  `visualizer_spectrum_gain`, `visualizer_spectrum_hz_min` and
  `visualizer_spectrum_hz_max` options to a configuration file for controlling
  the look of the new spectrum visualizer.
* Add `visualizer_autoscale` option to a configuration file.
* Add `visualizer_fps` option to a configuration file (60 by default).
* Allow for editing multiple titles in the Tag Editor.
* Support gstreamer's udpsink as a data source for visualization (Mopidy).
* Deprecate `visualizer_fifo_path` in favor of `visualizer_data_source`.
* Don't run volume changing actions if there is no mixer.
* Do not loop after sending a database update command to Mopidy.
* Deprecate `visualizer_sync_interval` configuration option.
* Deprecate `noop` value of `browser_sort_mode` in favor of `none`.
* Add `type` value of `browser_sort_mode` (set by default).
* Prefer `$XDG_CONFIG_HOME/ncmpcpp` as a folder with configuration.

# ncmpcpp-0.8.2 (2018-04-11)
* Help screen: fixed display of EoF keycode
* Fixed possible integer overflow when resizing screen
* Fixed fetching lyrics from lyricsmania.com, metrolyrics.com and sing365.com
* Search engine now properly interacts with filtering
* Fixed redraw of separator after interface switch while MPD was stopped
* Reset position of a window when fetching lyrics
* Fixed compilation with ICU >= 61.

# ncmpcpp-0.8.1 (2017-10-11)
* Setting `colors_enabled` to `no` no longer results in a crash.
* Using `--quiet` command line argument no longer results in a crash.
* If songs in media library have no track numbers, sort them by their display format.
* Fixed a situation in which songs added to playlist from media library or playlist editor screens would not be immediately marked as such.
* Do not start prompt with the current search constraint when applying a new one.

# ncmpcpp-0.8 (2017-05-21)
* Configuration variable `execute_on_player_state_change` was added.
* Support for controlling whether ncmpcpp should display multiple tags as-is or make an effort to hide duplicate values (show_duplicate_tags configuration variable, enabled by default).
* Support for filtering of lists was brought back from the dead.
* Require C++14 compatible compiler during compilation.
* Lyrics from files containing DOS line endings now load properly on Linux.
* Added support for fetching lyrics from genius.com.
* Added support for fetching lyrics from tekstowo.pl.
* The list of lyrics fetchers can now be set via configuration file.
* Lyrics can now be fetched for songs with no tags.
* libcurl dependency is no longer optional.
* When an attempt to write tags fails, show detailed error message.
* Support for fetching lyrics for selected items in background was added.
* Application will now exit if stdin is closed.
* Configuration variable `visualizer_sample_multiplier` was deprecated and will be removed in 0.9.
* Wide character version of ncurses is now required.
* Added `statusbar_time_color` and `player_state_color` configuration variables for further customization of statusbar.
* Setting foreground color only now preserves current background color.
* Format information can now be attached to selected color variables in the configuration file. Because of that variable `progressbar_boldness` is now deprecated in favor of extended `progressbar_color` and `progressbar_elapsed_color` (for more information see example configuration file).
* Lyrics and last_fm can now be startup screens and are lockable.
* Action `update_environment` now also synchronizes status with MPD.
* Fixed an issue that could cause some MPD events to be missed.
* Action `jump_to_playing_song` is not runnable now if there is no playing song.
* Fixed fetching artist info in language other than English.
* Added test that checks if lyrics fetchers work (available via command line parameter --test-lyrics-fetchers).
* Fixed fetching lyrics from justsomelyrics.com.
* Added support for fetching lyrics from jah-lyrics.com and plyrics.com.
* Seek immediately after invoking appropriate action once.
* Added support for locating current song in playlist editor.
* Disable autocenter mode while searching and filtering.
* Added `--quiet` comand line argument that supresses messages shown at startup.
* Multiple songs in Media library are now added to playlist in the same order they are displayed.
* Added configuration option `media_library_albums_split_by_date` that determines whether albums in media library should be split by date.
* Added configuration option `ignore_diacritics` that allows for ignoring diacritics while searching (boost compiled with ICU support is required).
* Added support for reading multiple bindings files (the ones in ~/.ncmpcpp/bindings and $XDG_CONFIG_HOME/ncmpcpp/bindings are read by default).
* `main_window_highlight_color` and `active_column_color` configuration options are now deprecated in favor of `current_item_prefix`/`current_item_suffix` and `current_item_inactive_column_prefix`/`current_item_inactive_column_suffix` (note that now highlight of inactive column is customizable instead of the active one in presence of multiple columns).

# ncmpcpp-0.7.7 (2016-10-31)
* Fixed compilation on 32bit platforms.

# ncmpcpp-0.7.6 (2016-10-30)
* Fixed assertion failure on trying to search backwards in an empty list.
* Updated installation instructions in INSTALL file.
* Make sure that stream of random numbers is not deterministic.
* Opening playlist editor when there is no MPD playlists directory no longer freezes the application.
* Added info about behavior of MPD_HOST and MPD_PORT environment variables to man page.
* Tilde will now be expanded to home directory in visualizer_fifo_path, execute_on_song_change and external_editor configuration variables.
* Fixed lyricwiki and justsomelyrics fetchers.

# ncmpcpp-0.7.5 (2016-08-17)
* Action chains can be now used for seeking.
* Fixed fetching artist info from last.fm.
* Default value of regular_expressions was changed from `basic` to `perl` to work around boost issue (#12222).
* Fixed crash occuring when searching backward in an empty list.

# ncmpcpp 0.7.4 (2016-04-17)
* Fetching lyrics from lyricwiki.org was fixed.
* Configure script now continues without errors if ICU library was not found.

# ncmpcpp 0.7.3 (2016-01-20)
* Home and End keys are now recognized in a few specific terminal emulators.

# ncmpcpp-0.7.2 (2016-01-16)
* Attempt to add non-song item to playlist from search engine doesn't trigger assertion failure anymore.
* Fetching lyrics from metrolyrics.com was fixed.
* Alternative UI separator color is now respected regardless of the header_visibility flag.

# ncmpcpp-0.7.1 (2016-01-01)
* Selected songs in media library can now be added to playlists.
* Confirmation before shuffling a playlist can now be disabled.

# ncmpcpp-0.7 (2015-11-22)
* Playlist sorting dialog now contains `Album artist` option.
* Default keybindings were corrected to allow tag edition in the right column of tag editor.
* Mouse is properly kept disabled if it was disabled in the configuration file.

# ncmpcpp-0.6.8 (2015-11-11)
* Application is now compatible with MPD >= 0.20.
* Check for readline library was fixed.

# ncmpcpp-0.7_beta1 (2015-11-04)
* Visualizer has now support for multiple colors (visualizer_color configuration variable takes the list of colors to be used).
* Visualizer has now support for two more modes: sound wave filled and sound ellipse.
* Visualizer's spectrum mode now scales better along with window's width.
* It is now possible to abort the current action using Ctrl-C or Ctrl-G in prompt mode. As a result, empty value is no longer a special value that aborts most of the actions.
* Directories and playlists in browser can now be sorted by modification time.
* ~ is now expanded to home directory in mpd_host configuration variable.
* It is now possible to define startup slave screen using -S/--slave-screen command line option or startup_slave_screen configuration variable.
* List filtering has been removed due to the major part of its functionality overlapping with find forward/backward and obscure bugs.
* Find backward/forward function is now incremental.
* Support for 256 colors and customization of background colors has been added.
* Multiple configuration files via command line arguments are now accepted. In addition, by default ncmpcpp attempts to read both $HOME/.ncmpcpp/config and $XDG_CONFIG_HOME/ncmpcpp/config (in this order).
* Support for PDCurses has been removed due to the library being unmaintained and buggy.
* Current MPD host may now be shown in playlist (playlist_show_mpd_host configuration variable, disabled by default).
* Random album artists can now be added to the playlist.
* Case insensitive searching is now Unicode aware as long as boost was compiled with ICU support.
* Searching with regular expressions is now enabled by default.
* Support for the Perl regular expression syntax was added.
* BOOST_LIB_SUFFIX configure variable is now empty by default.
* Shuffle function now shuffles only selected range if selection in playlist is active.
* NCurses terminal sequence escaping is no longer used as it's not accurate enough.
* Selecting items no longer depends on space mode and is bound by default to Insert key.
* Support for Alt, Ctrl and Shift modifiers as well as Escape key was added.
* Action that updates the environment can now be used in bindings configuration file.
* Monolithic `press_space` action was split into `add_item_to_playlist`, `toggle_lyrics_update_on_song_change` and `toggle_visualization_type`.
* Monolithic `press_enter` action was split into `enter_directory`, `play_item`, `run_action` and `toggle_output`.
* Sorting actions were rebound to Ctrl-S.
* Support for range selection was added (bound to Ctrl-V by default). In addition, sorting, reversing and shuffling items in playlist now works on ranges.
* Support for selecting found items was added (bound to Ctrl-_ by default).
* Tracks in media library are now properly sorted for track numbers greater than 99.
* Value of `visualizer_sync_interval` is now restricted to be greater than 9.
* Output of the visualizer now scales automatically as long as `visualizer_sample_multiplier` is set to 1.
* Command line switch that prints current song to the standard output is available once again.
* Defined action chains are now shown in help screen.
* Action chains for selecting an item and scrolling up/down are now bound to Shift-Up and Shift-Down respectively.

# ncmpcpp-0.6.7 (2015-09-12)
* Fetching artist info from last.fm was fixed.

# ncmpcpp-0.6.6 (2015-09-07)
* A typo in the example configuration file was fixed.
* Fetching lyrics from lyricwiki.com was fixed.

# ncmpcpp-0.6.5 (2015-07-05)
* Description of mouse wheel usage on volume is now correct.
* Configure script now fails if either readline or pthread specific headers are not present.
* Searching in text fields now respects regular expression configuration.
* When numbering tracks in tag editor all the other track tags are discarded.
* Xiph tag DESCRIPTION is now rewritten as COMMENT when updating tags.
* Possible access of already freed memory when downloading artist info is fixed.
* Name of an item is now displayed correctly if present.
* Assertion failure when resizing terminal window with no active MPD connection is fixed.
* Lyrics fetchers were updated to work with sites that changed their HTML structure.

# ncmpcpp-0.6.4 (2015-05-02)
* Fix title of a pop-up which shows during adding selected items to the current playlist.
* Correctly deal with leading separator while parsing filenames in tag editor.
* Fix initial incorrect window size that was occuring in some cases.
* Unknown and invalid configuration options can now be ignored using command line option `ignore-config-errors`.

# ncmpcpp-0.6.3 (2015-03-02)
* Fix floating point exception when adding a specific number of random items.
* Passwords are no longer added to the input history.
* It is now possible to put more than one specific flag consecutively in formats.
* Bash completion file was removed as it no longer works.
* Description of available configuration options in man page was updated.

# ncmpcpp-0.6.2 (2014-12-13)
* Delete key now aditionally binds by default to action that deletes files in browser.
* Fix incremental seeking so that it doesn't reset after 30 seconds.

# ncmpcpp-0.6.1 (2014-11-06)
* Comment tag is now properly written to mp3 files.
* Only ID3v2.4 tags are now saved to mp3 files.
* Mouse scrolling with newer ncurses API now works properly.
* Adding songs from an album in media library now works properly with fetch delay.
* Adding songs from a playlist in playlist editor now works properly with fetch delay.
* Trying to fetch lyrics from an empty list doesn't crash the application anymore.

# ncmpcpp-0.6 (2014-10-25)
* Feature `locate song` no more has a chance to crash the program.
* Following lyrics of current song now works properly with consume mode active.
* Deletion of empty playlists in playlist editor now works properly.
* Toggling consume mode with crossfade mode active displays proper info.
* Support for regular expressions works properly now.

# ncmpcpp-0.6_beta5 (2014-10-04)
* Corrupted tags no longer cause assertion failures.
* MPD connection attempts are now made at most once per second.
* Deletion of playlists in both browser and playlist editor is now properly handled.
* Various actions no longer cause the program to crash when invoked with empty playlist.

# ncmpcpp-0.6_beta4 (2014-09-19)
* Jumping to current song in playlist doesn't trigger assertion failures anymore.
* Readline's completion feature is now properly disabled.

# ncmpcpp-0.6_beta3 (2014-09-14)
* Handling of server events has been improved.
* Readline now ignores escape key.
* Cropping a playlist now works only if its length is bigger than 1.
* Browser and media library sorting now works properly with filtering.
* Compilation with boost-1.56 has been fixed.
* Configuration option visualizer_type now accepts "spectrum" value.
* Configuration option autocenter_mode is now properly read.
* Configuration option visualizer_sample_multiplier now accepts all positive numbers.
* Documentation was updated to reflect the changes.
* Mouse event handling in media library now works properly.
* ncmpcpp now works properly with password protected server.
* Data fetching delay in media library and playlist editor is now optional.

# ncmpcpp-0.6_beta2 (2014-09-03)
* Fixed the issue that prevented MPD flags toggling from working properly.
* Song change event is now properly handled after song ends with consume flag active.
* ncmpcpp now operates in raw terminal mode and ignores SIGINT.
* Separators between columns in media library, playlist editor and tag editor are now reliably colored.
* Build failures due to the missing include are now fixed.
* Playlist cropping now works as expected if no item is selected.

# ncmpcpp-0.6_beta1 (2014-09-03)
* Support for polling communication mode has been removed. Additionally, MPD version >= 0.16 is required.
* Keybinding system has been redesigned: "keys" file is now deprecated; "bindings" file is used instead. It has support for many features, including binding to non-ascii characters and macros.
* Support for readline library was added.
* Browser has support for custom sort format.
* Playlist has support for MPD priorities.
* Media library can be sorted by modification time.
* Visualizer's frequency spectrum was adjusted to look more pleasant.
* Broken compatibility of various actions with filtered lists has been corrected.
* Tons of other small changes and improvements were made.

# ncmpcpp-0.5.10 (2012-04-01)
* fix compilation with gcc 4.7
* fix building without curl support

# ncmpcpp-0.5.9 (2012-03-17)
* new feature: support for fetching lyrics for currently playing song in background
* new feature: support for stereo visualization
* new feature: support for merging screens together
* playlist editor: add support for columns display mode
* settings: provide a way to use alternative location for configuration file
* settings: make characters used in visualizer customizable
* lyrics fetcher: add support for lololyrics.com
* various bugfixes

# ncmpcpp-0.5.8 (2011-10-11)
* lyrics fetcher: add support for justsomelyrics.com
* add command line switch -s/--screen for switching screen at startup
* add proper support for asx/cue/m3u/pls/xspf playlists
* bugfixes

# ncmpcpp-0.5.7 (2011-04-21)
* convert input path to utf8 before calling `add` command
* extras: add program that copies Artist to AlbumArtist for mp3/ogg/flac files
* lyrics fetcher: add support for lyricsvip.com
* lyrics fetcher: skip lyricwiki if there's licence restriction on lyrics
* tag editor: do not convert filenames back to utf8 while reading files using taglib
* search engine: fix error occuring while trying to select first album in results

# ncmpcpp-0.5.6 (2011-01-02)
* settings: make displaying `empty tag` entry optional
* bugfixes

# ncmpcpp-0.5.5 (2010-08-04)
* new feature: select album around cursor
* new feature: `replay` function
* fixed feature: display separators between albums
* playlist: support for adding last.fm streams
* browser: support for operations on m3u playlists not created by MPD
* browser: support for deleting group of selected items
* media library: show songs with primary tag unspecified
* lyrics: new fetching system
* lyrics: support for storing lyrics in song's directory
* artist info: support for preffered language
* settings: support for custom visualization color
* settings: support for `empty` part of progressbar in progressbar_look
* settings: new config option: titles_visibility
* song format: support for limiting maximal length of a tag
* a lot of minor fixes

# ncmpcpp-0.5.4 (2010-06-03)
* new feature: toggle bitrate visibility at runtime
* new feature: locate song in tag editor
* new feature: separators between albums in playlist
* new feature: customizable columns' names
* new feature: support for multiple tag types in one column
* media library: support for "All tracks" option in middlle column
* playlist: shorten units' names displayed in statusbar
* browser: critical bugfix in feature "remove directory physically from hdd"
* a few other bugfixes

# ncmpcpp-0.5.3 (2010-04-04)
* new feature: new movement keys: {Up,Down}{Album,Artist}
* new feature: make displaying volume level in statusbar optional
* new feature: jump from browser to playlist editor
* bugfixes

# ncmpcpp-0.5.2 (2010-02-26)
* bugfixes

# ncmpcpp-0.5.1 (2010-02-15)
* new feature: customizable startup screen
* new feature: locate song in media library
* new feature: support for built-in mpd searching in search engine
* new feature: support for adding random artists/albums to playlist
* new feature: support for album artist tag
* new feature: support for switching between user-defined sequence of screens
* new feature: discard column colors if item is selected (optional)
* new feature: support for adding tracks to playlist after highlighted item
* make displaying dates of albums in media library optional
* several bugfixes

# ncmpcpp-0.5 (2009-12-31)
* bash completion support
* libmpdclient2 support
* "idle" command support
* new screen: mpd server info
* new feature: sort songs in browser by mtime
* new feature: lyrics "refreshing"
* new feature: toggle replay gain mode
* new feature: support for centered cursor
* new feature: add selected items to playlist at given position
* playlist: support for range sort/reverse
* tag editor: support for numerating tracks using xx/xx format
* a lot of minor fixes

# ncmpcpp-0.4.1 (2009-10-03)
* support for writing performer, composer and disc tags into ogg and flac files
* support for custom prefix/suffix of now playing song
* support for system charset encoding autodetection
* support for underlined text
* optimizations and bugfixes

# ncmpcpp-0.4 (2009-09-09)
* new screen: music visualizer with sound wave/frequency spectrum modes
* new feature: alternative user interface
* new feature: command line switch for displaying now playing song
* new feature: support for fixed size and/or right aligned columns
* new feature: display bitrate of current song in statusbar (optional)
* new feature: display remaining time of current song instead of elapsed (optional)
* new feature: jump to now playing song at start (enabled by default)
* new feature: additional attributes for columns
* song format: support for nested braces
* support for colors in song_status_format
* customizable progressbar look
* mouse support for additional windows
* optional user prompting before clearing main playlist
* track and year tag are not limited to numbers anymore if mp3 files are edited
* marker for full filename (F) has been replaced with directory (D)
* a lot of other minor improvements and bugfixes

# ncmpcpp-0.3.5 (2009-06-24)
* new feature: custom command execution on song change
* new feature: allow for physical files and directories deletion
* new feature: add local directories recursively
* new feature: add random songs to playlist
* new feature: mouse support
* new screen: outputs
* text scrolling in header was made optional
* some bugfixes

# ncmpcpp-0.3.4 (2009-05-19)
* new feature: search in help, lyrics and info screen
* new feature: two columns view in media library
* new feature: input box history
* make displaying hidden files in local browser optional
* bugfixes

# ncmpcpp-0.3.3 (2009-04-02)
* new feature: cyclic scrolling
* support for single mode (requires >=mpd-0.15_alpha*)
* support for consume mode (requires >=mpd-0.15_alpha*)
* support for pdcurses library
* support for WIN32
* improvements and bugfixes

# ncmpcpp-0.3.2 (2009-03-17)
* new feature: locate currently playing song in browser
* new feature: stop playing after current song
* new feature: move item(s) in playlist to actual cursor's position
* new feature: reverse playlist's order
* support for external console editor
* support for multiple composer, performer and disc tags
* support for basic and extended regular expressions
* make blocking search constraints after succesful searching optional
* improved searching in screens
* minor improvements and bugfixes

# ncmpcpp-0.3.1 (2009-02-23)
* support for columns in browser and search engine
* support for lyricsplugin database
* support for any tag in search engine
* support for ignoring leading "the" word while sorting (optional)
* new feature: apply filter to screen (Ctrl-F by default)
* new feature: playlist sorting (Ctrl-V by default)
* new feature: open lyrics in external editor
* other minor improvements and bugfixes

# ncmpcpp-0.3 (2009-01-22)
* general core rewrite
* new screen - clock
* support for asian wide characters
* support for non-utf8 encodings
* a lot of bugfixes

# ncmpcpp-0.2.5 (2008-12-05)
* support for unix domain sockets
* support for adding to playlist files outside mpd music directory (requires mpd >= 0.14_alpha*)
* support for browsing local directories tree (requires mpd >= 0.14_alpha*)
* support for searching in current playlist
* many bugfixes

# ncmpcpp-0.2.4 (2008-10-12)
* fixed bug with not null terminated strings on mac os x
* support for renaming files in tiny tag editor
* support for editing composer, performer and disc tag in mp3 files
* support for user defined base seek time
* support for user defined tag type in left column of media library
* support for user defined display format in album column of media library
* support for basic mpc commands and command line arguments
* adding items to playlist can be done like in ncmpc
* fetching lyrics and artist's info doesn't lock ncmpcpp anymore
* changed place of storing config files
* list of recently used patterns in tag editor is now remembered

# ncmpcpp-0.2.3 (2008-09-20)
* new screen - complex tag editor (with albums/directories view)
* brand new song info screen (old one removed)
* support for renaming files and directories
* support for reading tags from filename
* support for editing artist and album in media library
* support for playlist renaming
* support for following lyrics of now playing song
* support for fetching artist's info from last.fm
* fixed compilation for Mac OS X and *BSD
* fixed compilation for older gcc versions
* extended configuration (e.g. all colors can be user-defined)
* `repeat one song` mode works with random mode now
* incremental seeking (old behaviour is still available through config)
* a bunch of fixes and improvements

# ncmpcpp-0.2.2 (2008-09-05)
* new screen - playlist editor
* new playlist view - columns
* playlist view switcher added (key 'p' by default)
* find function modes added (wrapped/normal, switch is 'w' by default)
* albums in media library sorted by year (and it's also displayed)
* multiple items selection support and related functions added
* playlists management support
* new function - "go to dir containing selected song" (key 'G' by default)
* moving items improved
* lots of minor fixes

# ncmpcpp-0.2.1 (2008-08-27)
* support for composer, performer and disc tag
* customizable keybindings
* "add" option added
* example config files are installed automatically now
* "repeat one song" mode added (works only if ncmpcpp is running)
* minor fixes, improvements etc.

# ncmpcpp-0.2 (2008-08-20)
* libmpd dependency dropped
* pkgconfig is not needed anymore
* ncmpcpp now shows more info if it cannot connect to mpd
* proper handling for mpd password added
* if ncmpcpp lose connection to mpd it'll try to reconnect
* playlist status added
* new screen - lyrics
* switching between playlist and browser with tab key added
* alternate movement keys added (j and k keys)
* auto center mode added
* new option - crossfade can be set to any value now (X key)
* new option - going to parent directory in browser using backspace key
* issue with backspace key fixed
* sorting items in browser is case insensitive now
* many fixes and improvements

# ncmpcpp-0.1.2 (2008-08-12)
* parts of interface are hideable now
* new screen - media library
* new option - crop (it removes all songs from playlist except the playing one)
* many fixes and optimizations

# ncmpcpp-0.1.1 (2008-08-07)
* add example configuration file
* configure.in now works as expected
* taglib dependency is optional now
* more customizable options
