//! Raven example - instrument panel (Rust).
//!
//! A small process-control dashboard: an analog gauge, a digital meter,
//! indicator LEDs, push-button switches and a real-time strip chart. Every
//! instrument is a custom widget painted by the application (raven_draw.h);
//! Raven provides the layout, the theme, the input and the primitives.
//!
//! ```sh
//! RAVEN_LIB_DIR=/path/to/lib cargo run --release            # the dashboard
//! RAVEN_LIB_DIR=/path/to/lib cargo run --release -- --test  # checks only
//! RAVEN_LIB_DIR=/path/to/lib cargo run --release -- --capture panel.ppm
//! ```
//!
//! See the README one folder above.

use raven::*;
use std::ffi::c_void;
use std::os::raw::c_int;

const SAMPLES: usize = 300; // ten seconds at 30 Hz
const SAMPLE_INTERVAL: f64 = 1.0 / 30.0;

#[derive(Clone, Copy, Default)]
struct Color {
    r: f32,
    g: f32,
    b: f32,
    a: f32,
}

struct Led {
    on: bool,
    r: f32,
    g: f32,
    b: f32,
}

struct Panel {
    running: bool,
    pump: bool,
    valve: bool,
    armed: bool,
    time: f64,
    pressure: f32,
    flow: f32,

    pressure_history: [f32; SAMPLES],
    flow_history: [f32; SAMPLES],
    sample_count: usize,
    sample_head: usize,
    last_sample: f64,

    window: Rid,
    status: Rid,
    alarm_label: Rid,
    pump_button: Rid,
    valve_button: Rid,
    arm_button: Rid,
    pause_button: Rid,

    meter_paints: u32,
    digital_paints: u32,
    chart_paints: u32,
    led_paints: u32,
}

impl Default for Panel {
    fn default() -> Self {
        Self {
            running: true,
            pump: false,
            valve: false,
            armed: false,
            time: 0.0,
            pressure: 2.0,
            flow: 0.0,
            pressure_history: [0.0; SAMPLES],
            flow_history: [0.0; SAMPLES],
            sample_count: 0,
            sample_head: 0,
            last_sample: 0.0,
            window: R_INVALID_ID,
            status: R_INVALID_ID,
            alarm_label: R_INVALID_ID,
            pump_button: R_INVALID_ID,
            valve_button: R_INVALID_ID,
            arm_button: R_INVALID_ID,
            pause_button: R_INVALID_ID,
            meter_paints: 0,
            digital_paints: 0,
            chart_paints: 0,
            led_paints: 0,
        }
    }
}

// The panel is one global object, handed to the C callbacks as their user_data.
// A single-window dashboard has no other owner, and the paint callbacks only
// run on the UI thread.
static mut PANEL: *mut Panel = std::ptr::null_mut();

fn panel() -> &'static mut Panel {
    unsafe { &mut *PANEL }
}

static mut POWER: Led = Led { on: true, r: 0.30, g: 0.90, b: 0.45 };
static mut LINK: Led = Led { on: true, r: 0.30, g: 0.65, b: 1.00 };
static mut ALARM: Led = Led { on: false, r: 1.00, g: 0.30, b: 0.25 };

fn alarm_active() -> bool {
    let p = panel();
    p.armed && p.pressure > 75.0
}

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */

fn role(widget: Rid, which: RColorRole) -> Color {
    let (mut r, mut g, mut b, mut a) = (0.0f32, 0.0f32, 0.0f32, 0.0f32);
    unsafe { Rwidget_get_color(widget, which, &mut r, &mut g, &mut b, &mut a) };
    Color { r, g, b, a }
}

fn clamp01(value: f32) -> f32 {
    value.clamp(0.0, 1.0)
}

/// Raven has no text measuring call here; the panel only centres short strings.
fn text_width(text: &str, size: f32) -> f32 {
    text.chars().count() as f32 * size * 0.60
}

fn draw_text(painter: *mut RPainter, font: Rid, size: f32, x: f32, y: f32, text: &str, color: Color) {
    unsafe {
        Rdraw_text(painter, font, size, x, y, cstr(text).as_ptr(), color.r, color.g, color.b, color.a)
    };
}

