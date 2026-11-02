"""
Raven tutorial 02 (Python) - layout.

A panel arranges its children; each child carries a *slot* that says how. In XML
a slot is an attribute starting with "Slot.":

    Slot.Padding    space around the child
    Slot.Fill       share of the leftover space (1 = one share)
    Slot.HAlign     where the child sits horizontally in its cell
    Slot.VAlign     the same, vertically

"Spacing" on a panel is the gap between its children. No Python is needed for
any of this: the step is the XML.
"""

import os
import sys

import raven as r

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import tutorial

XML = """
<Window Title="Tutorial 02 - Layout" Width="640" Height="420">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Text="Layout" TextSize="22"/>
        <Label Text="The three buttons share the width: each has Slot.Fill = 1." Opacity="0.7"/>

        <HBox Name="equal" Spacing="8">
            <Button Text="One" Slot.Fill="1"/>
            <Button Text="Two" Slot.Fill="1"/>
            <Button Text="Three" Slot.Fill="1"/>
        </HBox>

        <Separator/>

        <HBox Name="mixed" Spacing="8">
            <Label Name="hint" Text="This label grows, the button keeps its size."
                Slot.Fill="1" Slot.VAlign="Center"/>
            <Button Text="Fixed"/>
        </HBox>

        <!-- takes all the height left over, and pushes the footer down -->
        <Spacer Slot.Fill="1"/>

        <HBox Name="footer" Spacing="8">
            <Label Name="status" Text="Nothing pressed yet." Slot.Fill="1" Slot.VAlign="Center"/>
            <Button Name="clear" Text="Clear" OnClicked="layout.clear"/>
        </HBox>
    </VBox>
</Window>"""


def main():
    if not tutorial.init("Raven tutorial 02"):
        return 1

    window = r.Rwindow_create_from_xml(XML)
    tutorial.require(window != r.R_INVALID_ID, "the window could not be built")
    status = r.Rwidget_find(window, "status")

    def on_clear(sender, action, user_data):
        r.Rlabel_set_text(status, "Nothing pressed yet.")

    r.Raction_bind("layout.clear", on_clear)

    if tutorial.is_test():
        r.Rdraw_frame()

        # The three buttons of the first row: same width, left to right.
        equal = r.Rwidget_find(window, "equal")
        first_x, _, first_width, _ = r.Rwidget_get_rect(r.Rwidget_get_child(equal, 0))
        tutorial.check(first_width > 0, "the first button has a width")

        previous_x = first_x
        for index in (1, 2):
            x, _, width, _ = r.Rwidget_get_rect(r.Rwidget_get_child(equal, index))
            tutorial.check(abs(width - first_width) < 1, "the three buttons are equally wide")
            tutorial.check(x > previous_x, "the buttons are placed left to right")
            previous_x = x

        # The footer is below the row of buttons.
        footer_y = r.Rwidget_get_rect(r.Rwidget_find(window, "footer"))[1]
        row_y, row_height = r.Rwidget_get_rect(equal)[1], r.Rwidget_get_rect(equal)[3]
        tutorial.check(footer_y > row_y + row_height, "the spacer pushes the footer to the bottom")

        # The Clear action resets the label.
        r.Rlabel_set_text(status, "pressed")
        r.Raction_fire("layout.clear", r.R_INVALID_ID)
        tutorial.check(r.Rlabel_get_text(status) == "Nothing pressed yet.",
                       "the action resets the status label")

        return tutorial.finish("02-layout")

    while r.Rwait_frame(-1.0):
        pass
    return tutorial.finish("02-layout")


if __name__ == "__main__":
    sys.exit(main())
