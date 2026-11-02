# Step 09 — Packaging and installation (Rust)

**Level 4 · Advanced · ~40 minutes**

The application of step 08 runs from `target/release/`. Nobody can install it.
This step wraps the cargo build in a small CMake project that installs it and
packs it with **CPack**.

Cargo has no packaging of its own: `cargo install` copies a binary and nothing
else. A system package also needs a data file, a desktop entry, metadata and
dependencies — that is CPack's job, and CMake is the tool that drives both.

---

## What you will learn

| Piece | What it does |
|---|---|
| `add_custom_command` + `add_custom_target` | runs `cargo build --release` as part of `cmake --build` |
| `install(PROGRAMS ... RENAME notes)` | installs the binary under the name a user types |
| `install(FILES ...)` | the interface file and the `.desktop` entry |
| `CPACK_*` / `CPACK_DEBIAN_*` | name, version, contact, generated dependencies |
| `RAVEN_STATIC=1` | a binary with no `libraven.so` to ship |

---

## Why this is a separate project

This folder contains no Rust source. It builds **step 08**:

```cmake
add_custom_command(
    OUTPUT "${NOTES_BINARY}"
    COMMAND ${CMAKE_COMMAND} -E env
        "CARGO_TARGET_DIR=${RUST_TARGET_DIR}"
        "NOTES_DATA_DIR=${CMAKE_INSTALL_FULL_DATADIR}/notes"
        "${CARGO}" build --release --manifest-path "${RUST_COURSE}/Cargo.toml"
            -p step-08-capstone-notes
    WORKING_DIRECTORY "${RUST_COURSE}")
add_custom_target(notes ALL DEPENDS "${NOTES_BINARY}")
```

`cmake --build` now runs cargo; a real project would put the same command in its
own `CMakeLists.txt`.

---

## Two decisions that make the package work

**1. Where the interface file lives.** Step 08 resolves `notes.xml` in this
order: `NOTES_XML`, then `NOTES_DATA_DIR`, then next to the crate:

```rust
fn xml_path() -> std::path::PathBuf {
    if let Some(path) = std::env::var_os("NOTES_XML") { return path.into(); }
    let folder = option_env!("NOTES_DATA_DIR").unwrap_or(env!("CARGO_MANIFEST_DIR"));
    std::path::PathBuf::from(folder).join("notes.xml")
}
```

`option_env!` is read **when the crate is compiled**, so the packaging build
passes it:

```cmake
"NOTES_DATA_DIR=${CMAKE_INSTALL_FULL_DATADIR}/notes"     # /usr/share/notes
```

**2. What the binary links.** Raven's crate links `libraven.so` by default, so
the package would have to ship it (or depend on a `raven` package that does not
exist). Link it statically instead:

```bash
export RAVEN_LIB_DIR=/usr/lib RAVEN_STATIC=1
```

The binary then needs only libc, libgcc and X11 — which `SHLIBDEPS` finds.

---

## The CMakeLists, part by part

```cmake
project(notes VERSION 1.0.0 LANGUAGES NONE)
include(GNUInstallDirs)
find_program(CARGO cargo REQUIRED)

# ... the add_custom_command above ...

install(PROGRAMS "${NOTES_BINARY}" DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME notes)
install(FILES "${RUST_COURSE}/08-capstone-notes/notes.xml"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES notes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "notes")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Raven tutorials <you@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()
include(CPack)
```

`include(CPack)` must come last: it writes `CPackConfig.cmake` from the variables
above.

---

## Build the package

```bash
cd "Tutorials/Rust/09-packaging"

export RAVEN_LIB_DIR=/usr/lib       # or your Raven build folder
export RAVEN_STATIC=1               # no libraven.so in the package

cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build                 # runs cargo --release
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
dpkg-deb -f notes_1.0.0_amd64.deb Package Version Depends
dpkg-deb -c notes_1.0.0_amd64.deb
```

`Depends` should name `libc6`, `libgcc-s1`, `libx11-6` — and **not** `libraven`:

```
Depends: libc6 (>= 2.38), libgcc-s1 (>= 4.2), libx11-6
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

`notes --test` is the check from step 08, now running from `/usr/bin/notes` and
reading `/usr/share/notes/notes.xml`.

---

## The archive, and staging an install

```bash
cpack -G TGZ
tar xzf notes-1.0.0-Linux.tar.gz -C /tmp/notes-1.0.0
NOTES_XML=/tmp/notes-1.0.0/usr/share/notes/notes.xml /tmp/notes-1.0.0/usr/bin/notes
```

The `NOTES_XML` line is needed because the compiled data path still says
`/usr/share/notes`. Configure with the prefix you will really use
(`-DCMAKE_INSTALL_PREFIX=$HOME/.local`) if you want a self-consistent archive.

To look at an install without doing it:

```bash
DESTDIR=/tmp/stage cmake --install build
find /tmp/stage -type f
```

`cpack` uses exactly this staging internally.

---

## Where the files go

| Path | Content |
|---|---|
| `/usr/bin/notes` | the compiled binary |
| `/usr/share/notes/notes.xml` | the interface, read-only data |
| `/usr/share/applications/notes.desktop` | menu entry |

Keep user data out of `/usr`: settings belong in the user's configuration
folder, documents wherever the user saves them.

---

## Checkpoint

* What does `option_env!` read, and when?
* Why link Raven statically for this package?
* What runs when `cmake --build` is called on this project?
* Which CPack variable produces `Depends:` without listing libraries by hand?

---

## Exercises

1. Add `RAVEN_LIB_DIR` and `RAVEN_STATIC` as CMake cache variables and pass them
   to the cargo command, so the build does not depend on the caller's shell.
2. Add a man page (`install(FILES notes.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)`).
3. Bump `project(... VERSION)` and repackage; check the file name.
4. Try the default (dynamic) linking and read the error `dpkg-shlibdeps` gives.
   What would it take to ship `libraven.so` in the package?
5. **AppImage.** Raven's `cmake/AppImage.cmake` is a CPack External generator
   that turns an install tree into one file. Read it, and adapt it here.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `cannot find -lraven` during the cargo build | `RAVEN_LIB_DIR` is not set for the packaging build. |
| `Depends` contains `libraven` | `RAVEN_STATIC=1` was not set: the binary links the shared library. |
| The installed app cannot find `notes.xml` | `NOTES_DATA_DIR` was compiled from a different prefix. Reconfigure with `-DCMAKE_INSTALL_PREFIX=/usr` or set `NOTES_XML`. |
| `cpack` says the file does not exist | `cmake --build` was not run, so cargo never produced the binary. |
| The binary is called `08-capstone-notes` after installing | `RENAME notes` is missing from `install(PROGRAMS ...)`. |

**Previous:** [Step 08 — Starter project: Notes](../08-capstone-notes/README.md) ·
**Back to the course index:** [README](../README.md)
