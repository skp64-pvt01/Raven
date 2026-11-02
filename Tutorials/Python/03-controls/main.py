"""
Raven tutorial 03 (Python) - controls and state.

A widget holds its own state. The program reads it with a getter and never keeps
a copy: Rcheckbox_is_checked IS the state of the checkbox.

In XML, OnChanged="name" fires the action "name" when the value changes (a text
input fires it on Enter). One action can serve every control, because it reads
them all.
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

XML = """
<Window Title="Tutorial 03 - Controls" Width="520" Height="480">
    <VBox Spacing="10" Slot.Padding="20">
        <Label Text="Controls" TextSize="22"/>

        <Label Text="Your name"/>
        <TextInput Name="name" Placeholder="Type a name, then Enter" OnChanged="form.update"/>

        <Checkbox Name="loud" Text="Shout it" OnChanged="form.update"/>

        <Label Text="Text size"/>
        <Slider Name="size" Min="12" Max="32" Value="20" OnChanged="form.update"/>

        <Label Text="Theme"/>
        <ComboBox Name="theme" Items="Light|Dark" Selected="0" OnChanged="form.update"/>

        <Separator/>

        <Label Name="preview" Text="Hello, world!" TextSize="20" Slot.HAlign="Center"/>
        <Label Name="logged" Text="ready" Opacity="0.6" Slot.HAlign="Center"/>
    </VBox>
</Window>"""


def main():
    if not tutorial.init("Raven tutorial 03"):
        return 1

    window = r.Rwindow_create_from_xml(XML)
    tutorial.require(window != r.R_INVALID_ID, "the window could not be built")
    name = r.Rwidget_find(window, "name")
    loud = r.Rwidget_find(window, "loud")
    size = r.Rwidget_find(window, "size")
    theme = r.Rwidget_find(window, "theme")
    preview = r.Rwidget_find(window, "preview")
    logged = r.Rwidget_find(window, "logged")

    def update(sender, action, user_data):
        who = r.Rtext_input_get_text(name) or "world"
        if r.Rcheckbox_is_checked(loud):
            who = who.upper()
        r.Rlabel_set_text(preview, "Hello, %s!" % who)
        r.Rlabel_set_text_size(preview, r.Rslider_get_value(size))

        chosen = r.Rcombo_box_get_selected(theme)
        r.Rtheme_apply(r.R_THEME_DARK if chosen == 1 else r.R_THEME_LIGHT)
        r.Rlabel_set_text(logged, "theme %d, size %.0f" % (chosen, r.Rslider_get_value(size)))

    r.Raction_bind("form.update", update)

    if tutorial.is_test():
        r.Rdraw_frame()

        # Typing, then Enter: the text input fires its action on Enter.
        tutorial.click(window, name)
        r.Rwindow_inject_text(window, "Ada")
        tutorial.key(window, r.R_KEY_ENTER)
        tutorial.check(r.Rlabel_get_text(preview) == "Hello, Ada!",
                       "Enter fires the action with the typed name")

        # The checkbox toggles and the same action runs again.
        tutorial.click(window, loud)
        tutorial.check(r.Rcheckbox_is_checked(loud), "the checkbox is checked")
        tutorial.check(r.Rlabel_get_text(preview) == "Hello, ADA!",
                       "the checked box uppercases the name")

        # The middle of the slider is the middle of the range: (12 + 32) / 2.
        tutorial.click(window, size)
        tutorial.check(abs(r.Rslider_get_value(size) - 22) < 1,
                       "clicking the middle of the slider gives 22")

        # The combo box: a click opens the list, a click on the second row picks it.
        x, y, _, height = r.Rwidget_get_rect(theme)
        row = r.Rtheme_get_metric(r.R_METRIC_ROW_HEIGHT)
        tutorial.click(window, theme)
        tutorial.click_at(window, x + 10, y + height + 2 + row * 1.5)
        tutorial.check(r.Rcombo_box_get_selected(theme) == 1, "the second theme is selected")
        tutorial.check(r.Rtheme_is_dark(), "the dark theme is applied")

        return tutorial.finish("03-controls")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("03-controls")


if __name__ == "__main__":
    sys.exit(main())
