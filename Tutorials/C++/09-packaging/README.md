# Step 09 — Packaging and installation (C++)

**Level 4 · Advanced · ~40 minutes**

The application of step 08 runs from `build/`. Nobody can install it. This step
turns it into a `.deb` a system can install, and an archive that works
everywhere — with **CPack**, which is CMake's packaging tool.

---

## What you will learn

| Piece | What it does |
|---|---|
| `install(TARGETS ...)` | puts the program in `bin/` |
| `install(FILES ...)` | puts the interface file and the `.desktop` entry in `share/` |
| `include(GNUInstallDirs)` | gives `CMAKE_INSTALL_BINDIR`, `DATADIR`... the conventional folders |
| `CPACK_*` variables | name, version, contact, description of the package |
| `CPACK_GENERATOR` | `TGZ`, `DEB`, `External` (used for the AppImage) |
| `CPACK_DEBIAN_*` | the `.deb` metadata; `SHLIBDEPS` reads the libraries the binary links |

---

## Why this is a separate project

This folder builds **step 08's source** — it has no `main.cpp` of its own:

```cmake
set(NOTES_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../08-capstone-notes")
add_executable(notes "${NOTES_SOURCE}/main.cpp")
```

Packaging is a build concern, not part of the program: the same source is built
by the course (for running and testing) and by this project (for installing).
If you copied step 08 into a real application, this `CMakeLists.txt` is the file
you would keep.

---

## The one problem packaging always hits: where is my data?

Step 08 reads `notes.xml` from a folder compiled into the binary:

```cpp
#ifndef RAVEN_TUTORIAL_DIR
#define RAVEN_TUTORIAL_DIR "."
#endif
...
const char* xml = std::getenv("NOTES_XML");
if (!xml) xml = RAVEN_TUTORIAL_DIR "/notes.xml";
```

A build tree can use `"."`. An **installed** program cannot: its working
directory is the user's. So this project compiles the path the system will have:

```cmake
target_compile_definitions(notes PRIVATE
    RAVEN_TUTORIAL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/notes")
```

and installs the file exactly there:

```cmake
install(FILES "${NOTES_SOURCE}/notes.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
```

`CMAKE_INSTALL_FULL_DATADIR` follows the prefix, which is why the packaging run
configures with **`-DCMAKE_INSTALL_PREFIX=/usr`** — `/usr/share/notes` is what a
`.deb` must use. `NOTES_XML` overrides it, which is how you run the build-tree
binary before installing.

---

## The CMakeLists, part by part

```cmake
project(notes VERSION 1.0.0 LANGUAGES CXX)     # the version becomes the package version
include(GNUInstallDirs)
find_package(raven REQUIRED)

add_executable(notes "${NOTES_SOURCE}/main.cpp")
target_link_libraries(notes PRIVATE raven::raven)
target_compile_definitions(notes PRIVATE
    RAVEN_TUTORIAL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/notes")

install(TARGETS notes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${NOTES_SOURCE}/notes.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES notes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
```

and the package description:

```cmake
set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "notes")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Raven tutorials <you@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()
include(CPack)          # always last: it writes CPackConfig.cmake
```

`SHLIBDEPS ON` is what turns "the program needs libc and X11" into
`Depends: libc6, libx11-6` without listing them by hand.

---

## Build the package

```bash
cd "Tutorials/C++/09-packaging"
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
cd build
cpack -G DEB          # the .deb
cpack -G TGZ          # the archive (or plain `cpack` for both)
```

Expected:

```
CPack: - package: .../build/notes_1.0.0_amd64.deb generated.
```

Check what it contains without installing:

```bash
dpkg-deb -f notes_1.0.0_amd64.deb Package Version Depends
dpkg-deb -c notes_1.0.0_amd64.deb
```

`c` lists the files, and they should be exactly what the `install()` lines say:

```
./usr/bin/notes
./usr/share/applications/notes.desktop
./usr/share/notes/notes.xml
```

---

## Install it

