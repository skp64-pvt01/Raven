# Step 01 — A first window

**Level 1 · Beginner · ~20 minutes**

You will open a window with three widgets and make a button change a label. This
is the smallest complete Raven program; everything later is more of this.

---

## What you will learn

1. How a Raven program starts and stops (`Rinit_ex`, `Rshutdown`).
2. What a window and a widget are, and how they are attached.
3. How a button calls your function.
4. Why the frame loop belongs to your `main`.

---

## The program

Open [`main.cpp`](main.cpp). In order:

### Starting the library

```cpp
RInitDesc desc{};
desc.app_name = "Raven tutorial 01";
desc.backend = R_BACKEND_VULKAN;
if (!Rinit_ex(&desc))
  return 1;
```

`Rinit_ex` takes a `RInitDesc` so every setting has a name. The course helper
`tutorial::init` fills it for you and switches to the null backend when the
program is run with `--test`. `Rinit(backend)` is the short form when the
defaults are fine.

`Rshutdown()` releases every window, widget and resource still alive. Call it
once, at the end; do not free windows by hand in a program this simple.

### A window

```cpp
Rid window =
    Rwindow_create("Tutorial 01 - First window", 440, 240, R_WINDOW_DEFAULT);
```

The result is a `Rid`: an opaque number, not a pointer. Every call checks it, so
a stale handle raises a clean error instead of corrupting memory.

### A tree of widgets

```cpp
Rid box = Rwidget_create(R_WIDGET_VBOX);
Rwindow_set_content(window, box);
Rid title = Rwidget_add_child(box, Rlabel_create("Hello from C++"));
```

A window shows exactly one root widget, set with `Rwindow_set_content`. That
root is usually a panel — here a `VBOX`, which stacks its children top to
bottom. Widgets are added with `Rwidget_add_child`, which returns the child so
calls can be chained.

`Rlabel_create("...")` and `Rbutton_create("...")` are convenience constructors
for the two most common controls. Everything else is created with
`Rwidget_create(R_WIDGET_...)`.

### A callback

```cpp
Rbutton_set_clicked_callback(click_me, on_click, nullptr);
```

The function receives the button and the `user_data` pointer passed here. The
program keeps no list of buttons: the widget owns the callback. `on_click`
updates a counter and calls `Rlabel_set_text`.

### The loop

```cpp
while (Rwait_frame(-1.0)) {
}
```

`Rwait_frame` draws one frame, then sleeps until there is input or an animation
to run (`-1.0` = no timeout). It returns `R_FALSE` after `Rquit`. Raven never
runs a loop of its own: a menu item that calls `Rquit(0)` ends this `while`.

---

## Build

From the course root (`Tutorials/C++`):

```bash
cmake -S . -B build
cmake --build build --target 01-first-window
```

If this is your first step, the configure line is only needed once:

```bash
cmake -S . -B build          # find_package(raven) runs here
```

---

## Run

```bash
./build/01-first-window
```

Expected: a 440×240 window with a title, a hint label, and two buttons. Press
**Click me** twice — the label becomes _Clicked 2 times_. Press **Quit** — the
window closes and the process exits with code 0.

No display or no Vulkan driver? That is fine:

```bash
./build/01-first-window --test
```

```
01-first-window: 5 checks, 0 failures
PASS
```

`--test` drives the same widgets with `Rwindow_inject_mouse_move` and
`Rwindow_inject_mouse_button` — real clicks, no window.

---

## Checkpoint

After this step you should be able to answer:

- What does `Rwindow_set_content` do, and why only one widget?
- Where does the frame loop live, and what ends it?
- Why does `on_click` not need to know which window it belongs to?

---

## Exercises

1. Add a third button that resets the counter to zero.
2. Change the label's text size to `32` and its colour via
   `Rlabel_set_color(label, 1, 0.4f, 0.4f, 1)`. _(Find the exact signature in
   `/usr/include/raven/raven_controls.h`, installed by `raven-dev`.)_
3. Open two windows in the same program. Which one does `Rwait_frame` draw? Read
   the `raven_window.h` comment to check your answer.

---

## Troubleshooting

| Problem                                              | Fix                                                                                                      |
| ---------------------------------------------------- | -------------------------------------------------------------------------------------------------------- |
| `cannot start Raven (is a Vulkan driver installed?)` | The machine has no Vulkan driver. Use `--test`, or run on a machine with one.                            |
| The window appears empty                             | `Rwindow_set_content` was never called, or it was given a widget that is also a child of another parent. |
| `undefined reference to Rwindow_create`              | `target_link_libraries(... raven::raven)` is missing in CMake.                                           |
| Clicking does nothing                                | The callback was not set, or it was set on a different widget than the one in the tree.                  |

---

## Going further: the C++ wrapper

Raven ships `bindings/cpp/raven.hpp`, generated from the C headers. It gives
each handle a class and accepts lambdas:

```cpp
#include <raven.hpp>

auto window = raven::Window::create("Hello", 400, 200, R_WINDOW_DEFAULT);
auto button = raven::Button::create("Quit");
button.set_clicked_callback([](Rid) { raven::quit(0); });
window.set_content(button);
while (raven::wait_frame(-1.0)) {
}
```

It is a convenience layer, not a different library: the same objects, the same
rules. The course stays on the C API so every snippet can also be pasted into a
C project and compared with the official manual.

**Next:** [Step 02 — Layout](../02-layout/README.md)
