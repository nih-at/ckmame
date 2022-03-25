## config

### Implement

- Remember ROM set state between runs, option to display changes (something like -3 +2 missing games)  
- Fix failing tests.
- Check parsers: what should be considered a warning and what an error (causing database creation to fail).
- Parser: header is not set if dat contains no games.
- Add error messages for detector parse errors.
- Variables in config file (e.g. for collection root directory).
- Make `SIGINFO` handler print set name.
- Add (per entry) option for directories to put cache database in central location.
- Auto-fix cue sheet when renaming track 1.

### Write Tests

- `use-central-cache-directory` for dat-directories and extra-directories
- multi-set invocation for mkmamedb
- Allow `--set` to be specified multiple times, run command for each one.
- Allow glob pattern as argument to `--set` option.
- Add option to run command once for each set.
- configuration option `use-description-as-name`
- per dat configuration option `game-name-suffix`
- per dat and extra-directory configuration options
- configuration option `report-no-good-dump`
- configuration option `needed-directory`
- configuration option `saved-directory`
- Better check for determining input file type.


## other

- Add test for `mkmamedb -F cm`.

- Make `SIGINFO` handler more responsive.

- handle multiple writers to ckmamedb
- search loose files in zipped mode.

- exceptions and error messages:
    - who creates which part of the error messages
    - catch exceptions in main and print errors (done in ckmame and mkmamedb).
    - clean up Exceptions without text
    
- When checking single game, search for its ROMs everywhere and report any matches.

- rar read support

- add option to keep ROMs with detector applied

- bug: when creating a fixdat and re-checks happen, games end up in the fixdat multiple times

- fixdat-missing-all: two copies of the same file (romof) in fixdat; depends on order of games in datfile

- C++ cleanups:
  - move lineno into ParserSource
  - add tests for all tokens in all parser backends:
    - cm
    - rc
    - xml

- when committing to garbage fails because source archive is broken, move source archive out of the way.

# Later

* use CkmameDB as old db, specify all detectors you'll ever need when creating.

* convert runtest to use ziptool instead of unzip

* `mkmamedb`: analyze speed, make it faster

* `mkmamedb`: use/update `.ckmame.db` files

* some runs use roms from `extra`, but then mark them as "not used", find out why

* instead of looking in cloneof for roms, move all extra roms to `needed`

* clean up archives in `.ckmame.db` for manually removed dirs/zips

* fix `.ckmame.db` support for roms with headers: store hashes with
  detector applied in `.ckmame.db` and adapt:
  - `regress/rom-from-extra-detector.test`
  - `regress/rom-from-superfluous-detector.test`
  - `regress/skip-full.test`
  - `regress/skip.test`
  - `src/dbh_cache.c`

* uncompressed support
** [bug] existing no good rom is not accepted in uncompressed mode (`nogood-diskgood.test`)

