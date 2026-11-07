# SD2

A card duel game for the SD2 assignment, built on ExcalEngine.

## Requirements

- CMake 3.25+, Ninja and a C++23 compiler (GCC 14+)
- For Windows builds from Linux: `mingw-w64`

## Getting the code

```bash
git clone --recursive https://github.com/ExcalixY/SD2
```

Already cloned without `--recursive`? Run `git submodule update --init --recursive`.

## Building

The build output and a copy of `Assets/` end up in `out/<Platform>-<BUILD_TYPE>/bin/`.

### Linux

```bash
cmake --preset linux-release
cmake --build build/linux-release
```

Use `linux-debug` for a debug build with sanitizers.

### Windows

The Windows build is cross-compiled from Linux with MinGW (`mingw-w64` must be installed).

```bash
cmake --preset windows-release
cmake --build build/windows-release
```

Use `windows-debug` for a debug build. The resulting `SD2Terminal.exe` is standalone and runs on Windows 10+.

## Running

```bash
./out/Linux-RELEASE/bin/SD2Terminal
```

On Windows, run `SD2Terminal.exe` from the same folder.

Options: `--deck ID`, `--seed N`, `--assets DIR`, `--fights N`, `--discard`, `--auto`, `--quiet`.
