"""
Raven example - instrument panel (Python).

A small process-control dashboard: an analog gauge, a digital meter, indicator
LEDs, push-button switches and a real-time strip chart. Every instrument is a
custom widget painted by the application (raven_draw.h); Raven provides the
layout, the theme, the input and the primitives.

    python3 panel.py                 the dashboard
    python3 panel.py --test          no window, no GPU: checks only
    python3 panel.py --capture a.ppm 200 frames, saved as a PPM image

Needs the Raven shared library (RAVEN_LIBRARY) and the binding
(raven.py in bindings/python), exactly like the Python course. See the README
one folder above.
"""

import ctypes
import math
import sys

import raven as r

SAMPLES = 300  # ten seconds at 30 Hz
SAMPLE_INTERVAL = 1.0 / 30.0
TAU = 6.283185307179586


class Panel:
    def __init__(self):
        self.running = True
        self.pump = False
        self.valve = False
        self.armed = False
        self.time = 0.0
        self.pressure = 2.0
        self.flow = 0.0
        self.pressure_history = [0.0] * SAMPLES
        self.flow_history = [0.0] * SAMPLES
        self.sample_count = 0
        self.sample_head = 0
        self.last_sample = 0.0
        self.window = r.R_INVALID_ID
        self.status = r.R_INVALID_ID
        self.alarm_label = r.R_INVALID_ID
        self.pump_button = r.R_INVALID_ID
        self.valve_button = r.R_INVALID_ID
        self.arm_button = r.R_INVALID_ID
        self.pause_button = r.R_INVALID_ID
        self.meter_paints = 0
        self.digital_paints = 0
        self.chart_paints = 0
        self.led_paints = 0


PANEL = Panel()

# The lamps, by the widget that paints them: the C callback passes no Python
# object, so the panel looks the lamp up from the widget it is drawing.
LEDS = {}


def led(name, on, red, green, blue):
    return {"name": name, "on": on, "color": (red, green, blue)}


POWER = led("POWER", True, 0.30, 0.90, 0.45)
LINK = led("LINK", True, 0.30, 0.65, 1.00)
ALARM = led("ALARM", False, 1.00, 0.30, 0.25)


def alarm_active():
    return PANEL.armed and PANEL.pressure > 75.0


# ---------------------------------------------------------------------------
#  Helpers
# ---------------------------------------------------------------------------


def role(widget, which):
    """The color the widget is drawn with for a role: (r, g, b, a)."""
    return r.Rwidget_get_color(widget, which)


def clamp01(value):
    return 0.0 if value < 0.0 else (1.0 if value > 1.0 else value)


def text_width(text, size):
    """Raven has no text measuring call here; the panel only centres short strings."""
    return len(text) * size * 0.60


def draw_text(painter, font, size, x, y, text, color):
    r.Rdraw_text(painter, font, size, x, y, text, *color)


def draw_text_centered(painter, font, size, center_x, y, text, color):
    draw_text(painter, font, size, center_x - text_width(text, size) / 2, y, text, color)


# ---------------------------------------------------------------------------
#  Simulation
# ---------------------------------------------------------------------------


def simulate(delta):
    if not PANEL.running or delta <= 0.0:
        return
    PANEL.time += delta
    t = PANEL.time

    pressure_target = 68.0 if PANEL.pump else 2.0
    if PANEL.pump:
        pressure_target += 2.5 * math.sin(t * 1.7)

    flow_target = (44.0 if PANEL.pump else 12.0) if PANEL.valve else 0.0
    if PANEL.valve:
        flow_target += 3.0 * math.sin(t * 3.1)

    PANEL.pressure += (pressure_target - PANEL.pressure) * (1.0 - math.exp(-delta / 0.6))
    PANEL.flow += (flow_target - PANEL.flow) * (1.0 - math.exp(-delta / 0.35))
    PANEL.pressure = max(PANEL.pressure, 0.0)
    PANEL.flow = max(PANEL.flow, 0.0)

    if PANEL.time - PANEL.last_sample >= SAMPLE_INTERVAL:
        PANEL.last_sample = PANEL.time
        PANEL.pressure_history[PANEL.sample_head] = PANEL.pressure
        PANEL.flow_history[PANEL.sample_head] = PANEL.flow
        PANEL.sample_head = (PANEL.sample_head + 1) % SAMPLES
        PANEL.sample_count = min(PANEL.sample_count + 1, SAMPLES)

    ALARM["on"] = alarm_active()


