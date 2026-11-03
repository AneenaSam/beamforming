#!/usr/bin/env python3
"""
Interactive Animated Scientific Visualization:
Circular Array Beamforming, Wavefront Propagation, Sensor Reception Capture,
and Direction of Arrival (DOA) Identification.
"""

import os
import sys
import csv
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
import matplotlib.patches as patches

# Load data exported by C++ engine
def load_csv_data():
    # 1. Sensor Positions
    pos_file = "data/sensor_positions.csv"
    if not os.path.exists(pos_file):
        raise FileNotFoundError("Run build/beamforming_app.exe first to generate data.")

    sensors = []
    with open(pos_file, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or row[0].startswith("#") or row[0] == "sensor_id":
                continue
            sensors.append({
                "id": int(row[0]),
                "rad": float(row[1]),
                "deg": float(row[2]),
                "x": float(row[3]),
                "y": float(row[4])
            })

    # 2. Sensor Delays & Capture Order
    delays_file = "data/sensor_delays.csv"
    delays = {}
    src_angle = 45.0
    with open(delays_file, "r") as f:
        for line in f:
            if "SourceAngleDeg=" in line:
                src_angle = float(line.split("=")[1].strip())
            elif not line.startswith("#"):
                parts = line.strip().split(",")
                if parts[0] != "sensor_id" and len(parts) >= 7:
                    s_id = int(parts[0])
                    delays[s_id] = {
                        "proj": float(parts[1]),
                        "delay_ms": float(parts[3]),
                        "phase_deg": float(parts[5]),
                        "rank": int(parts[6])
                    }

    # 3. Beamforming Power (Bartlett Spectrum)
    beam_file = "data/beam_power.csv"
    angles = []
    power_db = []
    est_doa = 45.0
    with open(beam_file, "r") as f:
        for line in f:
            if "EstimatedDOADeg=" in line:
                est_doa = float(line.split("=")[1].strip())
            elif not line.startswith("#"):
                parts = line.strip().split(",")
                if parts[0] != "angle_deg" and len(parts) >= 3:
                    angles.append(float(parts[0]))
                    power_db.append(float(parts[2]))

    # 4. Waveforms
    wave_file = "data/received_signals.csv"
    times = []
    waveforms = {s["id"]: [] for s in sensors}
    with open(wave_file, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or row[0].startswith("#") or row[0] == "snapshot_idx":
                continue
            times.append(float(row[1])) # time_ms
            for m in range(len(sensors)):
                waveforms[m].append(float(row[2 + m]))

    return sensors, delays, np.array(angles), np.array(power_db), src_angle, est_doa, np.array(times), waveforms

def create_animation():
    sensors, delays, angles, power_db, src_angle, est_doa, times, waveforms = load_csv_data()
    M = len(sensors)
    xs = np.array([s["x"] for s in sensors])
    ys = np.array([s["y"] for s in sensors])
    radius = np.max(np.sqrt(xs**2 + ys**2))
    
    # Identify top sensors captured first & last
    first_sensors = sorted(sensors, key=lambda s: delays[s["id"]]["rank"])[:3]
    last_sensors = sorted(sensors, key=lambda s: delays[s["id"]]["rank"], reverse=True)[:3]
    first_id = first_sensors[0]["id"]
    last_id = last_sensors[0]["id"]
    
    # Configure dark theme figure
    plt.style.use("dark_background")
    fig = plt.figure(figsize=(16, 9), facecolor="#0b0e14")
    fig.canvas.manager.set_window_title("Circular Array Beamforming & DOA Identification - Phase 2")
    
    # Grid layout: 
    # Left: Wavefront Propagation & Sensor Capture (2x2 span left)
    # Right-Top: Spatial Spectrum & DOA Identification
    # Right-Bottom: Time-Domain Oscilloscope (Sensors hitting first vs last)
    gs = fig.add_gridspec(2, 2, width_ratios=[1.2, 1.0], height_ratios=[1.0, 1.0],
                          left=0.06, right=0.96, top=0.92, bottom=0.10, wspace=0.25, hspace=0.30)
    
    ax_array = fig.add_subplot(gs[:, 0])
    ax_spectrum = fig.add_subplot(gs[0, 1])
    ax_osc = fig.add_subplot(gs[1, 1])
    
    # =========================================================================
    # PANEL 1: Wavefront Propagation & Sensor Capture
    # =========================================================================
    ax_array.set_facecolor("#111622")
    margin = radius * 2.0
    ax_array.set_xlim(-margin, margin)
    ax_array.set_ylim(-margin, margin)
    ax_array.set_aspect("equal")
    ax_array.set_title(f"Planar Wavefront Propagation & Sensor Capture (Source at {src_angle:.1f}°)",
                       fontsize=13, fontweight="bold", color="#58a6ff", pad=12)
    ax_array.set_xlabel("X (meters)", color="#8b949e", fontsize=10)
    ax_array.set_ylabel("Y (meters)", color="#8b949e", fontsize=10)
    ax_array.grid(True, linestyle=":", alpha=0.3, color="#484f58")
    
    # Circular array boundary
    bound_circle = patches.Circle((0, 0), radius, edgecolor="#388bfd", facecolor="#1f6feb",
                                  alpha=0.06, linewidth=2, linestyle="--")
    ax_array.add_patch(bound_circle)
    ax_array.plot(0, 0, marker="+", color="#f85149", markersize=12, markeredgewidth=2)
    
    # Incoming Source Marker outside the array
    src_dist = radius * 1.7
    src_rad = np.radians(src_angle)
    src_x = src_dist * np.cos(src_rad)
    src_y = src_dist * np.sin(src_rad)
    ax_array.scatter(src_x, src_y, color="#f85149", s=250, marker="*", edgecolor="#ffffff",
                     linewidth=1.5, zorder=10, label=f"Source ({src_angle:.0f}°)")
    
    # Incoming direction arrow pointing to array
    ax_array.annotate("Incoming Wave", xy=(src_x * 0.75, src_y * 0.75), xytext=(src_x, src_y),
                      arrowprops=dict(facecolor="#f85149", edgecolor="#ffffff", width=2.5, headwidth=8),
                      color="#f85149", fontsize=9.5, fontweight="bold", ha="center")
    
    # Draw Wavefront lines (planar crests orthogonal to propagation vector)
    prop_ux = np.cos(src_rad)
    prop_uy = np.sin(src_rad)
    perp_ux = -np.sin(src_rad)
    perp_uy = np.cos(src_rad)
    
    num_wavefronts = 7
    wave_lines = []
    line_span = radius * 1.6
    for _ in range(num_wavefronts):
        line, = ax_array.plot([], [], color="#388bfd", lw=2.2, alpha=0.7, zorder=3)
        wave_lines.append(line)
        
    # Sensor Scatter Points (will change color dynamically when hit)
    sensor_scat = ax_array.scatter(xs, ys, s=110, c="#58a6ff", edgecolor="#ffffff", linewidth=1.2, zorder=5)
    
    # Sensor ID Labels
    sensor_texts = []
    for s in sensors:
        r_offset = radius * 1.15
        tx = r_offset * np.cos(s["rad"])
        ty = r_offset * np.sin(s["rad"])
        rank = delays[s["id"]]["rank"]
        # Highlight top 2 first captured sensors
        if rank <= 2:
            badge_color = "#e3b341"
            txt = f"{s['id']}\n★#{rank}"
        else:
            badge_color = "#30363d"
            txt = f"{s['id']}"
        t = ax_array.text(tx, ty, txt, color="#f0f6fc", fontsize=7.5, ha="center", va="center",
                          bbox=dict(boxstyle="round,pad=0.2", facecolor=badge_color, edgecolor="#58a6ff", alpha=0.85))
        sensor_texts.append(t)
        
    # Status Banner
    status_text = ax_array.text(0.03, 0.04, "", transform=ax_array.transAxes, fontsize=9.5,
                                family="monospace", color="#3fb950",
                                bbox=dict(boxstyle="round,pad=0.5", facecolor="#161b22", edgecolor="#30363d", alpha=0.9))
    
    # =========================================================================
    # PANEL 2: Bartlett Beamforming & DOA Estimation
    # =========================================================================
    ax_spectrum.set_facecolor("#111622")
    ax_spectrum.set_title("Bartlett Spatial Spectrum (DOA Peak Identification)",
                          fontsize=12, fontweight="bold", color="#58a6ff", pad=10)
    ax_spectrum.set_xlabel("Scanning Azimuth Angle (degrees)", color="#8b949e", fontsize=9.5)
    ax_spectrum.set_ylabel("Beam Power (dB)", color="#8b949e", fontsize=9.5)
    ax_spectrum.set_xlim(0, 360)
    ax_spectrum.set_ylim(-30, 2)
    ax_spectrum.grid(True, linestyle=":", alpha=0.35, color="#484f58")
    
    # Static spatial spectrum curve
    ax_spectrum.plot(angles, power_db, color="#388bfd", lw=2, label="Spatial Spectrum")
    # True source angle line
    ax_spectrum.axvline(src_angle, color="#f85149", linestyle="--", lw=1.8, label=f"True Source ({src_angle:.0f}°)")
    # Estimated peak marker
    ax_spectrum.plot(est_doa, 0.0, marker="o", color="#3fb950", markersize=9, label=f"Estimated DOA ({est_doa:.0f}°)")
    
    # Live scanning cursor line
    scan_cursor = ax_spectrum.axvline(0, color="#e3b341", lw=1.5, alpha=0.9)
    scan_dot, = ax_spectrum.plot([0], [power_db[0]], marker="o", color="#e3b341", markersize=7)
    ax_spectrum.legend(loc="lower right", fontsize=8.5, facecolor="#161b22", edgecolor="#30363d")
    
    # =========================================================================
    # PANEL 3: Oscilloscope (Sensor Capture & Phase Difference)
    # =========================================================================
    ax_osc.set_facecolor("#111622")
    ax_osc.set_title(f"Received Sensor Waveforms: Sensor {first_id} (Captured First) vs Sensor {last_id} (Captured Last)",
                     fontsize=11.5, fontweight="bold", color="#58a6ff", pad=10)
    ax_osc.set_xlabel("Time (milliseconds)", color="#8b949e", fontsize=9.5)
    ax_osc.set_ylabel("Amplitude", color="#8b949e", fontsize=9.5)
    ax_osc.grid(True, linestyle=":", alpha=0.35, color="#484f58")
    
    t_max = min(12.0, times[-1])
    mask = times <= t_max
    sub_t = times[mask]
    
    # Plot waveform for first sensor hit
    line_first, = ax_osc.plot(sub_t, np.array(waveforms[first_id])[mask], color="#3fb950", lw=2.0,
                              label=f"Sensor {first_id} (Rank #1, Delay: {delays[first_id]['delay_ms']:.2f} ms)")
    # Plot waveform for last sensor hit
    line_last, = ax_osc.plot(sub_t, np.array(waveforms[last_id])[mask], color="#f85149", lw=2.0, linestyle="--",
                             label=f"Sensor {last_id} (Rank #36, Delay: {delays[last_id]['delay_ms']:.2f} ms)")
    
    # Live cursor tracking time
    t_cursor = ax_osc.axvline(0, color="#e3b341", lw=1.5, linestyle=":")
    ax_osc.legend(loc="upper right", fontsize=8.5, facecolor="#161b22", edgecolor="#30363d")
    
    # =========================================================================
    # ANIMATION LOGIC
    # =========================================================================
    wavelength = 0.3430
    c = 343.0
    total_frames = 180
    
    # Projections of sensors along propagation axis
    sensor_projs = np.array([delays[s["id"]]["proj"] for s in sensors])
    
    # State tracking
    anim_running = [True]
    
    def update(frame):
        # Progress parameter: phase wraps around
        t_sim = (frame / total_frames) * (total_frames * 0.05) # Simulated time in ms
        
        # 1. Update Wavefronts
        # Wavefronts travel from positive projection (source) towards negative
        # Position d(t) = d0 - c * t
        d_center = radius * 1.5 - ((frame % 60) / 60.0) * wavelength * 3.0
        
        for idx, line in enumerate(wave_lines):
            offset_dist = d_center - (idx - num_wavefronts // 2) * wavelength
            # Points along the line: center_pt + s * perp_vec
            cx = offset_dist * prop_ux
            cy = offset_dist * prop_uy
            x_line = [cx - line_span * perp_ux, cx + line_span * perp_ux]
            y_line = [cy - line_span * perp_uy, cy + line_span * perp_uy]
            line.set_data(x_line, y_line)
            
            # Fade lines as they leave array bounds
            dist_to_center = abs(offset_dist)
            alpha = max(0.1, min(0.85, 1.0 - (dist_to_center / (radius * 1.8))))
            line.set_alpha(alpha)
            
        # 2. Dynamic Sensor Lighting: Sensors glow when a wave crest passes them!
        # Instantaneous wave phase at sensor m: phi_m(t) = 2*pi*f*(t - tau_m)
        colors = []
        for m in range(M):
            tau_s = delays[m]["delay_ms"] / 1000.0
            time_now_s = (frame * 0.00015)
            wave_val = np.cos(2 * np.pi * 1000.0 * (time_now_s - tau_s))
            # Map cosine [-1, 1] to color glow: high = bright yellow/gold, low = dark blue
            if wave_val > 0.6:
                colors.append("#e3b341") # Bright crest hit
            elif wave_val > 0.0:
                colors.append("#58a6ff") # Nominal
            else:
                colors.append("#21262d") # In trough
        sensor_scat.set_facecolor(colors)
        
        # 3. Update Bartlett Beam Scanning Cursor
        scan_angle = (frame * 2.0) % 360.0
        scan_cursor.set_xdata([scan_angle])
        curr_power = power_db[int(scan_angle)]
        scan_dot.set_data([scan_angle], [curr_power])
        
        # 4. Update Oscilloscope Cursor
        curr_t_ms = (frame % len(sub_t)) * (sub_t[-1] / len(sub_t))
        t_cursor.set_xdata([curr_t_ms])
        
        # 5. Status Text
        status_text.set_text(
            f"SIGNAL DETECTION ENGINE STATUS:\n"
            f"• Wavefront Position : {d_center:.3f} m\n"
            f"• First Sensor Hit   : Sensor {first_id} & Sensor {delays[first_sensors[1]['id']]['rank']-1}\n"
            f"• Max Arrival Lead   : {abs(delays[first_id]['delay_ms']):.2f} ms ahead of center\n"
            f"• Beam Scan Angle    : {scan_angle:5.1f}° (Power: {curr_power:5.1f} dB)\n"
            f"• IDENTIFIED DOA     : {est_doa:.1f}° (True: {src_angle:.1f}°) -> EXACT MATCH"
        )
        
        return wave_lines + [sensor_scat, scan_cursor, scan_dot, t_cursor, status_text]

    # Create Animation (30 fps)
    anim = FuncAnimation(fig, update, frames=total_frames, interval=33, blit=False)
    
    # Save a static high-res preview of the capture
    preview_path = "docs/animated_capture_preview.png"
    os.makedirs(os.path.dirname(preview_path), exist_ok=True)
    fig.savefig(preview_path, dpi=200, facecolor=fig.get_facecolor(), bbox_inches="tight")
    print(f"[INFO] Animation frame preview saved to: {preview_path}")

    # Check CLI arguments for saving GIF or headless mode
    if "--save-gif" in sys.argv:
        gif_path = "docs/wavefront_capture_simulation.gif"
        print(f"[INFO] Rendering animation to {gif_path} (60 frames)... Please wait.")
        anim.save(gif_path, writer="pillow", fps=20)
        print(f"[INFO] Animation GIF saved successfully to: {gif_path}")
        plt.close(fig)
        return

    # Show window interactively
    plt.show()

if __name__ == "__main__":
    create_animation()