```bash
sudo apt install ./notes_1.0.0_amd64.deb
```

Use `apt`, not `dpkg -i`: apt resolves the dependencies CPack wrote.

Verify:

```bash
which notes                       # /usr/bin/notes
notes --test                      # the same self-check as step 08, installed
dpkg -L notes                     # every file the package owns
desktop-file-validate /usr/share/applications/notes.desktop
```

`notes` also appears in the application menu, and opens files dropped on it.

Remove it:

```bash
sudo apt remove notes
```

---

## The archive, and staging an install

`cpack -G TGZ` produces `notes-1.0.0-Linux.tar.gz`, the same tree without a
package manager. Useful for a `$HOME` install:

```bash
tar xzf notes-1.0.0-Linux.tar.gz -C /tmp/notes-1.0.0
/tmp/notes-1.0.0/usr/bin/notes            # looks for /usr/share/notes/notes.xml!
```

That last line fails on a machine where the `.deb` is not installed — the
compiled path still says `/usr/share/notes`. Two honest ways out:

```bash
NOTES_XML=/tmp/notes-1.0.0/usr/share/notes/notes.xml /tmp/notes-1.0.0/usr/bin/notes
```

or compile with the prefix you really install to
(`-DCMAKE_INSTALL_PREFIX=$HOME/.local`) and the archive is self-consistent.

For an install you do not want to see yet, stage it:

```bash
DESTDIR=/tmp/stage cmake --install build
find /tmp/stage -type f
```

`DESTDIR` is what package builders use: the files land under `/tmp/stage/usr/...`
while the compiled paths stay `/usr/...`. `cpack` does exactly this internally.

---

## Where the files go, and why

| Path | Content | Standard |
|---|---|---|
| `/usr/bin/notes` | the program | executables |
| `/usr/share/notes/notes.xml` | the interface | read-only application data |
| `/usr/share/applications/notes.desktop` | menu entry | desktop integration |

Versions and settings do **not** belong in `/usr`: an application writes to the
user's configuration folder (Raven has `Rshell_*` helpers and the dialog APIs
for that; step 08's "unsaved changes" guard is where a real one would save).

---

## Checkpoint

* Why can the installed program not use `"."` for `notes.xml`?
* What does `include(CPack)` write, and where?
* What does `CPACK_DEBIAN_PACKAGE_SHLIBDEPS` do that a hand-written `Depends:`
  does not?
* What is the difference between `DESTDIR` and `--prefix`?

---

## Exercises

1. Add `install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})` — where
   does the file end up?
2. Bump `VERSION` to `1.1.0` and repackage. Which two places changed?
3. Add a `notes.1` man page with `install(FILES notes.1 DESTINATION
   ${CMAKE_INSTALL_MANDIR}/man1)` and check `man notes` after installing.
4. Add a second package with **components** (`CPACK_COMPONENTS_ALL`) so that the
   program and the documentation install separately.
5. **AppImage.** Raven's own `cmake/AppImage.cmake` uses the CPack *External*
   generator to turn an install tree into one file. Read it and add an
   `External` generator here.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `Cannot find source file ../08-capstone-notes/main.cpp` | The step was copied without its neighbour. Keep the course layout. |
| The installed `notes` cannot find `notes.xml` | The build was configured with a prefix other than `/usr`. Reconfigure with `-DCMAKE_INSTALL_PREFIX=/usr`, or set `NOTES_XML`. |
| `dpkg-shlibdeps: error: no dependency information found` | The binary links a library that is not installed from a package. Link Raven statically (the default) or package the library too. |
| `cpack: command not found` | CPack ships with CMake; `include(CPack)` must run before the build. |
| `apt install` complains about an older version | `sudo apt install ./notes_1.0.0_amd64.deb` replaces it; the package name is the same. |
| The menu entry does nothing | `Exec=notes %F` needs `notes` on `PATH`; `desktop-file-validate` shows errors. |

**Previous:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md) ·
**Back to the course index:** [README](../README.md)