def update_status():
    state = "running" if PANEL.running else "paused"
    r.Rlabel_set_text(
        PANEL.status,
        "P-101  %5.1f bar      F-201  %5.1f L/min      %s" % (PANEL.pressure, PANEL.flow, state),
    )

    if alarm_active():
        r.Rlabel_set_text(PANEL.alarm_label, "HIGH PRESSURE")
        r.Rlabel_set_color(PANEL.alarm_label, 1.0, 0.35, 0.30, 1.0)
    elif not PANEL.armed:
        r.Rlabel_set_text(PANEL.alarm_label, "interlocks off")
        r.Rlabel_set_color(PANEL.alarm_label, 0.75, 0.75, 0.75, 0.8)
    else:
        r.Rlabel_set_text(PANEL.alarm_label, "interlocks armed")
        r.Rlabel_set_color(PANEL.alarm_label, 0.45, 0.85, 0.50, 0.9)


# ---------------------------------------------------------------------------
#  Analog meter
# ---------------------------------------------------------------------------


def paint_meter(widget, painter, width, height, user_data):
    PANEL.meter_paints += 1

    surface = role(widget, r.R_COLOR_PANEL)
    border = role(widget, r.R_COLOR_BORDER)
    text = role(widget, r.R_COLOR_TEXT)
    accent = role(widget, r.R_COLOR_ACCENT)
    danger = role(widget, r.R_COLOR_DANGER)
    track = role(widget, r.R_COLOR_TRACK)

    r.Rdraw_rect(painter, 0, 0, width, height, 14, surface[0], surface[1], surface[2], 0.85)
    r.Rdraw_rect_outline(painter, 0.5, 0.5, width - 1, height - 1, 14, 1,
                         border[0], border[1], border[2], 0.7)

    center_x = width * 0.5
    center_y = height * 0.58
    radius = min(width, height) * 0.36
    start = 2.35619449   # 135 degrees: bottom left
    sweep = 4.71238898   # 270 degrees, through the top

    for i in range(11):
        angle = start + sweep * i / 10.0
        major = i % 5 == 0
        inner = radius * (0.80 if major else 0.88)
        r.Rdraw_line(
            painter,
            center_x + math.cos(angle) * inner, center_y + math.sin(angle) * inner,
            center_x + math.cos(angle) * radius, center_y + math.sin(angle) * radius,
            2.0 if major else 1.0,
            text[0], text[1], text[2], 0.9 if major else 0.4,
        )

    value = clamp01(PANEL.pressure / 100.0)
    for i in range(60):
        a0 = start + sweep * i / 60.0
        a1 = start + sweep * (i + 1) / 60.0
        lit = (i + 1) / 60.0 <= value
        color = accent if lit else track
        r.Rdraw_line(
            painter,
            center_x + math.cos(a0) * radius * 0.92, center_y + math.sin(a0) * radius * 0.92,
            center_x + math.cos(a1) * radius * 0.92, center_y + math.sin(a1) * radius * 0.92,
            4.0, color[0], color[1], color[2], color[3] * (0.9 if lit else 0.5),
        )

    angle = start + sweep * value
    needle = danger if PANEL.pressure > 75.0 else text
    r.Rdraw_line(painter, center_x, center_y,
                 center_x + math.cos(angle) * radius * 0.78,
                 center_y + math.sin(angle) * radius * 0.78,
                 3.0, needle[0], needle[1], needle[2], 1.0)
    r.Rdraw_circle(painter, center_x, center_y, 6, needle[0], needle[1], needle[2], 1.0)
    r.Rdraw_circle(painter, center_x, center_y, 2.5, surface[0], surface[1], surface[2], 1.0)

    draw_text(painter, r.R_INVALID_ID, 12, 14, 10, "PRESSURE  P-101", text)
    draw_text_centered(painter, r.Rfont_get_monospace(), 26, center_x, height - 40,
                       "%.1f bar" % PANEL.pressure, accent)


# ---------------------------------------------------------------------------
#  Digital meter
# ---------------------------------------------------------------------------


