This is ckmame, a program to check ROM sets for
[MAME](http://mamedev.org), the Multiple Arcade Machine Emulator. It
tells you which ROM files are missing or have a wrong checksum, and
can delete unknown and unused files from the ROM sets, and rename or
move ROM files.

  To use it, you need
	- [zlib](http://www.zlib.net/) (at least version 1.1.2)
	- [libzip](https://libzip.org/) (at least version 1.0)
	- [SQLite3](https://www.sqlite.org/)
	- optionally [libxml2](http://xmlsoft.org/) (for M.A.M.E. -listxml and detectors)

You will also need a description of the ROM set you want to check. For
MAME, it can be obtained by running `mame -listxml`.

If you make a binary distribution, please include a pointer to the
distribution site
>	https://nih.at/ckmame/

The latest version can always be found there.

Mail suggestions and bug reports to <ckmame@nih.at>.
