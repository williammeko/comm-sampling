# comm-sampling

A cross-platform (Windows / macOS / Linux) desktop tool that communicates with
devices over a serial port or TCP (client or server), matches incoming byte
streams into frames, converts matched bytes to decimal values, and plots the
result in real time.

## Features

- **Communication settings** — serial port (ports, baud, data bits, parity,
  stop bits), TCP client (host + port), or TCP server (interface + port,
  including `ANY-INTERFACE` = `0.0.0.0`). Configurable byte cache size
  (default 10000).
- **Sending** — hex byte input with recent history (newest first, deduplicated,
  max 50), one-shot send or loop with configurable interval.
- **Matchers** — add / remove / reorder matchers that run as a pipeline:
  - *Fixed-keyword-frame*: search a keyword (≤10 bytes) and take `length`
    (0–255) bytes starting at the keyword.
  - *Decimal-picker*: cut a bit range (0-based start bit + bit length) from a
    frame and pack it back to bytes.
  - Each matcher shows matched frames (recent N) and an optional
    **bits-to-decimal convert** panel (Integer / IEEE-754, Big/Little endian).
- **Line chart** — enabled when the last matcher's convert panel is on; plots
  the converted value over a rolling duration window with auto-scaled Y axis.

## Architecture (non-blocking UI)

```
I/O thread (serial / TCP client / TCP server)
   └─ bytesReceived() ── queued signal ──► GUI thread
                                            └─ ByteCache (bounded queue)
                                                 └─ 5 ms timer ──► MatcherPipeline
                                                                      └─ frames ──► result views
                                                                      └─ last value ──► line chart
```

- All channel I/O runs in a dedicated `QThread`; nothing touches the GUI thread
  from I/O.
- The matcher pipeline runs on the GUI thread in small batches drained by a
  5 ms timer. For very high sustained throughput the pipeline can be moved to a
  worker thread without changing the UI.

## Requirements

- Qt 6 (Core, Gui, Widgets, Network, SerialPort)
- CMake ≥ 3.21
- A C++17 compiler (MSVC / MinGW on Windows, Clang on macOS, GCC/Clang on Linux)

## Install Qt (command line)

```bash
# macOS / Linux
python3 -m pip install aqtinstall
aqt install-qt mac desktop 6.8.1 clang_64 -m qtserialport -O ~/Qt

# Windows (pick your arch: win64_msvc2022_64 or win64_mingw)
python -m pip install aqtinstall
aqt install-qt windows desktop 6.8.1 win64_msvc2022_64 -m qtserialport -O C:\Qt
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.1/<arch>
cmake --build build --config Release --parallel
```

> `CMAKE_PREFIX_PATH` points at the Qt you installed. Examples:
> - macOS: `~/Qt/6.8.1/macos`
> - Windows: `C:/Qt/6.8.1/msvc2022_64` (forward slashes in CMake)

## Run

```bash
# macOS / Linux
./build/comm-sampling

# Windows
build\Release\comm-sampling.exe
```

## Package (portable, no Qt install needed on target)

```bash
# Windows — copies Qt DLLs next to the .exe (runs by double-click)
windeployqt --release build/Release/comm-sampling.exe

# macOS — self-contained .app
macdeployqt build/comm-sampling.app -dmg

# Linux — AppImage
linuxdeploy --appdir AppDir -e build/comm-sampling -d ... -i ... --output appimage
```

## Project layout

```
src/
  common/        ByteCache, HexUtils, shared types
  communication/ serial, TCP client, TCP server channels + ChannelManager
  matchers/      Matcher base, pipeline, fixed-keyword, decimal-picker, converter
  sender/        DataSender (one-shot + loop, send history)
  ui/            MainWindow and the component widgets
```

## Troubleshooting — macOS 26 on Intel

This Mac runs macOS 26 on an Intel CPU, which hits a few toolchain quirks:

1. **Homebrew has no Intel bottles anymore.** `brew install ninja` builds from
   source and fails. Use the Unix Makefiles generator (the default), or obtain
   ninja another way. `brew install cmake` still works.
2. **Broken CommandLineTools libc++.** `/usr/bin/c++` can't find `<utility>`
   because `/Library/Developer/CommandLineTools/usr/include/c++/v1` is
   incomplete. Workaround (already in `.vscode/settings.json`):
   `-DCMAKE_CXX_FLAGS=-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1`.
   Proper fix: reinstall the Command Line Tools.
3. **Qt 6.8.1 references the removed AGL framework.** On macOS 26 linking fails
   with `framework 'AGL' not found`. Fixed by editing Qt's
   `lib/cmake/Qt6/FindWrapOpenGL.cmake` to drop the `-framework AGL` fallback
   (this app doesn't use OpenGL). Newer Qt versions may not need this.

## License note

Qt is LGPLv3 / GPL / commercial. Linking dynamically and shipping with
`windeployqt` / `macdeployqt` keeps you LGPL-compliant without extra effort.
Static linking under LGPL requires relinkable object files (or use GPL /
commercial).