def paint_digital(widget, painter, width, height, user_data):
    PANEL.digital_paints += 1

    surface = role(widget, r.R_COLOR_PANEL)
    border = role(widget, r.R_COLOR_BORDER)
    text = role(widget, r.R_COLOR_TEXT)
    accent = role(widget, r.R_COLOR_ACCENT)

    r.Rdraw_rect(painter, 0, 0, width, height, 14, surface[0], surface[1], surface[2], 0.85)
    r.Rdraw_rect_outline(painter, 0.5, 0.5, width - 1, height - 1, 14, 1,
                         border[0], border[1], border[2], 0.7)

    r.Rdraw_rect(painter, 14, 34, width - 28, height - 62, 8, 0.05, 0.07, 0.09, 0.85)
    r.Rdraw_rect_outline(painter, 14.5, 34.5, width - 29, height - 63, 8, 1,
                         border[0], border[1], border[2], 0.5)

    size = height * 0.26
    draw_text_centered(painter, r.Rfont_get_monospace(), size, width * 0.5,
                       34 + (height - 62 - size) * 0.45, "%5.1f" % PANEL.flow, accent)

    draw_text(painter, r.R_INVALID_ID, 12, 14, 10, "FLOW  F-201", text)
    draw_text(painter, r.R_INVALID_ID, 12, width - 58, 10, "L/min", text)

    r.Rdraw_rect(painter, 20, height - 18, width - 40, 3, 1.5,
                 border[0], border[1], border[2], 0.35)
    r.Rdraw_rect(painter, 20, height - 18, (width - 40) * clamp01(PANEL.flow / 100.0), 3, 1.5,
                 accent[0], accent[1], accent[2], 0.9)


# ---------------------------------------------------------------------------
#  LED
# ---------------------------------------------------------------------------


def paint_led(widget, painter, width, height, user_data):
    lamp = LEDS[widget]
    PANEL.led_paints += 1

    center_x = width * 0.5
    center_y = height * 0.5
    radius = min(width, height) * 0.30
    strength = 1.0 if lamp["on"] else 0.0
    red, green, blue = lamp["color"]

    for i in range(3):
        r.Rdraw_circle(painter, center_x, center_y, radius * (1.4 + 0.35 * i),
                       red, green, blue, (0.10 - 0.03 * i) * strength)

    r.Rdraw_circle(painter, center_x, center_y, radius,
                   red * (0.35 + 0.65 * strength),
                   green * (0.35 + 0.65 * strength),
                   blue * (0.35 + 0.65 * strength), 0.95)

    r.Rdraw_circle(painter, center_x - radius * 0.25, center_y - radius * 0.30, radius * 0.28,
                   1.0, 1.0, 1.0, 0.18 + 0.22 * strength)


# ---------------------------------------------------------------------------
#  Real-time strip chart
# ---------------------------------------------------------------------------