fn draw_text_centered(
    painter: *mut RPainter,
    font: Rid,
    size: f32,
    center_x: f32,
    y: f32,
    text: &str,
    color: Color,
) {
    draw_text(painter, font, size, center_x - text_width(text, size) * 0.5, y, text, color);
}

/* ---------------------------------------------------------------------------
 *  Simulation
 * ------------------------------------------------------------------------- */

fn simulate(delta: f64) {
    let p = panel();
    if !p.running || delta <= 0.0 {
        return;
    }
    p.time += delta;
    let t = p.time as f32;

    let mut pressure_target = if p.pump { 68.0 } else { 2.0 };
    if p.pump {
        pressure_target += 2.5 * (t * 1.7).sin();
    }
    let mut flow_target = if p.valve { if p.pump { 44.0 } else { 12.0 } } else { 0.0 };
    if p.valve {
        flow_target += 3.0 * (t * 3.1).sin();
    }

    p.pressure += (pressure_target - p.pressure) * (1.0 - (-(delta as f32) / 0.6).exp());
    p.flow += (flow_target - p.flow) * (1.0 - (-(delta as f32) / 0.35).exp());
    p.pressure = p.pressure.max(0.0);
    p.flow = p.flow.max(0.0);

    if p.time - p.last_sample >= SAMPLE_INTERVAL {
        p.last_sample = p.time;
        p.pressure_history[p.sample_head] = p.pressure;
        p.flow_history[p.sample_head] = p.flow;
        p.sample_head = (p.sample_head + 1) % SAMPLES;
        if p.sample_count < SAMPLES {
            p.sample_count += 1;
        }
    }

    unsafe { ALARM.on = p.armed && p.pressure > 75.0 };
}

fn update_status() {
    let p = panel();
    let text = format!(
        "P-101  {:5.1} bar      F-201  {:5.1} L/min      {}",
        p.pressure,
        p.flow,
        if p.running { "running" } else { "paused" }
    );
    unsafe {
        Rlabel_set_text(p.status, cstr(&text).as_ptr());
        let (message, color) = if p.armed && p.pressure > 75.0 {
            ("HIGH PRESSURE", Color { r: 1.0, g: 0.35, b: 0.30, a: 1.0 })
        } else if !p.armed {
            ("interlocks off", Color { r: 0.75, g: 0.75, b: 0.75, a: 0.8 })
        } else {
            ("interlocks armed", Color { r: 0.45, g: 0.85, b: 0.50, a: 0.9 })
        };
        Rlabel_set_text(p.alarm_label, cstr(message).as_ptr());
        Rlabel_set_color(p.alarm_label, color.r, color.g, color.b, color.a);
    }
}

/* ---------------------------------------------------------------------------
 *  Analog meter
 * ------------------------------------------------------------------------- */

unsafe extern "C" fn paint_meter(widget: Rid, painter: *mut RPainter, width: f32, height: f32, _data: *mut c_void) {
    let p = panel();
    p.meter_paints += 1;

    let surface = role(widget, R_COLOR_PANEL);
    let border = role(widget, R_COLOR_BORDER);
    let text = role(widget, R_COLOR_TEXT);
    let accent = role(widget, R_COLOR_ACCENT);
    let danger = role(widget, R_COLOR_DANGER);
    let track = role(widget, R_COLOR_TRACK);

    Rdraw_rect(painter, 0.0, 0.0, width, height, 14.0, surface.r, surface.g, surface.b, 0.85);
    Rdraw_rect_outline(painter, 0.5, 0.5, width - 1.0, height - 1.0, 14.0, 1.0, border.r, border.g, border.b, 0.7);

    let center_x = width * 0.5;
    let center_y = height * 0.58;
    let radius = width.min(height) * 0.36;
    let start = 2.35619449; // 135 degrees: bottom left
    let sweep = 4.71238898; // 270 degrees, through the top

    for i in 0..=10 {
        let angle = start + sweep * i as f32 / 10.0;
        let major = i % 5 == 0;
        let inner = radius * if major { 0.80 } else { 0.88 };
        Rdraw_line(
            painter,
            center_x + angle.cos() * inner,
            center_y + angle.sin() * inner,
            center_x + angle.cos() * radius,
            center_y + angle.sin() * radius,
            if major { 2.0 } else { 1.0 },
            text.r,
            text.g,
            text.b,
            if major { 0.9 } else { 0.4 },
        );
    }

    let value = clamp01(p.pressure / 100.0);
    for i in 0..60 {
        let a0 = start + sweep * i as f32 / 60.0;
        let a1 = start + sweep * (i + 1) as f32 / 60.0;
        let lit = (i + 1) as f32 / 60.0 <= value;
        let color = if lit { accent } else { track };
        Rdraw_line(
            painter,
            center_x + a0.cos() * radius * 0.92,
            center_y + a0.sin() * radius * 0.92,
            center_x + a1.cos() * radius * 0.92,
            center_y + a1.sin() * radius * 0.92,
            4.0,
            color.r,
            color.g,
            color.b,
            color.a * if lit { 0.9 } else { 0.5 },
        );
    }

    let angle = start + sweep * value;
    let needle = if p.pressure > 75.0 { danger } else { text };
    Rdraw_line(
        painter,
        center_x,
        center_y,
        center_x + angle.cos() * radius * 0.78,
        center_y + angle.sin() * radius * 0.78,
        3.0,
        needle.r,
        needle.g,
        needle.b,
        1.0,
    );
    Rdraw_circle(painter, center_x, center_y, 6.0, needle.r, needle.g, needle.b, 1.0);
    Rdraw_circle(painter, center_x, center_y, 2.5, surface.r, surface.g, surface.b, 1.0);

    draw_text(painter, R_INVALID_ID, 12.0, 14.0, 10.0, "PRESSURE  P-101", text);
    draw_text_centered(
        painter,
        Rfont_get_monospace(),
        26.0,
        center_x,
        height - 40.0,
        &format!("{:.1} bar", p.pressure),
        accent,
    );
}

