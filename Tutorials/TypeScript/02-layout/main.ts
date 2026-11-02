/**
 * Raven tutorial 02 (TypeScript) - layout.
 *
 * A panel arranges its children; each child carries a *slot* that says how. In
 * XML a slot is an attribute starting with "Slot.":
 *
 *   Slot.Padding   space around the child
 *   Slot.Fill      share of the leftover space (1 = one share)
 *   Slot.HAlign    where the child sits horizontally in its cell
 *   Slot.VAlign    the same, vertically
 *
 * "Spacing" on a panel is the gap between its children. Almost no TypeScript is
 * needed for this: the step is the XML.
 *
 * Run:  deno task 02        Test: deno task 02 --test
 */

import * as r from "../raven.ts";
import * as t from "../tutorial.ts";

const XML = `
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
</Window>`;

function main(): number {
  if (!t.init("Raven tutorial 02")) return 1;

  const window = r.windowFromXml(XML);
  t.require(window !== r.INVALID_ID, "the window could not be built");
  const status = r.find(window, "status");

  r.actionBind("layout.clear", () => r.setText(status, "Nothing pressed yet."));

  if (t.isTest()) {
    r.drawFrame();

    // The three buttons of the first row: same width, left to right.
    const equal = r.find(window, "equal");
    const [firstX, , firstWidth] = r.rect(r.getChild(equal, 0));
    t.check(firstWidth > 0, "the first button has a width");

    let previousX = firstX;
    for (const index of [1, 2]) {
      const [x, , width] = r.rect(r.getChild(equal, index));
      t.check(Math.abs(width - firstWidth) < 1, "the three buttons are equally wide");
      t.check(x > previousX, "the buttons are placed left to right");
      previousX = x;
    }

    // The footer is below the row of buttons.
    const [, footerY] = r.rect(r.find(window, "footer"));
    const [, rowY, , rowHeight] = r.rect(equal);
    t.check(footerY > rowY + rowHeight, "the spacer pushes the footer to the bottom");

    // The Clear action resets the label.
    r.setText(status, "pressed");
    r.actionFire("layout.clear");
    t.check(r.getText(status) === "Nothing pressed yet.", "the action resets the status label");

    return t.finish("02-layout");
  }

  while (r.waitFrame(-1.0)) {}
  return t.finish("02-layout");
}

Deno.exit(main());
