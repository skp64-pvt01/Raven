# Step 09 — Packaging and installation (TypeScript)

**Level 4 · Advanced · ~35 minutes**

The application of step 08 runs as `deno task 08`. Nobody can install that. This
step installs it where a system expects an application, and packs it with
**CPack**.

Nothing is compiled: a TypeScript application is source plus a runtime. What a
package adds is the layout, the launcher, the FFI library, and the metadata a
package manager needs.

---

## What you will learn

| Piece | What it does |
|---|---|
| `install(PROGRAMS notes ...)` | installs the launcher in `bin/` **with the executable bit** |
| `install(FILES ...)` | the modules, the XML, the `.desktop` entry, `libraven.so` |
| `include(GNUInstallDirs)` | the conventional folders |
| `get_filename_component(REALPATH)` | copies the real library, not the `libraven.so -> libraven.so.0` symlink |
| `CPACK_*` / `CPACK_DEBIAN_*` | name, version, maintainer, `Recommends:` |

---

## Three things a TypeScript package must get right

**1. A launcher.** A `.ts` file is not executable by name. `/usr/bin/notes` is a
small script that starts Deno with the permissions the binding needs:

```sh
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
: "${RAVEN_LIBRARY:=${HERE}/../lib/libraven.so}"
export RAVEN_LIBRARY
exec deno run --allow-ffi --allow-env --allow-read --allow-write \
    "$HERE/../share/notes/app/main.ts" "$@"
```

**2. The FFI library.** `raven.ts` calls `libraven.so`. The package installs it
in `/usr/lib`; the launcher points `RAVEN_LIBRARY` at it, so the application
works on a machine where Raven was never built. (`libraven.so` in a checkout is
a symlink to `libraven.so.0.2.0`: `REALPATH` copies the file itself, not the
link.)

**3. The import layout.** `app/main.ts` imports `../raven.ts` and
`../tutorial.ts`, so those two modules are installed one folder above it:

```
/usr/share/notes/raven.ts
/usr/share/notes/tutorial.ts
/usr/share/notes/app/main.ts
/usr/share/notes/app/notes.xml
```

The `notes.xml` sits next to `main.ts` because the application reads it with
`new URL("./notes.xml", import.meta.url)`.

---

## The CMakeLists

```cmake
project(notes VERSION 1.0.0 LANGUAGES NONE)
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "")
include(GNUInstallDirs)

install(PROGRAMS notes DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ../raven.ts ../tutorial.ts DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES ../08-capstone-notes/main.ts ../08-capstone-notes/notes.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/notes/app)
install(FILES notes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

get_filename_component(RAVEN_LIBRARY_REAL "$ENV{RAVEN_LIBRARY}" REALPATH)
install(FILES "${RAVEN_LIBRARY_REAL}" DESTINATION ${CMAKE_INSTALL_LIBDIR} RENAME libraven.so)

set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "notes")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Raven tutorials <you@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "deno")
endif()
include(CPack)
```

`Recommends: deno` rather than `Depends:`: no distribution provides a `deno`
package today, and a hard dependency would make `apt install` fail on most
systems. The README of a real application would say "install Deno first".

`LANGUAGES NONE` tells CMake there is no compiler; `CMAKE_INSTALL_LIBDIR` is set
because with no target architecture CMake cannot guess `lib/`.

---

## Build the package

```bash
cd "Tutorials/TypeScript/09-packaging"

export RAVEN_LIBRARY=/path/to/Raven/build/libraven.so   # shipped in the package
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
dpkg-deb -f notes_1.0.0_amd64.deb Package Version Depends Recommends Installed-Size
dpkg-deb -c notes_1.0.0_amd64.deb
```

```
./usr/bin/notes
./usr/lib/libraven.so
./usr/share/applications/notes.desktop
./usr/share/notes/app/main.ts
./usr/share/notes/app/notes.xml
./usr/share/notes/raven.ts
./usr/share/notes/tutorial.ts
```

---

## Install it

```bash
sudo apt install ./notes_1.0.0_amd64.deb
which notes
notes --test
dpkg -L notes
sudo apt remove notes
```

`notes --test` runs the step 08 checks from `/usr/bin/notes`, using
`/usr/lib/libraven.so` and `/usr/share/notes/app/notes.xml`. Deno must be on
`PATH`; the package recommends it rather than installing it.

---

## The archive, and staging an install

```bash
cpack -G TGZ
tar xzf notes-1.0.0-Linux.tar.gz -C /tmp/notes-1.0.0
/tmp/notes-1.0.0/usr/bin/notes --test
```

The launcher resolves everything relative to itself, so the extracted tree works
anywhere — no compiled-in path to mismatch.

```bash
DESTDIR=/tmp/stage cmake --install build
find /tmp/stage -type f
```

`cpack` stages exactly like this internally.

---

## Where the files go

| Path | Content |
|---|---|
| `/usr/bin/notes` | the launcher |
| `/usr/lib/libraven.so` | the FFI library the launcher points at |
| `/usr/share/notes/app/main.ts` | the application |
| `/usr/share/notes/app/notes.xml` | the interface |
| `/usr/share/notes/raven.ts`, `tutorial.ts` | the modules it imports |
| `/usr/share/applications/notes.desktop` | menu entry |

Settings belong in the user's configuration folder, not in `/usr`.

---

## Checkpoint

* Why does a TypeScript application need a launcher, and a Python one too, but a
  C++ one not?
* What breaks if `libraven.so` is installed as a symlink?
* Why `Recommends: deno` instead of `Depends: deno`?
* Why does `main.ts` live one folder below `raven.ts` in the installed tree?

---

## Exercises

1. Add a syntax check as a build step:
   `add_custom_target(check ALL COMMAND deno check ${COURSE}/08-capstone-notes/main.ts)`.
2. Try `deno compile` instead: `deno compile --allow-ffi ... -o notes main.ts`.
   What happens to `import.meta.url`, and how large is the binary?
3. Package the same application for Node: what has to be installed besides the
   script, and where does `koffi` go?
4. Add a man page and check `man notes` after installing.
5. **AppImage.** Raven's `cmake/AppImage.cmake` turns an install tree into one
   file with CPack's External generator. Read it and adapt it here.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `notes: deno: not found` | Deno is not installed, or not on `PATH` for the user running it. |
| `Could not open library` after installing | The package was built without `RAVEN_LIBRARY`, so `libraven.so` is missing. |
| A broken symlink in the package | `install(FILES ...)` copied `libraven.so` instead of its target; use `REALPATH`. |
| `Module not found: ../raven.ts` | The modules were installed flat instead of one folder above `app/`. |
| `PermissionDenied` on the XML | The launcher passes `--allow-read`; a hand-written one must too. |

**Previous:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md) ·
**Back to the course index:** [README](../README.md)