/* ---------------------------------------------------------------------------
 *  Digital meter
 * ------------------------------------------------------------------------- */

unsafe extern "C" fn paint_digital(widget: Rid, painter: *mut RPainter, width: f32, height: f32, _data: *mut c_void) {
    let p = panel();
    p.digital_paints += 1;

    let surface = role(widget, R_COLOR_PANEL);
    let border = role(widget, R_COLOR_BORDER);
    let text = role(widget, R_COLOR_TEXT);
    let accent = role(widget, R_COLOR_ACCENT);

    Rdraw_rect(painter, 0.0, 0.0, width, height, 14.0, surface.r, surface.g, surface.b, 0.85);
    Rdraw_rect_outline(painter, 0.5, 0.5, width - 1.0, height - 1.0, 14.0, 1.0, border.r, border.g, border.b, 0.7);

    Rdraw_rect(painter, 14.0, 34.0, width - 28.0, height - 62.0, 8.0, 0.05, 0.07, 0.09, 0.85);
    Rdraw_rect_outline(painter, 14.5, 34.5, width - 29.0, height - 63.0, 8.0, 1.0, border.r, border.g, border.b, 0.5);

    let size = height * 0.26;
    draw_text_centered(
        painter,
        Rfont_get_monospace(),
        size,
        width * 0.5,
        34.0 + (height - 62.0 - size) * 0.45,
        &format!("{:5.1}", p.flow),
        accent,
    );

    draw_text(painter, R_INVALID_ID, 12.0, 14.0, 10.0, "FLOW  F-201", text);
    draw_text(painter, R_INVALID_ID, 12.0, width - 58.0, 10.0, "L/min", text);

    Rdraw_rect(painter, 20.0, height - 18.0, width - 40.0, 3.0, 1.5, border.r, border.g, border.b, 0.35);
    Rdraw_rect(
        painter,
        20.0,
        height - 18.0,
        (width - 40.0) * clamp01(p.flow / 100.0),
        3.0,
        1.5,
        accent.r,
        accent.g,
        accent.b,
        0.9,
    );
}

/* ---------------------------------------------------------------------------
 *  LED
 * ------------------------------------------------------------------------- */

