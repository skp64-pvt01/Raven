# Step 05 — An interface written in XML

**Level 3 · Intermediate · ~30 minutes**

So far the interface exists as a hundred lines of C++. For anything bigger than
a dialog that becomes hard to read and harder to tweak. Raven can build the same
tree from an XML file; the code then only names the widgets it actually touches.

---

## What you will learn

```cpp
Rid window = Rwindow_create_from_xml_file("window.xml");   // or _from_xml(string, size)
Rid name   = Rwidget_find(window, "name");                 // by the Name attribute
Raction_bind("app.greet", on_action, nullptr);             // OnClicked="app.greet"
Rxml_set_live_reload(R_TRUE);                              // reload when the file is saved
```

Two rules cover most of the format:

* **a tag is a widget** — `<Button>`, `<TextInput>`, `<VBox>`;
* **an attribute is a property** — `Text`, `TextSize`, `Name`, and the `Slot.*`
  ones that mirror the `Rslot_set_*` functions.

The full list of tags and attributes is in `docs/XML.md` in the Raven
repository, and in `docs/reference.html` of the installed documentation.

---

## The program

[`main.cpp`](main.cpp) is short on purpose — the interface is in
[`window.xml`](window.xml):

```xml
<Window Title="Tutorial 05 - XML interface" Width="520" Height="380">
    <VBox Spacing="10" Slot.Padding="24">
        <Label Text="Interface in XML" TextSize="22"/>
        <Separator/>
        <Label Text="Your name"/>
        <TextInput Name="name" Placeholder="Type a name"/>
        <Button Name="greet" Text="Greet" Style="Primary" OnClicked="app.greet"/>
        <Label Name="greeting" Text="Hello!" TextSize="18"/>
    </VBox>
</Window>
```

`OnClicked="app.greet"` does **not** call a C++ function by name. It fires the
action `app.greet`, which the program binds — exactly the mechanism of step 04.
That is why the XML does not need to know the language of the program.

The code that remains:

```cpp
Raction_bind("app.greet", on_action, nullptr);
Rid window = Rwindow_create_from_xml_file(RAVEN_TUTORIAL_DIR "/window.xml");
g_name = Rwidget_find(window, "name");
g_greeting = Rwidget_find(window, "greeting");
```

### Live reload

One line, and the file is read again whenever it is saved while the program
runs:

```cpp
Rxml_set_live_reload(R_TRUE);
```

It is compiled in a comment in `main.cpp` — uncomment it, run the program, and
edit `window.xml` in another window. Widgets keep their state where the file did
not change.

---

## Build

```bash
cd "Tutorials/C++"
cmake --build build --target 05-xml-interface
```

The XML path reaches the program through a compile definition set in
`CMakeLists.txt`:

```cmake
target_compile_definitions(05-xml-interface PRIVATE
    RAVEN_TUTORIAL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/05-xml-interface")
```

That is why this step runs from `build/` without arguments; a distributed
program would ship the file next to its binary and use a relative path instead.

## Run

```bash
./build/05-xml-interface
```

Type a name, press **Greet**. Then edit `window.xml` — change the title, add a
label — save, and see it applied without restarting (with live reload on).

## Test

```bash
./build/05-xml-interface --test
```

```
05-xml-interface: 4 checks, 0 failures
PASS
```

The test checks that the two `Name`s were found, drives the input and the
button, and then fires `app.greet` directly to prove the XML is wired to an
action and not to the button.

---

## Checkpoint

* What happens when `Rwidget_find` does not find the name? (It returns
  `R_INVALID_ID`; `Rwidget_is_valid` confirms it, and every call on an invalid
  handle raises a clean error.)
* Why is `OnClicked="app.greet"` better than naming a function?
* What does an XML file **not** contain, compared with the step 01 program?

---

## Exercises

1. Add a `<Checkbox Name="loud" Text="Shout it"/>` and make the greeting
   uppercase. Which file do you edit, and which function do you change?
2. Give the window `MinWidth` and `MinHeight` and try to shrink it.
3. Use `Rwindow_create_from_xml` with an XML **string** instead of a file, for
   an interface built at run time.
4. Turn on live reload and move a widget to another parent while the program
   runs. What survives and what does not?

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `cannot read window.xml` | The program was run from another directory, or `RAVEN_TUTORIAL_DIR` was lost: run from `build/`. |
| A widget is not found | `Name` is case-sensitive and must be on the widget, not on its parent. |
| `OnClicked` does nothing | The action name in the XML and in `Raction_bind` differ, or the bind comes after the window is built (bind first). |
| Unknown attribute warning | The attribute name is not part of the reference; check `docs/XML.md`. |
| The XML loads but the window is blank | The root tag is not `<Window>`, or the content is not inside it. |

**Previous:** [Step 04 — Actions and shortcuts](../04-actions-and-shortcuts/README.md) ·
**Next:** [Step 06 — Menus](../06-menus/README.md)
