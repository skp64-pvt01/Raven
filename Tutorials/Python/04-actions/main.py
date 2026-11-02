"""
Raven tutorial 04 (Python) - actions and shortcuts.

A button, a menu entry and a keyboard shortcut can run the same code when they
all name the same *action*:

    OnClicked="counter.add" in XML,
    Raction_fire("counter.add", sender) from code,
    Rwindow_register_shortcut(window, R_KEY_I, R_MOD_CTRL, "counter.add").

Raction_bind gives each name a Python function. The function receives the sender
and the action name, so it can tell the two add buttons apart if it wants to.
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

XML = """
<Window Title="Tutorial 04 - Actions" Width="480" Height="420">
    <VBox Spacing="10" Slot.Padding="20">
        <Label Text="Actions" TextSize="22"/>
        <Label Name="count" Text="0" TextSize="40" Slot.HAlign="Center"/>

        <HBox Spacing="8" Slot.HAlign="Center">
            <Button Name="add" Text="+1" Style="Primary" OnClicked="counter.add" Tooltip="Ctrl+I"/>
            <Button Name="add_ten" Text="+10" OnClicked="counter.add_ten"/>
            <Button Name="reset" Text="Reset" OnClicked="counter.reset" Tooltip="Ctrl+0"/>
        </HBox>

        <Checkbox Name="enabled" Text="Add buttons enabled" Checked="true"
            OnChanged="counter.enabled"/>

        <Spacer Slot.Fill="1"/>
        <Label Name="status" Text="Ready." Opacity="0.7"/>
    </VBox>
</Window>"""


def main():
    if not tutorial.init("Raven tutorial 04"):
        return 1

    window = r.Rwindow_create_from_xml(XML)
    tutorial.require(window != r.R_INVALID_ID, "the window could not be built")
    label = r.Rwidget_find(window, "count")
    status = r.Rwidget_find(window, "status")
    add = r.Rwidget_find(window, "add")
    add_ten = r.Rwidget_find(window, "add_ten")
    enabled = r.Rwidget_find(window, "enabled")

    count = [0]

    def show(what):
        r.Rlabel_set_text(label, str(count[0]))
        r.Rlabel_set_text(status, "Last action: %s" % what)

    def on_add(sender, action, user_data):
        count[0] += 1
        show(action)

    def on_add_ten(sender, action, user_data):
        count[0] += 10
        show(action)

    def on_reset(sender, action, user_data):
        count[0] = 0
        show(action)

    def on_enabled(sender, action, user_data):
        on = r.Rcheckbox_is_checked(enabled)
        r.Rwidget_set_enabled(add, on)
        r.Rwidget_set_enabled(add_ten, on)
        r.Rlabel_set_text(status, "Add buttons enabled." if on else "Add buttons disabled.")

    r.Raction_bind("counter.add", on_add)
    r.Raction_bind("counter.add_ten", on_add_ten)
    r.Raction_bind("counter.reset", on_reset)
    r.Raction_bind("counter.enabled", on_enabled)

    # Keyboard shortcuts run the same actions, wherever the focus is.
    r.Rwindow_register_shortcut(window, r.R_KEY_I, r.R_MOD_CTRL, "counter.add")
    r.Rwindow_register_shortcut(window, r.R_KEY_0, r.R_MOD_CTRL, "counter.reset")

    if tutorial.is_test():
        r.Rdraw_frame()

        tutorial.click(window, add)
        tutorial.check(count[0] == 1, "the button fires counter.add")

        tutorial.key(window, r.R_KEY_I, r.R_MOD_CTRL)
        tutorial.check(count[0] == 2, "Ctrl+I fires the same action")

        r.Raction_fire("counter.add_ten", r.R_INVALID_ID)
        tutorial.check(count[0] == 12, "an action can be fired from code")

        # A disabled widget takes no clicks...
        tutorial.click(window, enabled)
        tutorial.check(not r.Rcheckbox_is_checked(enabled), "the checkbox is cleared")
        tutorial.click(window, add)
        tutorial.check(count[0] == 12, "a disabled button does not fire")

        # ...but the shortcut still does: it belongs to the window, not the button.
        tutorial.key(window, r.R_KEY_I, r.R_MOD_CTRL)
        tutorial.check(count[0] == 13, "the shortcut ignores the disabled button")

        tutorial.key(window, r.R_KEY_0, r.R_MOD_CTRL)
        tutorial.check(count[0] == 0, "Ctrl+0 resets")
        tutorial.check(r.Rlabel_get_text(status) == "Last action: counter.reset",
                       "the status names the action")

        return tutorial.finish("04-actions")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("04-actions")


if __name__ == "__main__":
    sys.exit(main())
