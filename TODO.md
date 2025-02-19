# TODO

## Other

- Remove fixdat if no ROMs are missing.
- Fix rom in extra not found if in roms/.ckmame.db but not in roms/ (ckmamedb-file-removed.vtest)
- Fix failing tests.
- Empty directory in ArchiveDir is not cleaned up, which makes removing empty archive fail.
- status update: SIGINFO support in Archive::commit, via libzip progress callback
- search loose files in zipped mode: Add option in ArchiveDir to ignore zip files, but keep in .ckmame.db.
- speedup idea: when opening archives (in extra dirs) only compute hashes if we need them
- `mkmamedb`: When a game is in two dat files (identical name and ROMs), skip it from second (with warning).
- handle multiple writers to ckmamedb
- rar read support
- add option to keep ROMs with detector applied
- bug: when creating a fixdat and re-checks happen, games end up in the fixdat multiple times
- when committing to unknown fails because source archive is broken, move source archive out of the way.

# Later

- StatusDB tests
- use CkmameDB as old db
- `mkmamedb`: analyze speed, make it faster
- clean up archives in `.ckmame.db` for manually removed dirs/zips
- Variables in config file (e.g. for collection root directory).

## Other Features
- fixdat: if only checking child, ROM missing in parent is not in fixdat
- parse: add state checking to `parse-cm.c`
- parse: check for duplicate attributes
- `mkmamedb`: handle `size 0 crc -`
- `mkmamedb`: no error message for missing newline in last line
- complete raine support (multiple archive names: `archive ( name "64th_street" name "64street" ))`

## Code Cleanups
- make `parse_cm` table driven
- fix all TODOs
- Move `delete_unknown_pattern` to `DatOptions`.
- exceptions and error messages:
    - who creates which part of the error messages
    - catch exceptions in main and print errors (done in ckmame and mkmamedb).
    - clean up Exceptions without text
- C++ cleanups:
  - move lineno into ParserSource
  - add tests for all tokens in all parser backends:
    - cm
    - rc
    - xml

## Tests
- Remove mame.db from tests that don't need it.
- Only use mame.db files from inside the test directory.
- Add test for `mkmamedb -F cm`.
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
- test using roms from two different 'old' roms

## CacheDB

### maybe
- [feature] move database out of the way if it's an older version
- [test] unzipped: take ROM from top-level file in roms
- [test] run unzipped-`ckmamedb`-* for zip as well
- [cleanup] rename `dbh_cache` to `cachedb`
- [feature] compute all hashes when called with `-i`
- [test] unzipped, dir with name ending in `.zip`

### later
- [feature] save detector hashes to cachedb
- fix Xcode warnings
- [test] fix preload on OS X
- [feature] if `.ckmame.db` can't be opened, move aside and create new

# Unsorted/Old

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
+ [bug] check/fix database error reporting (pass on sqlite3 errors)
+ [feature] inconsistent zip in ROM set: copy files to new zip
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
- [cleanup] specify globally which parts of lists to fill/maintain
- [bug] check if needed/extra are different
- [bug] DB export: pass all dat entries to output backend
- [bug] DB export: export detector
- [bug] diagnostics (fix?): don't process disks if checking samples
- [feature] `mkmamedb`: split to original CM dat files + detector XML on export
- [feature] database consistency checks during `mkmamedb`
  - are all roms of one set included in one other set
  - are two sets the same, just different name
- [feature] `--cleanup-extra`: remove (unnecessary?) directories
- [feature] support multiple `-O` arguments
- [compatibility] Accept CRCs of length < 8 by zero-padding them (in front)
- [cleanup] refactor fixing code (one function per operation)
- [cleanup] handle archive refreshing in `archive.c`
- [cleanup] rename: file is part of zip archive, rom is part of game
