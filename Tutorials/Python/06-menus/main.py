"""
Raven tutorial 06 (Python) - menus.

A menu bar is XML like everything else:

    <MenuBar>                    the bar (first child of the window's VBox)
    <Menu Text="File">           a title, or a sub-menu when nested in a Menu
    <MenuItem Text="New" .../>   an entry; <Separator/> a line
    Shortcut="Ctrl+N"            works with the menu closed, and shows in it
    Checkable="true"             an entry with a check mark
    OnClicked="name"             fires the action, same as a button

Raven also has built-in actions that need no Python at all: app.quit,
theme.dark, theme.light, show:NAME, toggle:NAME...
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

XML = """
<Window Title="Tutorial 06 - Menus" Width="640" Height="420">
    <VBox Spacing="0">
        <MenuBar>
            <Menu Text="File">
                <MenuItem Text="New" Shortcut="Ctrl+N" OnClicked="menu.new"/>
                <MenuItem Text="Open..." Shortcut="Ctrl+O" OnClicked="menu.open"/>
                <Separator/>
                <Menu Text="Recent">
                    <MenuItem Text="notes.txt" OnClicked="menu.recent"/>
                </Menu>
                <Separator/>
                <MenuItem Text="Quit" Shortcut="Ctrl+Q" OnClicked="app.quit"/>
            </Menu>
            <Menu Text="View">
                <MenuItem Name="view.status" Text="Show status" Shortcut="Ctrl+G"
                    Checkable="true" Checked="true" OnClicked="view.status"/>
                <Separator/>
                <MenuItem Text="Dark" OnClicked="theme.dark"/>
                <MenuItem Text="Light" OnClicked="theme.light"/>
            </Menu>
        </MenuBar>

        <Label Name="body" Slot.Fill="1" Slot.Padding="24"
            Text="Open the File menu, or press Ctrl+N, Ctrl+O, Ctrl+G."/>
        <Label Name="status" Text="Menu: none" Slot.Padding="12,0,12,12" Opacity="0.8"/>
    </VBox>
</Window>"""


def main():
    if not tutorial.init("Raven tutorial 06"):
        return 1

    window = r.Rwindow_create_from_xml(XML)
    tutorial.require(window != r.R_INVALID_ID, "the window could not be built")
    status = r.Rwidget_find(window, "status")
    show_status = r.Rwidget_find(window, "view.status")

    last = ["none"]

    def on_menu(sender, action, user_data):
        last[0] = action
        r.Rlabel_set_text(status, "Menu: %s" % action.split(".")[-1])

    for name in ("menu.new", "menu.open", "menu.recent"):
        r.Raction_bind(name, on_menu)

    def on_show_status(sender, action, user_data):
        # A checkable entry keeps its own state: ask it, never mirror it.
        checked = r.Rmenu_item_is_checked(sender)
        r.Rlabel_set_text(status, "Status shown" if checked else "Status hidden")

    r.Raction_bind("view.status", on_show_status)

    if tutorial.is_test():
        r.Rdraw_frame()

        # Geometry of the menus: the bar is one control high, its rows are
        # R_METRIC_ROW_HEIGHT. A click on a title opens it, a click below picks
        # the entry at that row.
        bar = r.Rtheme_get_metric(r.R_METRIC_CONTROL_HEIGHT)
        row = r.Rtheme_get_metric(r.R_METRIC_ROW_HEIGHT)

        tutorial.click_at(window, 15, bar * 0.5)
        tutorial.click_at(window, 30, bar + 4 + row * 0.5)
        tutorial.check(last[0] == "menu.new", "File > New fires menu.new")
        tutorial.check(r.Rlabel_get_text(status) == "Menu: new",
                       "the status label names the entry")

        # The shortcut of the same menu, with the menu closed.
        tutorial.key(window, r.R_KEY_O, r.R_MOD_CTRL)
        tutorial.check(last[0] == "menu.open", "Ctrl+O fires menu.open")

        # A checkable entry toggles itself, then reports it.
        tutorial.check(r.Rmenu_item_is_checked(show_status), "Show status starts checked")
        tutorial.key(window, r.R_KEY_G, r.R_MOD_CTRL)
        tutorial.check(not r.Rmenu_item_is_checked(show_status), "Ctrl+G clears the check mark")
        tutorial.check(r.Rlabel_get_text(status) == "Status hidden",
                       "the entry reports its own new state")

        # The built-in app.quit needs no binding at all.
        tutorial.key(window, r.R_KEY_Q, r.R_MOD_CTRL)
        tutorial.check(r.Ris_running() == 0, "Ctrl+Q runs the built-in app.quit")

        return tutorial.finish("06-menus")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("06-menus")


if __name__ == "__main__":
    sys.exit(main())