def paint_chart(widget, painter, width, height, user_data):
    PANEL.chart_paints += 1

    surface = role(widget, r.R_COLOR_PANEL)
    border = role(widget, r.R_COLOR_BORDER)
    text = role(widget, r.R_COLOR_TEXT)
    accent = role(widget, r.R_COLOR_ACCENT)
    success = role(widget, r.R_COLOR_SUCCESS)

    r.Rdraw_rect(painter, 0, 0, width, height, 14, surface[0], surface[1], surface[2], 0.85)
    r.Rdraw_rect_outline(painter, 0.5, 0.5, width - 1, height - 1, 14, 1,
                         border[0], border[1], border[2], 0.7)

    left, right, top, bottom = 52.0, 18.0, 34.0, 22.0
    plot_width = width - left - right
    plot_height = height - top - bottom

    draw_text(painter, r.R_INVALID_ID, 12, 14, 10, "TREND  10 s", text)
    r.Rdraw_rect(painter, width - 128, 14, 9, 9, 2, accent[0], accent[1], accent[2], 0.9)
    draw_text(painter, r.R_INVALID_ID, 11, width - 114, 11, "pressure", text)
    r.Rdraw_rect(painter, width - 52, 14, 9, 9, 2, success[0], success[1], success[2], 0.9)
    draw_text(painter, r.R_INVALID_ID, 11, width - 38, 11, "flow", text)

    for i in range(5):
        y = top + plot_height * i / 4.0
        r.Rdraw_line(painter, left, y, left + plot_width, y, 1,
                     border[0], border[1], border[2], 0.6 if i == 4 else 0.22)
        draw_text(painter, r.Rfont_get_monospace(), 10, 16, y - 6, "%d" % (100 - 25 * i), text)
    for i in range(11):
        x = left + plot_width * i / 10.0
        r.Rdraw_line(painter, x, top, x, top + plot_height, 1,
                     border[0], border[1], border[2], 0.12)

    if PANEL.sample_count < 2:
        return

    step_x = plot_width / (SAMPLES - 1)
    previous_pressure = (0.0, 0.0)
    previous_flow = (0.0, 0.0)

    for i in range(PANEL.sample_count):
        index = (PANEL.sample_head - PANEL.sample_count + i + SAMPLES) % SAMPLES
        x = left + plot_width - step_x * (PANEL.sample_count - 1 - i)
        pressure_y = top + plot_height * (1.0 - clamp01(PANEL.pressure_history[index] / 100.0))
        flow_y = top + plot_height * (1.0 - clamp01(PANEL.flow_history[index] / 100.0))

        if i > 0:
            r.Rdraw_line(painter, previous_pressure[0], previous_pressure[1], x, pressure_y, 1.8,
                         accent[0], accent[1], accent[2], 0.95)
            r.Rdraw_line(painter, previous_flow[0], previous_flow[1], x, flow_y, 1.8,
                         success[0], success[1], success[2], 0.95)
        previous_pressure = (x, pressure_y)
        previous_flow = (x, flow_y)

    if PANEL.armed:
        y = top + plot_height * (1.0 - 0.75)
        r.Rdraw_line(painter, left, y, left + plot_width, y, 1.5, 0.95, 0.35, 0.30, 0.5)
        draw_text(painter, r.R_INVALID_ID, 10, left + 6, y - 14, "alarm 75", text)


# ---------------------------------------------------------------------------
#  UI
# ---------------------------------------------------------------------------


def tile():
    column = r.Rwidget_create(r.R_WIDGET_VBOX)
    r.Rpanel_set_spacing(column, 8)
    r.Rslot_set_padding(column, 12, 12, 12, 12)
    return column


def custom_panel(paint, min_width, min_height):
    widget = r.Rwidget_create(r.R_WIDGET_CUSTOM)
    r.Rcustom_set_paint_callback(widget, paint)
    r.Rwidget_set_min_size(widget, min_width, min_height)
    r.Rwidget_set_hit_testable(widget, 0)
    return widget


def sync_controls():
    r.Rbutton_set_text(PANEL.pump_button, "PUMP  ON" if PANEL.pump else "PUMP  OFF")
    r.Rbutton_set_style(PANEL.pump_button, r.R_BUTTON_PRIMARY if PANEL.pump else r.R_BUTTON_DEFAULT)
    r.Rbutton_set_text(PANEL.valve_button, "VALVE  OPEN" if PANEL.valve else "VALVE  SHUT")
    r.Rbutton_set_style(PANEL.valve_button, r.R_BUTTON_PRIMARY if PANEL.valve else r.R_BUTTON_DEFAULT)
    r.Rbutton_set_text(PANEL.arm_button, "INTERLOCKS  ARMED" if PANEL.armed else "INTERLOCKS  OFF")
    r.Rbutton_set_style(PANEL.arm_button, r.R_BUTTON_PRIMARY if PANEL.armed else r.R_BUTTON_DEFAULT)
    r.Rbutton_set_text(PANEL.pause_button, "PAUSE" if PANEL.running else "RESUME")


def on_pump(button, user_data):
    PANEL.pump = not PANEL.pump
    sync_controls()


def on_valve(button, user_data):
    PANEL.valve = not PANEL.valve
    sync_controls()


def on_arm(button, user_data):
    PANEL.armed = not PANEL.armed
    sync_controls()
    update_status()


def on_pause(button, user_data):
    PANEL.running = not PANEL.running
    sync_controls()
    update_status()


