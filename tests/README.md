# Engine and native logic checks

Requires CMake 3.16+, a C++14 compiler, and Python 3. No third-party test framework is needed. Assertions stay enabled in Release builds.

```sh
cmake -S tests -B build/checks -DCMAKE_BUILD_TYPE=Debug -DOPENKEY_SANITIZERS=ON
cmake --build build/checks --parallel 2
ctest --test-dir build/checks --output-on-failure
```

On Windows, omit sanitizers and pass `-C Debug` to CTest for a Visual Studio build.

The tests check expected text for Telex, simple Telex and VNI, all five tones, both orthographies, restoration, backspacing, long history, capitalization, quick consonants, macros and English-mode macros. They also cover truncated/oversized settings, macro import/export, encoding and case conversion, concurrent conversion, and the reverse lookup against every populated encoding-table entry. Native sender tests compile the actual production function bodies against OS mocks and verify output, chunk boundaries, surrogate pairs, maximum-length macros, restore keys and failed clipboard writes. On Windows, installer tests use temporary files to exercise validation, replacement and rollback.

## Coverage

```sh
cmake -S tests -B build/coverage -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DOPENKEY_COVERAGE=ON
cmake --build build/coverage --parallel 2
python3 tests/Coverage.py build/coverage
```

LLVM's `llvm-cov` and `llvm-profdata` must match the compiler. On macOS the script also finds tools through `xcrun`. On Windows run in a Visual Studio developer environment with LLVM on PATH.

The script deletes stale profiles, runs the tests, and generates `build/coverage/coverage/{summary.txt,coverage.json,html/}`. CI requires **strictly more than 80% executable-line coverage overall and for each measured production file**. The denominator includes every engine `.cpp`, `CharacterOutput.h`, both extracted native sender bodies (`Senders.inc`), and, on Windows, `UpdateInstaller.h`. No engine functions are excluded. Regions and branches are reported separately; the 80% requirement is for lines. UI, OS hooks, clipboard OS calls and updater download/process orchestration are outside this unit-test metric and still require native integration testing. This is not whole-application coverage.

## Performance

```sh
cmake -S tests -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --config Release --parallel 2
./build/release/engine_benchmark
```

The fixed benchmark replays 570,000 engine events and converts 540 KB of ASCII. Keep sanitizers and coverage instrumentation off when measuring. These are microbenchmarks, not end-to-end typing latency.

## Native checks before a release

Build the native macOS app and both Windows architectures (CI does this), then manually check typing/backspacing and string mode in relevant applications, Spotlight/browser compatibility, switching IMEs, clipboard conversion while the clipboard changes, and an update from a disposable Windows installation. Automated sender tests do not inject keystrokes or verify destination-application behavior. Typing history remains unlimited within an uninterrupted session so prior words can still be restored; the allocation changes do not cap memory growth.