unsafe extern "C" fn paint_led(_widget: Rid, painter: *mut RPainter, width: f32, height: f32, data: *mut c_void) {
    let led = &*(data as *const Led);
    panel().led_paints += 1;

    let center_x = width * 0.5;
    let center_y = height * 0.5;
    let radius = width.min(height) * 0.30;
    let strength = if led.on { 1.0 } else { 0.0 };

    for i in 0..3 {
        let r = radius * (1.4 + 0.35 * i as f32);
        Rdraw_circle(painter, center_x, center_y, r, led.r, led.g, led.b, (0.10 - 0.03 * i as f32) * strength);
    }
    Rdraw_circle(
        painter,
        center_x,
        center_y,
        radius,
        led.r * (0.35 + 0.65 * strength),
        led.g * (0.35 + 0.65 * strength),
        led.b * (0.35 + 0.65 * strength),
        0.95,
    );
    Rdraw_circle(
        painter,
        center_x - radius * 0.25,
        center_y - radius * 0.30,
        radius * 0.28,
        1.0,
        1.0,
        1.0,
        0.18 + 0.22 * strength,
    );
}

/* ---------------------------------------------------------------------------
 *  Real-time strip chart
 * ------------------------------------------------------------------------- */

unsafe extern "C" fn paint_chart(widget: Rid, painter: *mut RPainter, width: f32, height: f32, _data: *mut c_void) {
    let p = panel();
    p.chart_paints += 1;

    let surface = role(widget, R_COLOR_PANEL);
    let border = role(widget, R_COLOR_BORDER);
    let text = role(widget, R_COLOR_TEXT);
    let accent = role(widget, R_COLOR_ACCENT);
    let success = role(widget, R_COLOR_SUCCESS);

    Rdraw_rect(painter, 0.0, 0.0, width, height, 14.0, surface.r, surface.g, surface.b, 0.85);
    Rdraw_rect_outline(painter, 0.5, 0.5, width - 1.0, height - 1.0, 14.0, 1.0, border.r, border.g, border.b, 0.7);

    let (left, right, top, bottom) = (52.0f32, 18.0f32, 34.0f32, 22.0f32);
    let plot_width = width - left - right;
    let plot_height = height - top - bottom;

    draw_text(painter, R_INVALID_ID, 12.0, 14.0, 10.0, "TREND  10 s", text);
    Rdraw_rect(painter, width - 128.0, 14.0, 9.0, 9.0, 2.0, accent.r, accent.g, accent.b, 0.9);
    draw_text(painter, R_INVALID_ID, 11.0, width - 114.0, 11.0, "pressure", text);
    Rdraw_rect(painter, width - 52.0, 14.0, 9.0, 9.0, 2.0, success.r, success.g, success.b, 0.9);
    draw_text(painter, R_INVALID_ID, 11.0, width - 38.0, 11.0, "flow", text);

    for i in 0..=4 {
        let y = top + plot_height * i as f32 / 4.0;
        Rdraw_line(painter, left, y, left + plot_width, y, 1.0, border.r, border.g, border.b, if i == 4 { 0.6 } else { 0.22 });
        draw_text(painter, Rfont_get_monospace(), 10.0, 16.0, y - 6.0, &format!("{}", 100 - 25 * i), text);
    }
    for i in 0..=10 {
        let x = left + plot_width * i as f32 / 10.0;
        Rdraw_line(painter, x, top, x, top + plot_height, 1.0, border.r, border.g, border.b, 0.12);
    }

    if p.sample_count < 2 {
        return;
    }

    let step_x = plot_width / (SAMPLES - 1) as f32;
    let mut previous_pressure = (0.0f32, 0.0f32);
    let mut previous_flow = (0.0f32, 0.0f32);

    for i in 0..p.sample_count {
        let index = (p.sample_head + SAMPLES - p.sample_count + i) % SAMPLES;
        let x = left + plot_width - step_x * (p.sample_count - 1 - i) as f32;
        let pressure_y = top + plot_height * (1.0 - clamp01(p.pressure_history[index] / 100.0));
        let flow_y = top + plot_height * (1.0 - clamp01(p.flow_history[index] / 100.0));

        if i > 0 {
            Rdraw_line(painter, previous_pressure.0, previous_pressure.1, x, pressure_y, 1.8, accent.r, accent.g, accent.b, 0.95);
            Rdraw_line(painter, previous_flow.0, previous_flow.1, x, flow_y, 1.8, success.r, success.g, success.b, 0.95);
        }
        previous_pressure = (x, pressure_y);
        previous_flow = (x, flow_y);
    }

    if p.armed {
        let y = top + plot_height * (1.0 - 0.75);
        Rdraw_line(painter, left, y, left + plot_width, y, 1.5, 0.95, 0.35, 0.30, 0.5);
        draw_text(painter, R_INVALID_ID, 10.0, left + 6.0, y - 14.0, "alarm 75", text);
    }
}

