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

```bash
cmake --preset linux-release
cmake --build build/linux-release
```

For Windows, use `windows-release` instead (cross-compiles with MinGW). Debug presets: `linux-debug`, `windows-debug`.

The build output and a copy of `Assets/` end up in `out/<Platform>-RELEASE/bin/`.

## Running

```bash
./out/Linux-RELEASE/bin/SD2Terminal
```

On Windows, run `SD2Terminal.exe` from the same folder.

Options: `--deck ID`, `--seed N`, `--assets DIR`, `--fights N`, `--discard`, `--auto`, `--quiet`.
