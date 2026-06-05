"""
Haptic Paddle Simulator - Ground Dip Force Field
=================================================
The haptic force displayed is the x-component of gravity along the slope:
    Fx = Fg * sin(theta)
where theta is the ramp angle at the paddle's current position, and Fg is
the weight of the paddle (PADDLE_MASS * g).  On flat ground and the flat
dip floor theta = 0, so Fx = 0.  The arrow direction flips with the slope
sign (pointing left when climbing out of the dip on the right, etc.).

Profile:
  flat ground  →  cosine ramp down  →  flat dip floor  →  cosine ramp up  →  flat ground

Controls:
  LEFT / RIGHT arrow keys — move the paddle (placeholder for encoder input)

Replace get_position_from_input() with your encoder read from the Pico.

Run with:  pgzrun haptic_paddle.py
"""

import math
import pygame
import pgzrun  # noqa: F401
import serial

ser = serial.Serial('COM6')

# ---------------------------------------------------------------------------
# Window
# ---------------------------------------------------------------------------
WIDTH  = 900
HEIGHT = 550
TITLE  = "Haptic Paddle – Ground Dip (Fx = Fg·sin θ)"

# ---------------------------------------------------------------------------
# Dip geometry
# ---------------------------------------------------------------------------
DIP_DEPTH      = 80    # px: depth of dip below baseline
DIP_FLAT_W     = 80    # px: half-width of flat dip floor from centre
TRANSITION_W   = 120   # px: width of each cosine ramp
BASELINE_Y     = 320   # px from top: y of flat ground

# ---------------------------------------------------------------------------
# Physics
# ---------------------------------------------------------------------------
PADDLE_MASS    = 1.0   # kg  (arbitrary; scales the force display)
G              = 9.81  # m/s²

PADDLE_W       = 28    # wheel diameter (px)
KEY_SPEED      = 5     # px per frame (keyboard mode)

