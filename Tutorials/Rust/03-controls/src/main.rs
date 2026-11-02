//! Raven tutorial 03 (Rust) - controls and state.
//!
//! A widget holds its own state. The program reads it with a getter and never
//! keeps a copy: `Rcheckbox_is_checked` *is* the state of the checkbox.
//!
//! In XML, `OnChanged="name"` fires the action `name` when the value changes
//! (a text input fires it on Enter). One action can serve every control.
//!
//! `Rwidget_find` gives back a widget by its `Name`; the getters are in the
//! `raven` crate under the C names.

mod harness;

use raven::*;

const XML: &str = r#"
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
</Window>"#;

fn main() {
    if !harness::init("Raven tutorial 03") {
        std::process::exit(1);
    }

    let window = window_from_xml(XML);
    let name = widget_find(window, "name");
    let loud = widget_find(window, "loud");
    let size = widget_find(window, "size");
    let theme = widget_find(window, "theme");
    let preview = widget_find(window, "preview");
    let logged = widget_find(window, "logged");

    // One action for every control: read them all, then show the result.
    action_bind("form.update", move |_sender, _action| unsafe {
        let typed = text(Rtext_input_get_text(name));
        let mut who = if typed.is_empty() { "world".to_string() } else { typed };
        if Rcheckbox_is_checked(loud) != 0 {
            who = who.to_uppercase();
        }
        Rlabel_set_text(preview, cstr(&format!("Hello, {who}!")).as_ptr());
        Rlabel_set_text_size(preview, Rslider_get_value(size));

        let chosen = Rcombo_box_get_selected(theme);
        Rtheme_apply(if chosen == 1 { R_THEME_DARK } else { R_THEME_LIGHT });
        Rlabel_set_text(logged, cstr(&format!("theme {chosen}, size {:.0}", Rslider_get_value(size))).as_ptr());
    });

    if harness::is_test() {
        unsafe { Rdraw_frame() };

        // Typing, then Enter: the text input fires its action on Enter.
        harness::click(window, name);
        unsafe { Rwindow_inject_text(window, cstr("Ada").as_ptr()) };
        harness::key(window, R_KEY_ENTER, 0);
        harness::check(
            unsafe { text(Rlabel_get_text(preview)) } == "Hello, Ada!",
            "Enter fires the action with the typed name",
        );

        // The checkbox toggles and the same action runs again.
        harness::click(window, loud);
        harness::check(unsafe { Rcheckbox_is_checked(loud) } != 0, "the checkbox is checked");
        harness::check(
            unsafe { text(Rlabel_get_text(preview)) } == "Hello, ADA!",
            "the checked box uppercases the name",
        );

        // The middle of the slider is the middle of the range: (12 + 32) / 2.
        harness::click(window, size);
        let value = unsafe { Rslider_get_value(size) };
        harness::check((value - 22.0).abs() < 1.0, "clicking the middle of the slider gives 22");

        // The combo box: a click opens the list, a click on the second row picks it.
        let (x, y, _, height) = harness::rect(theme);
        let row = unsafe { Rtheme_get_metric(R_METRIC_ROW_HEIGHT) };
        harness::click(window, theme);
        harness::click_at(window, x + 10.0, y + height + 2.0 + row * 1.5);
        harness::check(unsafe { Rcombo_box_get_selected(theme) } == 1, "the second theme is selected");
        harness::check(unsafe { Rtheme_is_dark() } != 0, "the dark theme is applied");

        std::process::exit(harness::finish("03-controls"));
    }

    unsafe {
        while Rwait_frame(-1.0) != 0 {}
    }
    std::process::exit(harness::finish("03-controls"));
}