/* ---------------------------------------------------------------------------
 *  UI
 * ------------------------------------------------------------------------- */

fn tile() -> Rid {
    let column = unsafe { Rwidget_create(R_WIDGET_VBOX) };
    unsafe {
        Rpanel_set_spacing(column, 8.0);
        Rslot_set_padding(column, 12.0, 12.0, 12.0, 12.0);
    }
    column
}

fn custom_panel(paint: RPaintCallback, min_width: f32, min_height: f32, data: *mut c_void) -> Rid {
    let widget = unsafe { Rwidget_create(R_WIDGET_CUSTOM) };
    unsafe {
        Rcustom_set_paint_callback(widget, paint, data);
        Rwidget_set_min_size(widget, min_width, min_height);
        Rwidget_set_hit_testable(widget, 0);
    }
    widget
}

fn sync_controls() {
    let p = panel();
    unsafe {
        Rbutton_set_text(p.pump_button, cstr(if p.pump { "PUMP  ON" } else { "PUMP  OFF" }).as_ptr());
        Rbutton_set_style(p.pump_button, if p.pump { R_BUTTON_PRIMARY } else { R_BUTTON_DEFAULT });
        Rbutton_set_text(p.valve_button, cstr(if p.valve { "VALVE  OPEN" } else { "VALVE  SHUT" }).as_ptr());
        Rbutton_set_style(p.valve_button, if p.valve { R_BUTTON_PRIMARY } else { R_BUTTON_DEFAULT });
        Rbutton_set_text(p.arm_button, cstr(if p.armed { "INTERLOCKS  ARMED" } else { "INTERLOCKS  OFF" }).as_ptr());
        Rbutton_set_style(p.arm_button, if p.armed { R_BUTTON_PRIMARY } else { R_BUTTON_DEFAULT });
        Rbutton_set_text(p.pause_button, cstr(if p.running { "PAUSE" } else { "RESUME" }).as_ptr());
    }
}

unsafe extern "C" fn on_pump(_button: Rid, _data: *mut c_void) {
    panel().pump = !panel().pump;
    sync_controls();
}

unsafe extern "C" fn on_valve(_button: Rid, _data: *mut c_void) {
    panel().valve = !panel().valve;
    sync_controls();
}

unsafe extern "C" fn on_arm(_button: Rid, _data: *mut c_void) {
    panel().armed = !panel().armed;
    sync_controls();
    update_status();
}

unsafe extern "C" fn on_pause(_button: Rid, _data: *mut c_void) {
    panel().running = !panel().running;
    sync_controls();
    update_status();
}