* bugs with test cases
! [bug] `mkmamedb`: three generation merge grandchild to child does not work (`mamedb-merge-parent.dump`)
+ [bug] improve ``zero-miss.test` diagnostics when creating zero size file
+ [bug] `mkmamedb`: warn if rom from parent has different crc (`mkmamedb-parent-crcdiff-mame.test`)
+ [bug] `mkmamedb`: merge does not match name in parent: warning and fixup (`mkmamedb-merge-wrong-name.test`)
+ [bug] `mkmamedb`: does not warn about duplicate roms of type nodump or duplicate samples (`mkmamedb-duplicate-nodump.test`)
- [bug] detector bugs (`detector-interaction-with-headerless.test`, `detector-prefer-file-with-header.test`)

* unsorted
+ support `^T` inside a game, not only between games (e.g. while searching extra dirs)
- `mkmamedb` doesn't handle chds (ignores for zipped, includes in games as rom for unzipped)
! [feature] reorder cleanup step when renaming files to remove the copies
  earlier; otherwise big renames on big sets don't work.
! [bug] cleanup reports file that were used this time as "not used"
! [bug] cleanup after using disks reports errors because disks were moved
  Example:
  ```
  ckmame: new/file.chd: cannot remove: No such file or directory
  In image new/file.chd:
  image new/file.chd: not used
  ```
+ [feature] `mkmamedb`: parser for mtree files
+ [feature] server mode: tell location/mamedb of ROM sets, serves files needed by remote `ckmame` client
+ [bug] check/fix database error reporting (pass on sqlite3 errors)
+ [feature] get needed files directly from parent (`inparent.test`)
+ [feature] inconsistent zip in ROM set: copy files to new zip
+ [feature] config file (provides defaults overridable on command line)
+ [feature] needed cleanup (automatically?)
+ [feature] `mkmamedb`: mark ROMs without checksums as `nogooddump`
+ [bug] `mkmamedb -F dat`:
  - write `&` unquoted (perhaps other meta characters as well)
  - write Umlaut wrong
  ```
    Entity: line 5986: parser error : Input is not proper UTF-8, indicate encoding !
    Bytes: 0x81 0x63 0x6B 0x73
                <rom name="Gl�cksrad.ipf" size="1049180" crc="008b8870" sha1="b1a4bba7c96ce0c2
                             ^
  ```
    Suggested solution: using `iconv` and `LC_CTYPE`, try transcribing invalid UTF-8 files to UTF-8
    if it fails or no `iconv` or `LC_CTYPE` available, replace invalid characters with `'?'`
- [cleanup] access db directly in `find_*`
- [cleanup] specify globally which parts of memdb/lists to fill/maintain
- [cleanup] rom: use flag to specify whether we know the size
- [bug] `dumpgame`: report real database version for /dat key
- [bug] check if needed/extra are different
- [bug] DB export: pass all dat entries to output backend
- [bug] DB export: export detector
- [bug] diagnostics (fix?): don't process disks if checking samples
- [feature] add hash-types option to `dumpgame`
- [feature] `mkmamedb`: split to original CM dat files + detector XML on export
- [feature] when file for nogooddump rom exists, check if needed elsewhere
- [feature] `mkmamedb`: add XML output format
- [feature] database consistency checks during `mkmamedb`
  - are all roms of one set included in one other set
  - are two sets the same, just different name
- [feature] `--cleanup-extra`: remove (unnecessary?) directories
- [feature] support multiple `-O` arguments
- [compatibility] Accept CRCs of length < 8 by zero-padding them (in front)
- [cleanup] refactor fixing code (one function per operation)
- [cleanup] handle archive refreshing in `archive.c`
- [cleanup] rename: file is part of zip archive, rom is part of game

other features:
- fixdat: if only checking child, ROM missing in parent is not in fixdat
- parse: add state checking to `parse-cm.c`
- parse: check for duplicate attributes
- `mkmamedb`: handle rom without crc
- `mkmamedb`: handle `size 0 crc -`
- `mkmamedb`: no error message for missing newline in last line
- `mkmamedb`: warn about sets without parent that use "merge" (`mamedb-merge-no-parent.dump`)
- complete raine support (multiple archive names: `archive ( name "64th_street" name "64street" ))`
- option to check if no good dumps are needed elsewhere

* code cleanups:
  - make `parse_cm` table driven
  - split `util*`, `funcs.h`
  - fix all TODOs

* tests:
- extend at least following tests to use md5/sha1 as well:
  - `delete-used-superfluous.test`
  - `needed-cleanup.test`
  - `punused.test`
  - `rom-broken-replace.test`
  - `rom-from-extra-child.test`
  - `rom-unused.test`
  - `swap-roms-2.test`
  - `rom-many.test`
  - `swap-roms.test`
- case differences (game name / archive / rom / file in archive)
- extra cleanup done when all games checked
- is superfluous file that needs detector cleaned up correctly?
- detector
  - header field
  - write to db
  - use on matching file
  - use on non-matching file
  - rule operations
  - [test] is superfluous file that needs detector cleaned up correctly?
- `dumpgame`
- `mkmamedb` tests
  - mtree output format
  - `/prog` from command line
  - `/prog` from file
  - broken input

* cachedb

** maybe:
- [feature] move database out of the way if it's an older version
- [test] unzipped: take ROM from top-level file in roms
- [test] run unzipped-`ckmamedb`-* for zip as well
- [cleanup] rename `dbh_cache` to `cachedb`
- [feature] compute all hashes when called with `-i`
- [test] unzipped, dir with name ending in `.zip`

** later
- [feature] save detector hashes to cachedb
- fix Xcode warnings
- [cleanup] make `ARCHIVE_IFL_MODIFIED` a bool member
- [test] fix preload on OS X
- [feature] if `.ckmame.db` can't be opened, move aside and create new

### Improve Detector support

- define unique id for each detector (e. g. checksum over all fields)
- in mame.db, store which dat uses which detector
- split ckmamedb file table in (archive_id, file_number, name) and (archive_id, file_number, detector_uuid, size, hashes)
- when entering archives in map, compute hashes for all needed detectors
- when searching in ckmamedb, pass in which detector to use (or none)
