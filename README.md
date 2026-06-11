# Greenline

## Running
Requires recent NVIDIA drivers and ncurses.
This program has only been tested on x86-64 Linux, though in theory it could be built
for ARM.

## Building
### Dependencies:
**NixOS:** Use the dev shell; it handles dependencies, and handles the NVML
library path on NixOS. Loads automatically with direnv, or run `nix develop`.

**Everyone else:** You need headers and libraries for NVML and ncurses.
pkg-config should figure out the rest.

### Development:
```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --build build --target analyze # Optional: run static analyzer
```
Development builds run with ASan and UBSan.

### Release:
For NixOS:
```sh
nix build
```

For legacy distros:
```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