fn build_panel(window: Rid) -> Rid {
    let p = panel();
    unsafe {
        let root = Rwidget_create(R_WIDGET_VBOX);
        Rpanel_set_spacing(root, 12.0);
        Rslot_set_padding(root, 16.0, 16.0, 16.0, 16.0);
        Rwindow_set_content(window, root);

        let row = Rwidget_add_child(root, Rwidget_create(R_WIDGET_HBOX));
        Rpanel_set_spacing(row, 12.0);

        let gauge_tile = Rwidget_add_child(row, tile());
        Rslot_set_fill(gauge_tile, 1.0);
        let gauge = Rwidget_add_child(gauge_tile, custom_panel(Some(paint_meter), 240.0, 240.0, std::ptr::null_mut()));
        Rslot_set_fill(gauge, 1.0);

        let flow_tile = Rwidget_add_child(row, tile());
        Rslot_set_fill(flow_tile, 1.0);
        let flow = Rwidget_add_child(flow_tile, custom_panel(Some(paint_digital), 240.0, 150.0, std::ptr::null_mut()));
        Rslot_set_fill(flow, 1.0);

        let control_tile = Rwidget_add_child(row, tile());
        Rslot_set_fill(control_tile, 1.0);

        let leds = Rwidget_add_child(control_tile, Rwidget_create(R_WIDGET_HBOX));
        Rpanel_set_spacing(leds, 10.0);
        Rslot_set_halign(leds, R_HALIGN_CENTER);

        let lamps: [(*mut Led, &str); 3] = [
            (&raw mut POWER, "POWER"),
            (&raw mut LINK, "LINK"),
            (&raw mut ALARM, "ALARM"),
        ];
        for (led, name) in lamps {
            let column = Rwidget_add_child(leds, Rwidget_create(R_WIDGET_VBOX));
            Rpanel_set_spacing(column, 2.0);
            let lamp = Rwidget_add_child(
                column,
                custom_panel(Some(paint_led), 46.0, 46.0, led as *mut c_void),
            );
            Rwidget_set_size(lamp, 46.0, 46.0);
            let label = Rwidget_add_child(column, Rlabel_create(cstr(name).as_ptr()));
            Rlabel_set_text_size(label, 10.0);
            Rslot_set_halign(label, R_HALIGN_CENTER);
        }

        p.alarm_label = Rwidget_add_child(control_tile, Rlabel_create(cstr("interlocks off").as_ptr()));
        Rlabel_set_text_size(p.alarm_label, 12.0);
        Rslot_set_halign(p.alarm_label, R_HALIGN_CENTER);

        p.pump_button = Rwidget_add_child(control_tile, Rbutton_create(cstr("PUMP  OFF").as_ptr()));
        Rbutton_set_clicked_callback(p.pump_button, Some(on_pump), std::ptr::null_mut());
        p.valve_button = Rwidget_add_child(control_tile, Rbutton_create(cstr("VALVE  SHUT").as_ptr()));
        Rbutton_set_clicked_callback(p.valve_button, Some(on_valve), std::ptr::null_mut());
        p.arm_button = Rwidget_add_child(control_tile, Rbutton_create(cstr("INTERLOCKS  OFF").as_ptr()));
        Rbutton_set_clicked_callback(p.arm_button, Some(on_arm), std::ptr::null_mut());

        let spacer = Rwidget_add_child(control_tile, Rwidget_create(R_WIDGET_SPACER));
        Rslot_set_fill(spacer, 1.0);

        let chart_tile = Rwidget_add_child(root, tile());
        Rslot_set_fill(chart_tile, 1.0);
        let chart = Rwidget_add_child(chart_tile, custom_panel(Some(paint_chart), 480.0, 180.0, std::ptr::null_mut()));
        Rslot_set_fill(chart, 1.0);

        let bar = Rwidget_add_child(root, Rwidget_create(R_WIDGET_HBOX));
        Rpanel_set_spacing(bar, 12.0);
        p.status = Rwidget_add_child(bar, Rlabel_create(cstr("starting").as_ptr()));
        Rslot_set_fill(p.status, 1.0);
        Rslot_set_valign(p.status, R_VALIGN_CENTER);

        p.pause_button = Rwidget_add_child(bar, Rbutton_create(cstr("PAUSE").as_ptr()));
        Rbutton_set_clicked_callback(p.pause_button, Some(on_pause), std::ptr::null_mut());

        sync_controls();
        root
    }
}

