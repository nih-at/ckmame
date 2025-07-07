# ckmame
## Check ROM sets and dat files for MAME and other emulators

## What Is ckmame?

ckmame is a command line tool to check and fix ROM sets. It tells you which ROM files are missing or wrong, and
can delete unknown and unused files. It also handles renamed games or ROMs and can find missing files in other directories.

## Why Use ckmame?

ckmame has been continuously developed since 1999. It is efficient and flexible. It is usable on Linux, macOS, and probably Windows.

## Getting Started

For building and installing ckmame from source, see the [INSTALL.md](INSTALL.md) file.

A list of available binary packages can be found on [Repology](https://repology.org/project/ckmame/versions).

## Using ckmame

Where to find ROMs and dat files and various options can be set in a [configuration file](https://nih.at/ckmame/ckmamerc.html) and overridden on the [command line](https://nih.at/ckmame/ckmame.html).

To speed up operations, ckmame creates an SQLite database from the dat file. This can either be done by hand [mkmamedb](https://nih.at/ckmame/mkmamedb.html) or ckmame can do it automatically if the dat file is specified in the configuration file.

To get the ROM set in its best possible state, run `ckmame --fix`.

THe state of the ROM set is recorded in another database, which can be queried using [ckstatus](https://nih.at/ckmame/ckstatus.html).
## Staying in Touch

More information and the latest version can always be found on [nih.at](https://nih.at/ckmame).
The official repository is at [GitHub](https://github.com/nih-at/ckmame/).

If you want to reach the authors in private, use <ckmame@nih.at>.


[![Github Actions Build Status](https://github.com/nih-at/ckmame/workflows/build/badge.svg)](https://github.com/nih-at/ckmame/actions?query=workflow%3Abuild)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/ktyebjukjnuqf4fb?svg=true)](https://ci.appveyor.com/project/nih-at/ckmame)
[![Coverity Status](https://scan.coverity.com/projects/14647/badge.svg)](https://scan.coverity.com/projects/nih-at-ckmame)
