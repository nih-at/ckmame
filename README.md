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

[![Github Actions Build Status](https://github.com/nih-at/ckmame/workflows/build/badge.svg)](https://github.com/nih-at/ckmame/actions?query=workflow%3Abuild)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/ktyebjukjnuqf4fb?svg=true)](https://ci.appveyor.com/project/nih-at/ckmame)
[![Coverity Status](https://scan.coverity.com/projects/14647/badge.svg)](https://scan.coverity.com/projects/nih-at-ckmame)
