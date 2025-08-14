3.1 (Unreleased)

* Improve speed on slow file systems.
* By default, don't update RomDB if dat is empty, can be overridden by `allow-empty-dat` configuration option.

3.0 (2025-01-20)
================

* Keep results of last runs, use `ckstatus` to query state of ROM set. The `ckmame` command line options duplicating this functionality (`--complete-list`, `--missing-list`, and `--report-changes`) are deprecated.
* Add support for SHA256 and mia flag for ROMs and disks.
* Don't match any files for ROMs or disks with no hashes.
* When only keeping complete games, minimize searching when a ROM is missing.

2.1 (2024-02-23)
================

* Add `--report-changes` to show changes between last and current run.
* Add `--delete-unknown-pattern` to remove unknown files matching a pattern.
* Switch to nihtest.

2.0 (2022-05-31)
=================
* Support for configuration file and multiple sets.
* Overhaul command line options.
* Automatically update ROM DB.
* Speed up ROM sets with many files shared among many games.
* Improve parse error reporting, don't create mamedb for dats with errors, fix inconsistencies in dat.
* Improve detector support.

1.1 (2021-06-28)
=================
* Read-only 7z support.
* Improve detector support.
* Bump mamedb version. All mame.db files need to be regenerated.
* Follow MAME in expecting disk images in subdirectories.
* Remove broken support for checking samples.
* Convert to C++17.

1.0 (2018-12-18)
=================

* add `--stats` to print stats about the state of the ROM set
* add support to only keep complete games in ROM set
* add `--autofixdat` to create fixdat with a name based on the dat name
* improve MAME (neogeo set) support
* fix up regression test suite
* switched to CMake build system

0.12 (2015-05-03)
=================

* add support for on-disk caches to speed up consecutive runs
* support for ROMs in directories instead of in zip archives
* fix all fixable ROM errors automatically in one run
* speed up database operations by preparing statements only once
* remove torrentzip support (removed in libzip)

0.11 (2011-02-20)
=================

* MESS Software Lists support
* retire `mkmamedb-xmame.sh`, use `mame -listxml | mkmamedb` instead
* link/copy disks from extra if `-j` (`--delete-found`) isn't specified
* `ckmame`: display current game on `SIGINFO` (`CTRL-T`)
* removed `ROMPATH` support since mame uses a path in the config file nowadays

0.10 (2008-07-25)
=================

* use SQLite3 instead of Berkeley DB
* optionally TorrentZip ROM set
* `mkmamedb`: add support for ROM Management Datafile format
* `mkmamedb`: add support to read dat files from zip archives
* `mkmamedb`: extract version and game description from `-listxml` output
* `ckmame`: fix finding ROMs that need detectors from superfluous and extra
* change to 3-clause BSD license

0.9 (2007-06-04)
================

* add support for CMPro XML header skip detectors
* fix handling of zero byte ROMs
* `dumpgame`: brief option: no ROM/disk info
* `mkmamedb`: add support for reading Romcenter dat files
* `mkmamedb`: warn about multiple games with same name
* `mkmamedb`: create CMPro dat files
* `mkmamedb`: create mame db or CMPro dat file from zip archives
* `ckmame`: add option to keep files present in old ROM database

0.8 (2006-05-18)
================

* clean up superfluous files and extra directories
* use additional DB listing files that exist elsewhere
* read games to check from file
* create DB from multiple dat files
* omit games matching shell glob patterns from database
* don't open archives or disk images multiple times
* don't accept disk images that need a parent
* move unknown files to directory `unknown`, not to `garbage` in ROM path

0.7 (2005-12-28)
================

* fix all fixable errors in at most two runs
* rename disks
* support searching for files in additional directories
* detect faked ROMs (correct CRC, wrong MD5/SHA1)
* adapt `mkmamedb-xmame.sh` for MAME version numbers of 0.100 and above
* dropped support for inferring flags/status from CRC (old MAME versions)
* deprecated `-u`/`-U` command line options
* simplify ROM matching logic
* many new regression tests, causing
* various bug fixes

0.6 (2005-06-17)
================

* libzip is distributed separately
* Support MAME/MESS `-listxml` output
* Adapt for MAME `-listinfo` output changes (honor flags baddump and nodump).
* Check disk images (chd files).
* Superfluous merged into `ckmame`. `ckmame` now by default reports
  extra files.  Also works for samples.
* add file format version to database and don't use incompatible database
* `dumpgame`: find game by checksum of one of its ROMs
* add script to create database by running xmame

0.5 (2003-01-30)
================

* MESS, Impact, Raine, and others based on the CMPro format now supported.
* Man page added.
* Bug fixes.

0.4.1 (1999-10-06)
==================

* If a a clone zip didn't exist, it was not created.
* The DOS port now really can fix romsets.

0.4 (1999-09-23)
================

* The whole program.
