/**
 * Runs every step's --test mode and reports the result.
 *
 *     deno task test
 *
 * The environment must reach Raven: RAVEN_LIBRARY pointing at libraven.so
 * (see README.md).
 */

const steps = [
  "01-first-window",
  "02-layout",
  "03-controls",
  "04-actions",
  "05-xml-files",
  "06-menus",
  "07-text-editor",
  "08-capstone-notes",
];

let failed = 0;

for (const step of steps) {
  console.log(`=== ${step}`);
  const result = new Deno.Command(Deno.execPath(), {
    args: [
      "run",
      "--allow-ffi",
      "--allow-env",
      "--allow-read",
      "--allow-write",
      `${step}/main.ts`,
      "--test",
    ],
    stdout: "inherit",
    stderr: "inherit",
  }).outputSync();
  if (!result.success) failed += 1;
}

console.log();
console.log(failed ? `${failed} step(s) failed` : "All steps passed.");
Deno.exit(failed ? 1 : 0);
