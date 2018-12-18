This is ckmame, a program to check ROM sets for
[MAME](https://www.mamedev.org/), the Multiple Arcade Machine Emulator. It
tells you which ROM files are missing or have a wrong checksum, and
can delete unknown and unused files from the ROM sets, and rename or
move ROM files.

See the [INSTALL.md](INSTALL.md) file for installation instructions and
dependencies.

You will also need a description of the ROM set you want to check. For
MAME, it can be obtained by running `mame -listxml`.

If you make a binary distribution, please include a pointer to the
distribution site
>	https://nih.at/ckmame/

The latest version can always be found there.

Mail suggestions and bug reports to <ckmame@nih.at>.

[![Travis Build Status](https://travis-ci.com/nih-at/ckmame.svg?branch=master)](https://travis-ci.com/nih-at/ckmame)
[![Coverity Status](https://scan.coverity.com/projects/14647/badge.svg)](https://scan.coverity.com/projects/nih-at-ckmame)
