//! Raven tutorial 02 (Rust) - layout.
//!
//! A panel arranges its children; each child carries a *slot* that says how.
//! In XML the slot settings are the attributes that start with `Slot.`:
//!
//!   Slot.Padding    space around the child
//!   Slot.Fill       share of the leftover space (1 = one share)
//!   Slot.HAlign     where the child sits horizontally in its cell
//!   Slot.VAlign     the same, vertically
//!
//! `Spacing` on a panel is the gap between its children.

mod harness;

use raven::*;

const XML: &str = r#"
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
</Window>"#;

fn main() {
    if !harness::init("Raven tutorial 02") {
        std::process::exit(1);
    }

    let window = window_from_xml(XML);
    let status = widget_find(window, "status");
    action_bind("layout.clear", move |_sender, _action| {
        unsafe { Rlabel_set_text(status, cstr("Nothing pressed yet.").as_ptr()) };
    });

    if harness::is_test() {
        unsafe { Rdraw_frame() };

        // The three buttons of the first row: same width, left to right.
        let equal = widget_find(window, "equal");
        let first = unsafe { Rwidget_get_child(equal, 0) };
        let (mut previous_x, _, first_width, _) = harness::rect(first);
        harness::check(first_width > 0.0, "the first button has a width");

        for index in 1..3 {
            let (x, _, width, _) = harness::rect(unsafe { Rwidget_get_child(equal, index) });
            harness::check((width - first_width).abs() < 1.0, "the three buttons are equally wide");
            harness::check(x > previous_x, "the buttons are placed left to right");
            previous_x = x;
        }

        // The footer is below the row of buttons.
        let (_, footer_y, _, _) = harness::rect(widget_find(window, "footer"));
        let (_, row_y, _, row_height) = harness::rect(equal);
        harness::check(footer_y > row_y + row_height, "the spacer pushes the footer to the bottom");

        // The Clear action resets the label.
        unsafe {
            Rlabel_set_text(status, cstr("pressed").as_ptr());
            Raction_fire(cstr("layout.clear").as_ptr(), R_INVALID_ID);
        }
        harness::check(
            unsafe { text(Rlabel_get_text(status)) } == "Nothing pressed yet.",
            "the action resets the status label",
        );

        std::process::exit(harness::finish("02-layout"));
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("02-layout"));
}