# ---------------------------------------------------------------------------
# State
# ---------------------------------------------------------------------------
paddle_x = float(WIDTH // 2)


# ---------------------------------------------------------------------------
# Surface profile & physics
# ---------------------------------------------------------------------------
def dip_y(x: float) -> float:
    """Y-coordinate of the ground surface at x (pixels, y increases downward)."""
    dist = abs(x - WIDTH / 2)
    if dist <= DIP_FLAT_W:
        return BASELINE_Y + DIP_DEPTH
    elif dist <= DIP_FLAT_W + TRANSITION_W:
        t     = (dist - DIP_FLAT_W) / TRANSITION_W
        blend = (1 - math.cos(t * math.pi)) / 2
        return BASELINE_Y + DIP_DEPTH * (1.0 - blend)
    else:
        return BASELINE_Y


def slope_angle(x: float) -> float:
    """
    Ramp angle theta at position x, in radians.
    Computed from the numerical slope dy/dx (px/px — dimensionless ratio).
    theta > 0 means the surface slopes downward to the right.
    theta < 0 means the surface slopes downward to the left.
    """
    h = 0.5
    dy_dx = (dip_y(x + h) - dip_y(x - h)) / (2 * h)
    return math.atan(dy_dx)


def fx_haptic(x: float) -> float:
    """
    X-component of gravitational force the paddle exerts on the ramp surface
    (i.e. the force the user must overcome to move horizontally):
        Fx = Fg * sin(theta)
    Positive = force pushes paddle to the right (ramp slopes down-right).
    Negative = force pushes paddle to the left  (ramp slopes down-left).
    Zero on flat sections.
    """
    theta = slope_angle(x)
    fg    = PADDLE_MASS * G
    return fg * math.sin(theta)


# ---------------------------------------------------------------------------
# Input  —  replace body with encoder read from Pico
# ---------------------------------------------------------------------------
def get_position_from_input() -> float:
    global paddle_x

    n_bytes = ser.readline()
    result = n_bytes.decode('utf-8').strip()
    value = float(result)

    # Scale voltage to pixel speed the same way keyboard added ±KEY_SPEED
    paddle_x += value * KEY_SPEED / 1.65

    paddle_x = max(PADDLE_W, min(WIDTH - PADDLE_W, paddle_x))
    return paddle_x

# ---------------------------------------------------------------------------
# Colours
# ---------------------------------------------------------------------------
SKY_TOP      = ( 28,  42,  72)
SKY_BOT      = ( 55,  85, 135)
GROUND_FILL  = (100,  80,  50)
SURF_LINE    = (185, 162, 118)
DIP_LINE     = (135, 112,  78)
PADDLE_COL   = (220,  55,  55)
FORCE_POS    = (  0, 220, 160)   # arrow when Fx pushes right
FORCE_NEG    = (255, 160,  30)   # arrow when Fx pushes left
PANEL_BG     = ( 15,  25,  45)
PANEL_BORDER = ( 60,  90, 130)
DASH_COL     = (220, 200, 140)   # dashed boundary lines


def lerp_color(c1, c2, t):
    return tuple(int(a + (b - a) * t) for a, b in zip(c1, c2))


# ---------------------------------------------------------------------------
# Draw helpers
# ---------------------------------------------------------------------------
def draw_background(screen):
    limit = BASELINE_Y + DIP_DEPTH + 30
    for row in range(limit):
        t = row / limit
        pygame.draw.line(screen.surface, lerp_color(SKY_TOP, SKY_BOT, t),
                         (0, row), (WIDTH, row))


def draw_surface(screen):
    step = 3
    pts  = [(x, dip_y(x)) for x in range(0, WIDTH + step, step)]

    # Filled ground body
    poly = [(int(x), int(y)) for x, y in pts] + [(WIDTH, HEIGHT), (0, HEIGHT)]
    pygame.draw.polygon(screen.surface, GROUND_FILL, poly)

    # Surface highlight line — colour shifts by zone
    for i in range(len(pts) - 1):
        dist   = abs(pts[i][0] - WIDTH / 2)
        in_dip = dist <= DIP_FLAT_W
        in_ramp = DIP_FLAT_W < dist < DIP_FLAT_W + TRANSITION_W
        t   = 1.0 if in_dip else (0.5 if in_ramp else 0.0)
        col = lerp_color(SURF_LINE, DIP_LINE, t)
        screen.draw.line(
            (int(pts[i][0]),   int(pts[i][1])),
            (int(pts[i+1][0]), int(pts[i+1][1])),
            col
        )

    # --- Dashed vertical lines at the FLAT DIP FLOOR boundaries ---
    dash_on, dash_off = 8, 5
    for sign in (-1, 1):
        bx     = int(WIDTH / 2 + sign * DIP_FLAT_W)
        surf_y = int(dip_y(bx))
        y      = surf_y - 160          # start well above the surface
        while y < surf_y:
            y_end = min(y + dash_on, surf_y)
            screen.draw.line((bx, y), (bx, y_end), DASH_COL)
            y += dash_on + dash_off

    # Dashed lines at the outer ramp boundaries (subtler colour)
    for sign in (-1, 1):
        bx     = int(WIDTH / 2 + sign * (DIP_FLAT_W + TRANSITION_W))
        surf_y = int(BASELINE_Y)
        y      = surf_y - 60
        while y < surf_y:
            y_end = min(y + dash_on, surf_y)
            screen.draw.line((bx, y), (bx, y_end), (150, 135, 100))
            y += dash_on + dash_off


def draw_paddle(screen, px):
    py = int(dip_y(px)) - PADDLE_W // 2
    screen.draw.filled_circle((int(px) + 3, py + 4), PADDLE_W // 2, (20, 15, 10))
    screen.draw.filled_circle((int(px), py), PADDLE_W // 2, PADDLE_COL)
    screen.draw.circle((int(px), py), PADDLE_W // 2, (255, 115, 95))
    screen.draw.filled_circle((int(px), py), 4, (245, 225, 205))

    # Draw a small theta arc on the surface to show the ramp angle
    theta = slope_angle(px)
    if abs(theta) > 0.01:
        surf_y = int(dip_y(px))
        arc_r  = 22
        # Draw a short tangent line to represent the slope
        dx = int(arc_r * math.cos(theta))
        dy = int(arc_r * math.sin(theta))
        screen.draw.line((int(px) - dx, surf_y - dy),
                         (int(px) + dx, surf_y + dy),
                         (220, 210, 160))


def draw_force_arrow(screen, px):
    """
    Horizontal arrow showing Fx = Fg·sin(theta).
    Direction: points in the direction gravity pulls the paddle along the slope.
    """
    fx      = fx_haptic(px)
    py_surf = int(dip_y(px))
    py_ctr  = py_surf - PADDLE_W // 2   # paddle centre y

    max_fx   = PADDLE_MASS * G          # max possible |Fx| = Fg (theta=90°, never reached)
    max_px   = 90                        # max arrow length in pixels
    arrow_px = int(abs(fx) / max_fx * max_px)

    col   = FORCE_POS if fx >= 0 else FORCE_NEG
    dirn  = 1 if fx >= 0 else -1

    bx   = int(px)
    tip_x = bx + dirn * arrow_px
    head  = 7

    if arrow_px > 2:
        screen.draw.line((bx, py_ctr), (tip_x, py_ctr), col)
        screen.draw.line((tip_x, py_ctr), (tip_x - dirn * head, py_ctr - head), col)
        screen.draw.line((tip_x, py_ctr), (tip_x - dirn * head, py_ctr + head), col)

    # Numeric label above paddle
    theta_deg = math.degrees(slope_angle(px))
    label = f"Fx = {fx:+.2f} N  (θ={theta_deg:.1f}°)"
    lx = min(max(bx - 70, 5), WIDTH - 200)
    screen.draw.text(label, (lx, py_surf - PADDLE_W - 24), color=col, fontsize=17)


def draw_force_profile(screen, px):
    """Mini graph of Fx vs x across the full screen width."""
    gx, gy = 20, 20
    gw, gh = 220, 80

    screen.draw.filled_rect(Rect(gx - 5, gy - 5, gw + 10, gh + 28), PANEL_BG)
    screen.draw.rect(Rect(gx - 5, gy - 5, gw + 10, gh + 28), PANEL_BORDER)

    # Zero line
    zero_y = gy + gh // 2
    screen.draw.line((gx, zero_y), (gx + gw, zero_y), (60, 80, 110))

    # Compute Fx across full width, find max for normalisation
    fx_vals = [fx_haptic(i / gw * WIDTH) for i in range(gw + 1)]
    max_val = max(abs(v) for v in fx_vals) or 1.0

    pts = []
    for i, fv in enumerate(fx_vals):
        # positive Fx → above zero line, negative → below
        y = zero_y - int((fv / max_val) * (gh // 2 - 2))
        pts.append((gx + i, y))

    for i in range(len(pts) - 1):
        col = FORCE_POS if fx_vals[i] >= 0 else FORCE_NEG
        screen.draw.line(pts[i], pts[i + 1], col)

    # Current position dot
    mi = int((px / WIDTH) * gw)
    screen.draw.filled_circle((gx + mi, pts[mi][1]), 4, (255, 220, 80))

    screen.draw.text("Fx = Fg·sin θ", (gx, gy + gh + 6),
                     color=(155, 178, 218), fontsize=14)


def draw_zone_labels(screen):
    cx = WIDTH // 2
    # Dip floor
    screen.draw.text("dip floor", (cx - 28, int(dip_y(cx)) - 18),
                     color=(200, 185, 145), fontsize=14)
    # Ramp zones
    for sign, label in ((-1, "ramp"), (1, "ramp")):
        lx = int(cx + sign * (DIP_FLAT_W + TRANSITION_W / 2))
        screen.draw.text(label, (lx - 14, BASELINE_Y - 16),
                         color=(170, 155, 120), fontsize=13)
    # Flat zones
    for lx in (80, WIDTH - 110):
        screen.draw.text("flat", (lx, BASELINE_Y - 18),
                         color=(155, 140, 110), fontsize=13)


def draw_hud(screen, px):
    dist = abs(px - WIDTH / 2)
    if dist <= DIP_FLAT_W:
        zone = "dip floor"
    elif dist <= DIP_FLAT_W + TRANSITION_W:
        zone = "ramp"
    else:
        zone = "flat ground"

    screen.draw.text(f"x = {int(px)}  [{zone}]",
                     (WIDTH - 240, HEIGHT - 30),
                     color=(200, 210, 230), fontsize=18)
    screen.draw.text("◀ ▶  arrow keys  |  swap get_position_from_input() for encoder",
                     (10, HEIGHT - 24), color=(100, 120, 160), fontsize=14)


# ---------------------------------------------------------------------------
# Pgzrun hooks
# ---------------------------------------------------------------------------
def update():
    get_position_from_input()


def draw():
    screen.clear()
    draw_background(screen)
    draw_surface(screen)
    draw_zone_labels(screen)
    draw_force_arrow(screen, paddle_x)
    draw_paddle(screen, paddle_x)
    draw_force_profile(screen, paddle_x)
    draw_hud(screen, paddle_x)


pgzrun.go()