unsafe extern "C" fn on_frame(delta: f64, _data: *mut c_void) {
    simulate(delta);
    update_status();
    Rwindow_request_redraw(panel().window);
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

fn init(headless: bool) -> bool {
    unsafe {
        let mut desc: RInitDesc = std::mem::zeroed();
        desc.backend = if headless { R_BACKEND_NULL } else { R_BACKEND_VULKAN };
        desc.headless = headless as c_int;
        if Rinit_ex(&desc) == 0 {
            eprintln!("cannot start Raven (is a Vulkan driver installed?)");
            return false;
        }
    }
    true
}

fn save_ppm(window: Rid, path: &str) -> bool {
    let (mut width, mut height) = (0i32, 0i32);
    let pixels = unsafe { Rwindow_capture(window, &mut width, &mut height) };
    if pixels.is_null() {
        return false;
    }
    let mut out = format!("P6\n{} {}\n255\n", width, height).into_bytes();
    let count = (width * height) as usize;
    for i in 0..count {
        out.extend_from_slice(unsafe { std::slice::from_raw_parts(pixels.add(i * 4), 3) });
    }
    unsafe { Rpixels_free(pixels) };
    std::fs::write(path, out).is_ok()
}

fn create_window(title: &str) -> Rid {
    unsafe { Rwindow_create(cstr(title).as_ptr(), 1180, 760, R_WINDOW_DEFAULT as u32) }
}

fn run_tests() -> i32 {
    unsafe { Rtheme_apply(R_THEME_DARK) };
    let window = create_window("Instrument panel");
    panel().window = window;
    build_panel(window);
    update_status();
    unsafe { Rdraw_frame() };

    {
        let p = panel();
        if p.meter_paints == 0 || p.digital_paints == 0 || p.chart_paints == 0 || p.led_paints == 0 {
            println!("  FAIL: a custom widget was never painted");
            return 1;
        }
    }

    let pressure_at_rest = panel().pressure;
    panel().pump = true;
    for _ in 0..180 {
        simulate(1.0 / 60.0);
    }
    if panel().pressure <= pressure_at_rest + 20.0 {
        println!("  FAIL: the pump did not raise the pressure ({:.1})", panel().pressure);
        return 1;
    }

    panel().valve = true;
    for _ in 0..120 {
        simulate(1.0 / 60.0);
    }
    if panel().flow <= 20.0 {
        println!("  FAIL: the valve did not let the flow through ({:.1})", panel().flow);
        return 1;
    }

    if panel().sample_count < 20 {
        println!("  FAIL: the chart has {} samples", panel().sample_count);
        return 1;
    }

    panel().armed = true;
    panel().pressure = 80.0;
    simulate(1.0 / 60.0);
    if !alarm_active() || !unsafe { ALARM.on } {
        println!("  FAIL: the alarm did not trip");
        return 1;
    }

    panel().pressure = 2.0;
    unsafe { Rdraw_frame() };
    unsafe {
        let (mut x, mut y, mut w, mut h) = (0.0f32, 0.0f32, 0.0f32, 0.0f32);
        Rwidget_get_rect(panel().pump_button, &mut x, &mut y, &mut w, &mut h);
        let before = panel().pump;
        Rwindow_inject_mouse_move(window, x + w * 0.5, y + h * 0.5);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
        if panel().pump == before {
            println!("  FAIL: the pump button did not toggle");
            return 1;
        }
    }

    panel().running = false;
    let frozen = panel().time;
    simulate(1.0);
    if panel().time != frozen {
        println!("  FAIL: the simulation ran while paused");
        return 1;
    }

    println!(
        "instrument panel: simulated {:.1} s, {} samples, {} meter paints, {} chart paints",
        panel().time,
        panel().sample_count,
        panel().meter_paints,
        panel().chart_paints
    );
    unsafe { Rshutdown() };
    println!("PASS");
    0
}

fn run_capture(path: &str, frames: i32) -> i32 {
    unsafe { Rtheme_apply(R_THEME_DARK) };
    let window = create_window("Instrument panel");
    panel().window = window;
    build_panel(window);
    panel().pump = true;
    panel().valve = true;
    panel().armed = true;
    sync_controls();
    update_status();

    for _ in 0..frames {
        simulate(1.0 / 60.0);
        update_status();
        unsafe {
            Rwindow_request_redraw(window);
            Rwait_frame(0.0);
        }
    }

    let saved = save_ppm(window, path);
    unsafe { Rshutdown() };
    if !saved {
        eprintln!("cannot write {path}");
        return 1;
    }
    println!("saved {path}");
    0
}

fn main() {
    let arguments: Vec<String> = std::env::args().collect();
    let test = arguments.iter().any(|a| a == "--test");
    let capture = arguments.iter().position(|a| a == "--capture").and_then(|i| arguments.get(i + 1)).cloned();
    let frames = arguments
        .iter()
        .position(|a| a == "--frames")
        .and_then(|i| arguments.get(i + 1))
        .and_then(|value| value.parse().ok())
        .unwrap_or(180);

    unsafe { PANEL = Box::into_raw(Box::new(Panel::default())) };

    let code = if test {
        if init(true) {
            run_tests()
        } else {
            1
        }
    } else if !init(false) {
        1
    } else if let Some(path) = capture {
        run_capture(&path, frames)
    } else {
        unsafe {
            Rtheme_apply(R_THEME_DARK);
            let window = create_window("Instrument panel");
            panel().window = window;
            build_panel(window);
            update_status();
            Rset_frame_callback(Some(on_frame), std::ptr::null_mut());
            while Rwait_frame(-1.0) != 0 {}
            Rshutdown();
        }
        0
    };

    unsafe { drop(Box::from_raw(PANEL)) };
    std::process::exit(code);
}
