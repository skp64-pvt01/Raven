# Step 09 — Packaging and installation (Python)

**Level 4 · Advanced · ~35 minutes**

The application of step 08 runs as `python3 main.py`. Nobody can install that.
This step installs it where a system expects an application, and packs it with
**CPack**.

Python has its own packaging (wheels, `pip`), which is the right tool for a
*library*. For a **desktop application** — a launcher, a menu entry, a data
file, a system path — a CPack package is the usual answer on Linux, and it is
the same mechanism the C++ and Rust steps use.

---

## What you will learn

| Piece | What it does |
|---|---|
| `install(PROGRAMS notes ...)` | installs the launcher in `bin/` **with the executable bit** |
| `install(FILES main.py notes.xml ...)` | installs the application and its data in `share/` |
| `include(GNUInstallDirs)` | the conventional folders |
| `CPACK_*` / `CPACK_DEBIAN_*` | name, version, maintainer, `Depends:` |
| `CPACK_DEBIAN_PACKAGE_DEPENDS` | for an interpreted program, CPack cannot scan binaries: name the interpreter |

---

## Why a launcher

A `.deb` installs files; it does not make `main.py` runnable by name. The usual
shape is a small script in `/usr/bin` that starts the real file:

```sh
#!/bin/sh
# notes - starts the application installed in ../share/notes.
HERE=$(dirname "$(readlink -f "$0")")
exec python3 "$HERE/../share/notes/main.py" "$@"
```

`readlink -f` follows a symlink, so `/usr/local/bin/notes → /opt/notes/bin/notes`
still resolves. `"$@"` passes `--test`, the file to open, everything.

The application itself is installed next to its data:

```
/usr/share/notes/main.py
/usr/share/notes/notes.xml
```

Step 08 reads `notes.xml` from its own folder (`HERE`), so the two files must
stay together. `NOTES_XML` overrides the path when they do not.

---

## The CMakeLists

```cmake
project(notes VERSION 1.0.0 LANGUAGES NONE)     # nothing is compiled
include(GNUInstallDirs)

set(NOTES_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../08-capstone-notes")

install(PROGRAMS notes DESTINATION ${CMAKE_INSTALL_BINDIR})        # the launcher
install(FILES
    "${NOTES_SOURCE}/main.py"
    "${NOTES_SOURCE}/notes.xml"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES notes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "notes")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Raven tutorials <you@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3")
endif()
include(CPack)
```

`LANGUAGES NONE` tells CMake there is no compiler to look for. There is also no
`cmake --build` step: the "build" of an interpreted application is copying files
— unless you add a syntax check or a test.

### What the package cannot provide

The application needs the `raven` module. A `.deb` can only depend on packages
that exist in a repository, and `raven-gui` is a pip package (and a Windows-only
wheel at the time of writing). So the README of your application must say how to
get it:

```bash
pip install raven-gui                                  # Windows today
# or, from the Raven checkout:
export PYTHONPATH="/path/to/Raven/bindings/python:$PYTHONPATH"
export RAVEN_LIBRARY="/path/to/libraven.so"
```

A distribution would ship `raven` itself as a `.deb`; then this package would
`Depends: python3, raven-gui`.

---

## Build the package

```bash
cd "Tutorials/Python/09-packaging"
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cd build
cpack -G DEB
cpack -G TGZ
```

Expected:

```
CPack: - package: .../build/notes_1.0.0_amd64.deb generated.
```

Inspect it:

```bash
dpkg-deb -f notes_1.0.0_amd64.deb Package Version Depends Installed-Size
dpkg-deb -c notes_1.0.0_amd64.deb
```

It is about 5 KB, and `Depends` is exactly what the CMakeLists said:

```
Depends: python3
```

---

## Install it

```bash
sudo apt install ./notes_1.0.0_amd64.deb
which notes
notes --test            # needs the raven module, see above
dpkg -L notes
sudo apt remove notes
```

If you did not `pip install raven-gui`, give the environment to the launcher:

```bash
PYTHONPATH=/path/to/Raven/bindings/python \
RAVEN_LIBRARY=/path/to/libraven.so \
notes --test
```

`notes` also appears in the application menu (`/usr/share/applications`), and
`notes somefile.txt` opens that file.

---

## The archive, and staging an install

```bash
cpack -G TGZ
tar xzf notes-1.0.0-Linux.tar.gz -C /tmp/notes-1.0.0
PYTHONPATH=... RAVEN_LIBRARY=... /tmp/notes-1.0.0/usr/bin/notes --test
```

The launcher resolves `../share/notes/main.py` relative to itself, so the
extracted tree works anywhere — no compiled path to mismatch. That is one
advantage of a launcher over a compiled-in data directory.

To look at the tree without installing:

```bash
DESTDIR=/tmp/stage cmake --install build
find /tmp/stage -type f
```

---

## Where the files go

| Path | Content |
|---|---|
| `/usr/bin/notes` | the launcher script |
| `/usr/share/notes/main.py` | the application |
| `/usr/share/notes/notes.xml` | the interface, read-only data |
| `/usr/share/applications/notes.desktop` | menu entry |

Settings do not belong in `/usr`: the application writes them to the user's
configuration folder.

---

## Checkpoint

* Why is a launcher needed for a Python application?
* Why does `CPACK_DEBIAN_PACKAGE_DEPENDS` have to name `python3` explicitly?
* Why does the extracted archive work here, when the C++ step's did not?
* What does `install(PROGRAMS ...)` add that `install(FILES ...)` does not?

---

## Exercises

1. Add a syntax check as a build step:
   `add_custom_target(check ALL COMMAND python3 -m py_compile ${NOTES_SOURCE}/main.py)`.
   Now `cmake --build` fails on a broken script.
2. Add a man page, then check `man notes`.
3. Build a **wheel** for the same application with `pip wheel` and compare what
   each format installs. Which one belongs in a distribution?
4. Add `CPACK_DEBIAN_PACKAGE_RECOMMENDS "raven-gui"` and see it in the control
   file.
5. Start the application from a desktop entry and make sure the working
   directory it gets does not matter.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `ModuleNotFoundError: No module named 'raven'` when running `notes` | The module is not installed for the system python. Use `pip install`, or set `PYTHONPATH`/`RAVEN_LIBRARY`. |
| `/usr/bin/notes: Permission denied` | The launcher lost its executable bit: it must be installed with `install(PROGRAMS ...)`. |
| `notes` opens but shows no window content | `notes.xml` is not next to `main.py` in `/usr/share/notes`. |
| `Depends:` is empty | Set `CPACK_DEBIAN_PACKAGE_DEPENDS`; there are no ELF files for CPack to scan. |
| The menu entry does nothing | `Exec=notes %F` needs `notes` on `PATH`. |

**Previous:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md) ·
**Back to the course index:** [README](../README.md)
