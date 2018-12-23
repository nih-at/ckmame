ckmame uses [cmake](https://cmake.org) to build.

For running the tests, you need to have [perl](https://www.perl.org).

To use ckmame, you need
- [zlib](http://www.zlib.net/) (at least version 1.1.2)
- [libzip](https://libzip.org/) (at least version 1.0)
- [SQLite3](https://www.sqlite.org/)
- optionally [libxml2](http://xmlsoft.org/) (for M.A.M.E. -listxml and detectors)

For running the test suite, you also need to install [perl](https://www.perl.org/).

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