def build_panel(window):
    root = r.Rwidget_create(r.R_WIDGET_VBOX)
    r.Rpanel_set_spacing(root, 12)
    r.Rslot_set_padding(root, 16, 16, 16, 16)
    r.Rwindow_set_content(window, root)

    row = r.Rwidget_add_child(root, r.Rwidget_create(r.R_WIDGET_HBOX))
    r.Rpanel_set_spacing(row, 12)

    gauge_tile = r.Rwidget_add_child(row, tile())
    r.Rslot_set_fill(gauge_tile, 1)
    gauge = r.Rwidget_add_child(gauge_tile, custom_panel(paint_meter, 240, 240))
    r.Rslot_set_fill(gauge, 1)

    flow_tile = r.Rwidget_add_child(row, tile())
    r.Rslot_set_fill(flow_tile, 1)
    flow = r.Rwidget_add_child(flow_tile, custom_panel(paint_digital, 240, 150))
    r.Rslot_set_fill(flow, 1)

    control_tile = r.Rwidget_add_child(row, tile())
    r.Rslot_set_fill(control_tile, 1)

    leds = r.Rwidget_add_child(control_tile, r.Rwidget_create(r.R_WIDGET_HBOX))
    r.Rpanel_set_spacing(leds, 10)
    r.Rslot_set_halign(leds, r.R_HALIGN_CENTER)

    for lamp in (POWER, LINK, ALARM):
        column = r.Rwidget_add_child(leds, r.Rwidget_create(r.R_WIDGET_VBOX))
        r.Rpanel_set_spacing(column, 2)
        widget = r.Rwidget_add_child(column, custom_panel(paint_led, 46, 46))
        LEDS[widget] = lamp
        r.Rwidget_set_size(widget, 46, 46)
        label = r.Rwidget_add_child(column, r.Rlabel_create(lamp["name"]))
        r.Rlabel_set_text_size(label, 10)
        r.Rslot_set_halign(label, r.R_HALIGN_CENTER)

    PANEL.alarm_label = r.Rwidget_add_child(control_tile, r.Rlabel_create("interlocks off"))
    r.Rlabel_set_text_size(PANEL.alarm_label, 12)
    r.Rslot_set_halign(PANEL.alarm_label, r.R_HALIGN_CENTER)

    PANEL.pump_button = r.Rwidget_add_child(control_tile, r.Rbutton_create("PUMP  OFF"))
    r.Rbutton_set_clicked_callback(PANEL.pump_button, on_pump)
    PANEL.valve_button = r.Rwidget_add_child(control_tile, r.Rbutton_create("VALVE  SHUT"))
    r.Rbutton_set_clicked_callback(PANEL.valve_button, on_valve)
    PANEL.arm_button = r.Rwidget_add_child(control_tile, r.Rbutton_create("INTERLOCKS  OFF"))
    r.Rbutton_set_clicked_callback(PANEL.arm_button, on_arm)

    spacer = r.Rwidget_add_child(control_tile, r.Rwidget_create(r.R_WIDGET_SPACER))
    r.Rslot_set_fill(spacer, 1)

    chart_tile = r.Rwidget_add_child(root, tile())
    r.Rslot_set_fill(chart_tile, 1)
    chart = r.Rwidget_add_child(chart_tile, custom_panel(paint_chart, 480, 180))
    r.Rslot_set_fill(chart, 1)

    bar = r.Rwidget_add_child(root, r.Rwidget_create(r.R_WIDGET_HBOX))
    r.Rpanel_set_spacing(bar, 12)
    PANEL.status = r.Rwidget_add_child(bar, r.Rlabel_create("starting"))
    r.Rslot_set_fill(PANEL.status, 1)
    r.Rslot_set_valign(PANEL.status, r.R_VALIGN_CENTER)

    PANEL.pause_button = r.Rwidget_add_child(bar, r.Rbutton_create("PAUSE"))
    r.Rbutton_set_clicked_callback(PANEL.pause_button, on_pause)

    sync_controls()


def on_frame(delta, user_data):
    simulate(delta)
    update_status()
    r.Rwindow_request_redraw(PANEL.window)


# ---------------------------------------------------------------------------
#  Start
# ---------------------------------------------------------------------------


def init(headless):
    desc = r.RInitDesc()
    if headless:
        desc.backend = r.R_BACKEND_NULL
        desc.headless = 1
    else:
        desc.backend = r.R_BACKEND_VULKAN
    if not r.Rinit_ex(desc):
        print("cannot start Raven (is a Vulkan driver installed?)")
        return False
    return True


