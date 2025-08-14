## Dependencies

ckmame is written in C++20 and uses [cmake](https://cmake.org) to build.

To use ckmame, you need
- [zlib](http://www.zlib.net/) (at least version 1.1.2)
- [libzip](https://libzip.org/) (at least version 1.10.0)
- [SQLite3](https://www.sqlite.org/)
- optionally [libxml2](http://xmlsoft.org/) (for M.A.M.E. -listxml and detectors)
- optionally [libarchive](https://www.libarchive.org/) (for reading from 7z archives)

For running the tests, you need to have [nihtest](https://nih.at/nihtest/) (at least version 1.9.1) and [Python](https://python.org).

For code coverage of the test suite, you need to have [lcov](https://github.com/linux-test-project/lcov) installed.


## Building ckmame

The basic usage is
```sh
mkdir build
cd build
cmake ..
make
make test
make install
```

Some useful parameters you can pass to `cmake` with `-Dparameter=value`:

- `CMAKE_INSTALL_PREFIX`: for setting the installation path
- `DOCUMENTATION_FORMAT`: choose one of 'man', 'mdoc', and 'html' for
  the installed documentation (default: decided by cmake depending on
  available tools)

You can get verbose build output with by passing `VERBOSE=1` to `make`.

You can also check the [cmake FAQ](https://cmake.org/Wiki/CMake_FAQ).


## Test Suite Code Coverage

To enable collecting code coverage, pass `-DENABLE_COVERAGE=ON` to `cmake`. After running the tests with `make test`, run `make coverage` to create the report in `coverage/index.html`.

Please note that this builds ckmame with coverage gathering enabled. You should not use such a build in production.
