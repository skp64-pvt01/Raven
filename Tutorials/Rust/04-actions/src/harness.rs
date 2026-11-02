//! What every step needs: start Raven, drive widgets without a window, count
//! the checks, and shut down cleanly.
//!
//! This file is repeated in every step on purpose: each folder is a complete
//! project you can copy and run on its own.

#![allow(dead_code)]

use raven::*;
use std::sync::atomic::{AtomicUsize, Ordering};

static CHECKS: AtomicUsize = AtomicUsize::new(0);
static FAILURES: AtomicUsize = AtomicUsize::new(0);

pub fn is_test() -> bool {
    std::env::args().any(|argument| argument == "--test")
}

/// Starts Raven: a real window normally, the null backend with `--test`.
pub fn init(app_name: &str) -> bool {
    let test = is_test();
    unsafe {
        let mut desc: RInitDesc = std::mem::zeroed();
        desc.backend = if test { R_BACKEND_NULL } else { R_BACKEND_VULKAN };
        desc.headless = test as i32;
        if Rinit_ex(&desc) == 0 {
            eprintln!("{app_name}: cannot start Raven (is a Vulkan driver installed?)");
            return false;
        }
        if test {
            Rset_animations_enabled(0);
        }
    }
    true
}

/// One assertion. The counts are printed by [`finish`].
pub fn check(condition: bool, message: &str) {
    CHECKS.fetch_add(1, Ordering::Relaxed);
    if !condition {
        FAILURES.fetch_add(1, Ordering::Relaxed);
        println!("  FAIL: {message}");
    }
}

/// A condition the rest of the step depends on: stop here when it is false.
pub fn require(condition: bool, message: &str) {
    if !condition {
        println!("  FAIL: {message}");
        unsafe { Rshutdown() };
        std::process::exit(1);
    }
}

/// Bounds of a widget in its window (x, y, width, height).
pub fn rect(widget: Rid) -> (f32, f32, f32, f32) {
    let (mut x, mut y, mut width, mut height) = (0.0f32, 0.0f32, 0.0f32, 0.0f32);
    unsafe { Rwidget_get_rect(widget, &mut x, &mut y, &mut width, &mut height) };
    (x, y, width, height)
}

/// Moves the pointer to the middle of a widget and clicks it.
pub fn click(window: Rid, widget: Rid) {
    let (mut x, mut y, mut width, mut height) = (0.0f32, 0.0f32, 0.0f32, 0.0f32);
    unsafe {
        Rwidget_get_rect(widget, &mut x, &mut y, &mut width, &mut height);
        Rwindow_inject_mouse_move(window, x + width / 2.0, y + height / 2.0);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
    }
}

/// Clicks a point of the window, in logical units.
pub fn click_at(window: Rid, x: f32, y: f32) {
    unsafe {
        Rwindow_inject_mouse_move(window, x, y);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
    }
}

/// Presses and releases a key, with modifiers (`R_MOD_CTRL`, `R_MOD_SHIFT`...).
pub fn key(window: Rid, key: RKey, modifiers: RModifier) {
    unsafe {
        Rwindow_inject_key(window, key, 1, modifiers as u32);
        Rwindow_inject_key(window, key, 0, modifiers as u32);
    }
}

/// Stops the library, prints the result, and returns the process exit code.
pub fn finish(name: &str) -> i32 {
    unsafe { Rshutdown() };
    let checks = CHECKS.load(Ordering::Relaxed);
    let failures = FAILURES.load(Ordering::Relaxed);
    if checks > 0 {
        println!("{name}: {checks} checks, {failures} failures");
        println!("{}", if failures == 0 { "PASS" } else { "FAIL" });
    }
    if failures == 0 {
        0
    } else {
        1
    }
}