def create_window():
    PANEL.window = r.Rwindow_create("Instrument panel", 1180, 760, r.R_WINDOW_DEFAULT)
    build_panel(PANEL.window)
    update_status()


def save_ppm(window, path):
    pixels, width, height = r.Rwindow_capture(window)
    if not pixels:
        return False
    data = ctypes.string_at(pixels, width * height * 4)
    with open(path, "wb") as file:
        file.write(b"P6\n%d %d\n255\n" % (width, height))
        for i in range(width * height):
            file.write(data[i * 4:i * 4 + 3])
    r.Rpixels_free(pixels)
    return True


def run_tests():
    r.Rtheme_apply(r.R_THEME_DARK)
    create_window()
    r.Rdraw_frame()

    if not (PANEL.meter_paints and PANEL.digital_paints and PANEL.chart_paints and PANEL.led_paints):
        print("  FAIL: a custom widget was never painted")
        return 1

    pressure_at_rest = PANEL.pressure
    PANEL.pump = True
    for _ in range(180):
        simulate(1.0 / 60.0)
    if PANEL.pressure <= pressure_at_rest + 20.0:
        print("  FAIL: the pump did not raise the pressure (%.1f)" % PANEL.pressure)
        return 1

    PANEL.valve = True
    for _ in range(120):
        simulate(1.0 / 60.0)
    if PANEL.flow <= 20.0:
        print("  FAIL: the valve did not let the flow through (%.1f)" % PANEL.flow)
        return 1

    if PANEL.sample_count < 20:
        print("  FAIL: the chart has %d samples" % PANEL.sample_count)
        return 1

    PANEL.armed = True
    PANEL.pressure = 80.0
    simulate(1.0 / 60.0)
    if not alarm_active() or not ALARM["on"]:
        print("  FAIL: the alarm did not trip")
        return 1

    PANEL.pressure = 2.0
    r.Rdraw_frame()
    x, y, width, height = r.Rwidget_get_rect(PANEL.pump_button)
    before = PANEL.pump
    r.Rwindow_inject_mouse_move(PANEL.window, x + width / 2, y + height / 2)
    r.Rwindow_inject_mouse_button(PANEL.window, r.R_MOUSE_LEFT, 1)
    r.Rwindow_inject_mouse_button(PANEL.window, r.R_MOUSE_LEFT, 0)
    if PANEL.pump == before:
        print("  FAIL: the pump button did not toggle")
        return 1

    PANEL.running = False
    frozen = PANEL.time
    simulate(1.0)
    if PANEL.time != frozen:
        print("  FAIL: the simulation ran while paused")
        return 1

    print("instrument panel: simulated %.1f s, %d samples, %d meter paints, %d chart paints"
          % (PANEL.time, PANEL.sample_count, PANEL.meter_paints, PANEL.chart_paints))
    r.Rshutdown()
    print("PASS")
    return 0


def run_capture(path, frames):
    r.Rtheme_apply(r.R_THEME_DARK)
    create_window()
    PANEL.pump = True
    PANEL.valve = True
    PANEL.armed = True
    sync_controls()
    update_status()

    for _ in range(frames):
        simulate(1.0 / 60.0)
        update_status()
        r.Rwindow_request_redraw(PANEL.window)
        r.Rwait_frame(0.0)

    saved = save_ppm(PANEL.window, path)
    r.Rshutdown()
    if not saved:
        print("cannot write %s" % path)
        return 1
    print("saved %s" % path)
    return 0


def main():
    arguments = sys.argv[1:]
    if "--test" in arguments:
        return run_tests() if init(True) else 1

    if not init(False):
        return 1

    if "--capture" in arguments:
        index = arguments.index("--capture")
        path = arguments[index + 1] if index + 1 < len(arguments) else "panel.ppm"
        frames = 180
        if "--frames" in arguments:
            frames = int(arguments[arguments.index("--frames") + 1])
        return run_capture(path, frames)

    r.Rtheme_apply(r.R_THEME_DARK)
    create_window()
    r.Rset_frame_callback(on_frame)
    while r.Rwait_frame(-1.0):
        pass
    r.Rshutdown()
    return 0


if __name__ == "__main__":
    sys.exit(main())
