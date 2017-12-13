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