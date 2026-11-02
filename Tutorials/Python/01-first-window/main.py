"""
Raven tutorial 01 (Python) - a first window.

Three ideas only:
  1. Rinit_ex starts the library, Rshutdown ends it.
  2. A window is built from XML; Rwidget_find gets a widget back by name.
  3. Raction_bind answers OnClicked="..."; the frame loop is yours.

Build and test: see README.md next to this file.
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

XML = """
<Window Title="Tutorial 01 - First window" Width="440" Height="240">
    <VBox Spacing="12" Slot.Padding="24" Slot.HAlign="Center" Slot.VAlign="Center">
        <Label Text="Hello from Python" TextSize="24" Slot.HAlign="Center"/>
        <Label Name="counter" Text="Press the button" Slot.HAlign="Center"/>
        <HBox Spacing="8" Slot.HAlign="Center">
            <Button Name="click" Text="Click me" Style="Primary" OnClicked="counter.add"
                Tooltip="Counts the clicks"/>
            <Button Name="quit" Text="Quit" OnClicked="app.quit"/>
        </HBox>
    </VBox>
</Window>"""


def main():
    if not tutorial.init("Raven tutorial 01"):
        return 1

    window = r.Rwindow_create_from_xml(XML)
    tutorial.require(window != r.R_INVALID_ID, "the window could not be built")
    label = r.Rwidget_find(window, "counter")

    clicks = [0]

    def on_click(sender, action, user_data):
        clicks[0] += 1
        count = clicks[0]
        r.Rlabel_set_text(label, "Clicked %d time%s" % (count, "" if count == 1 else "s"))

    r.Raction_bind("counter.add", on_click)
    # app.quit is a built-in action of Raven: no binding needed.

    if tutorial.is_test():
        r.Rdraw_frame()  # lays the widgets out

        button = r.Rwidget_find(window, "click")
        tutorial.click(window, button)
        tutorial.check(r.Rlabel_get_text(label) == "Clicked 1 time", "the first click updates the label")

        tutorial.click(window, button)
        tutorial.check(r.Rlabel_get_text(label) == "Clicked 2 times", "the second click counts too")

        tutorial.click(window, r.Rwidget_find(window, "quit"))
        tutorial.check(r.Ris_running() == 0, "Quit ends the loop")
        return tutorial.finish("01-first-window")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("01-first-window")


if __name__ == "__main__":
    sys.exit(main())
