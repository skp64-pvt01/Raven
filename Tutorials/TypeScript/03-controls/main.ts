/**
 * Raven tutorial 03 (TypeScript) - controls and state.
 *
 * A widget holds its own state. The program reads it with a getter and never
 * keeps a copy: `isChecked(checkbox)` IS the state of the checkbox.
 *
 * In XML, OnChanged="name" fires the action `name` when the value changes (a
 * text input fires it on Enter). One action can serve every control, because it
 * reads them all.
 *
 * Run:  deno task 03        Test: deno task 03 --test
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML = `
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
</Window>`;

function main(): number {
  if (!t.init("Raven tutorial 03")) return 1;

  const window = r.windowFromXml(XML);
  t.require(window !== r.INVALID_ID, "the window could not be built");
  const name = r.find(window, "name");
  const loud = r.find(window, "loud");
  const size = r.find(window, "size");
  const theme = r.find(window, "theme");
  const preview = r.find(window, "preview");
  const logged = r.find(window, "logged");

  r.actionBind("form.update", () => {
    const typed = r.inputText(name);
    let who = typed.length ? typed : "world";
    if (r.isChecked(loud)) who = who.toUpperCase();

    r.setText(preview, `Hello, ${who}!`);
    r.setTextSize(preview, r.sliderValue(size));

    const chosen = r.selectedIndex(theme);
    r.applyTheme(chosen === 1 ? r.R_THEME_DARK : r.R_THEME_LIGHT);
    r.setText(logged, `theme ${chosen}, size ${r.sliderValue(size).toFixed(0)}`);
  });

  if (t.isTest()) {
    r.drawFrame();

    // Typing, then Enter: the text input fires its action on Enter.
    t.click(window, name);
    r.typeText(window, "Ada");
    t.key(window, r.R_KEY_ENTER);
    t.check(r.getText(preview) === "Hello, Ada!", "Enter fires the action with the typed name");

    // The checkbox toggles and the same action runs again.
    t.click(window, loud);
    t.check(r.isChecked(loud), "the checkbox is checked");
    t.check(r.getText(preview) === "Hello, ADA!", "the checked box uppercases the name");

    // The middle of the slider is the middle of the range: (12 + 32) / 2.
    t.click(window, size);
    t.check(Math.abs(r.sliderValue(size) - 22) < 1, "clicking the middle of the slider gives 22");

    // The combo box: a click opens the list, a click on the second row picks it.
    const [x, y, , height] = r.rect(theme);
    const row = r.metric(r.R_METRIC_ROW_HEIGHT);
    t.click(window, theme);
    t.clickAt(window, x + 10, y + height + 2 + row * 1.5);
    t.check(r.selectedIndex(theme) === 1, "the second theme is selected");
    t.check(r.isDark(), "the dark theme is applied");

    return t.finish("03-controls");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("03-controls");
}

Deno.exit(main());
