/**
 * What every step of the TypeScript course needs: start Raven, drive widgets
 * without a window, count the checks, and shut down cleanly.
 *
 * A program ends with
 *
 *     Deno.exit(t.finish("01-first-window"));
 */

import * as r from "./raven.ts";

let checks = 0;
let failures = 0;

/** True when the program was run with `--test`. */
export function isTest(): boolean {
  return Deno.args.includes("--test");
}

/** One assertion. The counts are printed by `finish`. */
export function check(condition: boolean, message: string): void {
  checks += 1;
  if (!condition) {
    failures += 1;
    console.log("  FAIL:", message);
  }
}

/** A condition the rest of the step depends on: stop here when it is false. */
export function require(condition: boolean, message: string): void {
  if (!condition) {
    console.log("  FAIL:", message);
    r.shutdown();
    Deno.exit(1);
  }
}

/**
 * Starts Raven: a real window normally, the null backend with `--test`.
 * Returns false when the library cannot start (no Vulkan driver, typically).
 */
export function init(appName: string): boolean {
  if (!r.init(isTest())) {
    console.log(`${appName}: cannot start Raven (is a Vulkan driver installed?)`);
    return false;
  }
  if (isTest()) r.setAnimationsEnabled(false);
  return true;
}

/** Moves the pointer to the middle of a widget and clicks it. */
export function click(window: number, widget: number): void {
  const [x, y, width, height] = r.rect(widget);
  clickAt(window, x + width / 2, y + height / 2);
}

/** Clicks a point of the window, in logical units. */
export function clickAt(window: number, x: number, y: number): void {
  r.mouseMove(window, x, y);
  r.mouseButton(window, 0, true);
  r.mouseButton(window, 0, false);
}

/** Presses and releases a key, with modifiers (`R_MOD_CTRL`, `R_MOD_SHIFT`...). */
export function key(window: number, keyCode: number, modifiers = 0): void {
  r.key(window, keyCode, modifiers);
}

/** Stops the library, prints the result, and returns the process exit code. */
export function finish(name: string): number {
  r.shutdown();
  if (checks > 0) {
    console.log(`${name}: ${checks} checks, ${failures} failures`);
    console.log(failures === 0 ? "PASS" : "FAIL");
  }
  return failures === 0 ? 0 : 1;
}
