#!/usr/bin/env python3
"""
Interactive Animated Scientific Visualization:
Circular Array Beamforming with Dynamic Random Signal Direction, Wavefront Propagation,
Sensor Capture Tracking, and Live Bartlett DOA Peak Finding.
"""

import os
import sys
import csv
import subprocess
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
import matplotlib.patches as patches

# Helper to load data exported by C++ engine
def load_csv_data():
    pos_file = "data/sensor_positions.csv"
    delays_file = "data/sensor_delays.csv"
    beam_file = "data/beam_power.csv"
    wave_file = "data/received_signals.csv"

    if not all(os.path.exists(p) for p in [pos_file, delays_file, beam_file, wave_file]):
        print("[INFO] Data files missing. Running C++ engine to generate random signal data...")
        subprocess.run(["build/beamforming_app.exe"], check=True)

    # 1. Sensor Positions
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

    # 2. Delays & Arrival Ranking
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
    angles = []
    power_db = []
    est_doa = src_angle
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
    times = []
    waveforms = {s["id"]: [] for s in sensors}
    with open(wave_file, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or row[0].startswith("#") or row[0] == "snapshot_idx":
                continue
            times.append(float(row[1]))
            for m in range(len(sensors)):
                waveforms[m].append(float(row[2 + m]))

    return sensors, delays, np.array(angles), np.array(power_db), src_angle, est_doa, np.array(times), waveforms

class InteractiveBeamformingDashboard:
    def __init__(self):
        plt.style.use("dark_background")
        self.fig = plt.figure(figsize=(16, 9), facecolor="#0b0e14")
        self.fig.canvas.manager.set_window_title("Circular Array Beamforming - Random Signal & DOA Identification")

        # Grid Layout
        gs = self.fig.add_gridspec(2, 2, width_ratios=[1.2, 1.0], height_ratios=[1.0, 1.0],
                                   left=0.06, right=0.96, top=0.92, bottom=0.12, wspace=0.25, hspace=0.32)

        self.ax_array = self.fig.add_subplot(gs[:, 0])
        self.ax_spectrum = self.fig.add_subplot(gs[0, 1])
        self.ax_osc = self.fig.add_subplot(gs[1, 1])

        # Animation state
        self.anim_running = True
        self.speed = 1.0
        self.wavelength = 0.3430
        self.total_frames = 180

        # Load initial data
        self.load_data()
        self.setup_ui()
        self.setup_plots()

        # Connect keyboard shortcuts
        self.fig.canvas.mpl_connect("key_press_event", self.on_key_press)

    def load_data(self):
        self.sensors, self.delays, self.angles, self.power_db, self.src_angle, self.est_doa, self.times, self.waveforms = load_csv_data()
        self.M = len(self.sensors)
        self.xs = np.array([s["x"] for s in self.sensors])
        self.ys = np.array([s["y"] for s in self.sensors])
        self.radius = np.max(np.sqrt(self.xs**2 + self.ys**2))

        # Sensor capture order
        self.sorted_sensors = sorted(self.sensors, key=lambda s: self.delays[s["id"]]["rank"])
        self.first_id = self.sorted_sensors[0]["id"]
        self.second_id = self.sorted_sensors[1]["id"]
        self.last_id = self.sorted_sensors[-1]["id"]

    def setup_ui(self):
        # Interactive Control Buttons at Bottom
        ax_btn_rand = self.fig.add_axes([0.06, 0.03, 0.18, 0.045])
        self.btn_rand = Button(ax_btn_rand, "New Random Signal (R)", color="#238636", hovercolor="#2ea043")
        self.btn_rand.label.set_color("#ffffff")
        self.btn_rand.label.set_fontsize(10)
        self.btn_rand.label.set_fontweight("bold")
        self.btn_rand.on_clicked(self.trigger_new_random_signal)

        ax_btn_pause = self.fig.add_axes([0.26, 0.03, 0.12, 0.045])
        self.btn_pause = Button(ax_btn_pause, "Pause / Play (P)", color="#1f6feb", hovercolor="#388bfd")
        self.btn_pause.label.set_color("#ffffff")
        self.btn_pause.label.set_fontsize(9.5)
        self.btn_pause.label.set_fontweight("bold")
        self.btn_pause.on_clicked(self.toggle_play_pause)

    def trigger_new_random_signal(self, event=None):
        print("\n[ACTION] Triggering C++ simulation with new random signal direction...")
        # Run C++ executable to generate a new random incoming angle & beamforming data
        subprocess.run(["build/beamforming_app.exe"], check=True)
        self.load_data()
        self.update_plots_for_new_source()
        self.fig.canvas.draw_idle()

    def toggle_play_pause(self, event=None):
        self.anim_running = not self.anim_running
        status = "Resumed" if self.anim_running else "Paused"
        print(f"[ACTION] Animation {status}")

    def on_key_press(self, event):
        if event.key in ['r', 'R']:
            self.trigger_new_random_signal()
        elif event.key in ['p', 'P', ' ']:
            self.toggle_play_pause()

    def setup_plots(self):
        # =========================================================================
        # PANEL 1: Wavefront Propagation & Sensor Capture
        # =========================================================================
        self.ax_array.set_facecolor("#111622")
        margin = self.radius * 2.0
        self.ax_array.set_xlim(-margin, margin)
        self.ax_array.set_ylim(-margin, margin)
        self.ax_array.set_aspect("equal")
        self.ax_array.set_xlabel("X (meters)", color="#8b949e", fontsize=10)
        self.ax_array.set_ylabel("Y (meters)", color="#8b949e", fontsize=10)
        self.ax_array.grid(True, linestyle=":", alpha=0.3, color="#484f58")

        # Boundary circle & center
        bound_circle = patches.Circle((0, 0), self.radius, edgecolor="#388bfd", facecolor="#1f6feb",
                                      alpha=0.06, linewidth=2, linestyle="--")
        self.ax_array.add_patch(bound_circle)
        self.ax_array.plot(0, 0, marker="+", color="#f85149", markersize=12, markeredgewidth=2)

        # Incoming Source Marker
        src_dist = self.radius * 1.7
        src_rad = np.radians(self.src_angle)
        self.src_x = src_dist * np.cos(src_rad)
        self.src_y = src_dist * np.sin(src_rad)
        self.source_scat = self.ax_array.scatter([self.src_x], [self.src_y], color="#f85149", s=260,
                                                 marker="*", edgecolor="#ffffff", linewidth=1.5, zorder=10)

        # Incoming direction annotation
        self.src_annot = self.ax_array.annotate(
            f"Incoming Wave\n({self.src_angle:.1f}°)",
            xy=(self.src_x * 0.75, self.src_y * 0.75), xytext=(self.src_x, self.src_y),
            arrowprops=dict(facecolor="#f85149", edgecolor="#ffffff", width=2.5, headwidth=8),
            color="#f85149", fontsize=9.5, fontweight="bold", ha="center")

        # Wavefront Lines
        self.num_wavefronts = 7
        self.wave_lines = []
        self.line_span = self.radius * 1.6
        for _ in range(self.num_wavefronts):
            line, = self.ax_array.plot([], [], color="#388bfd", lw=2.2, alpha=0.7, zorder=3)
            self.wave_lines.append(line)

        # Sensors
        self.sensor_scat = self.ax_array.scatter(self.xs, self.ys, s=110, c="#58a6ff",
                                                 edgecolor="#ffffff", linewidth=1.2, zorder=5)

        # Sensor text labels with rank badges
        self.sensor_texts = []
        for s in self.sensors:
            r_offset = self.radius * 1.15
            tx = r_offset * np.cos(s["rad"])
            ty = r_offset * np.sin(s["rad"])
            t = self.ax_array.text(tx, ty, f"{s['id']}", color="#f0f6fc", fontsize=7.5, ha="center", va="center",
                                   bbox=dict(boxstyle="round,pad=0.2", facecolor="#30363d", edgecolor="#58a6ff", alpha=0.85))
            self.sensor_texts.append(t)

        self.status_text = self.ax_array.text(0.03, 0.04, "", transform=self.ax_array.transAxes, fontsize=9.5,
                                              family="monospace", color="#3fb950",
                                              bbox=dict(boxstyle="round,pad=0.5", facecolor="#161b22", edgecolor="#30363d", alpha=0.9))

        # =========================================================================
        # PANEL 2: Bartlett Spectrum & DOA Peak
        # =========================================================================
        self.ax_spectrum.set_facecolor("#111622")
        self.ax_spectrum.set_xlabel("Scanning Azimuth Angle (degrees)", color="#8b949e", fontsize=9.5)
        self.ax_spectrum.set_ylabel("Beam Power (dB)", color="#8b949e", fontsize=9.5)
        self.ax_spectrum.set_xlim(0, 360)
        self.ax_spectrum.set_ylim(-30, 2)
        self.ax_spectrum.grid(True, linestyle=":", alpha=0.35, color="#484f58")

        self.spectrum_line, = self.ax_spectrum.plot(self.angles, self.power_db, color="#388bfd", lw=2, label="Spatial Spectrum")
        self.true_line = self.ax_spectrum.axvline(self.src_angle, color="#f85149", linestyle="--", lw=1.8, label="True Source")
        self.peak_point, = self.ax_spectrum.plot([self.est_doa], [0.0], marker="o", color="#3fb950", markersize=9, label="Estimated DOA")
        self.scan_cursor = self.ax_spectrum.axvline(0, color="#e3b341", lw=1.5, alpha=0.9)
        self.scan_dot, = self.ax_spectrum.plot([0], [self.power_db[0]], marker="o", color="#e3b341", markersize=7)
        self.spectrum_legend = self.ax_spectrum.legend(loc="lower right", fontsize=8.5, facecolor="#161b22", edgecolor="#30363d")

        # =========================================================================
        # PANEL 3: Oscilloscope
        # =========================================================================
        self.ax_osc.set_facecolor("#111622")
        self.ax_osc.set_xlabel("Time (milliseconds)", color="#8b949e", fontsize=9.5)
        self.ax_osc.set_ylabel("Amplitude", color="#8b949e", fontsize=9.5)
        self.ax_osc.grid(True, linestyle=":", alpha=0.35, color="#484f58")

        t_max = min(12.0, self.times[-1])
        mask = self.times <= t_max
        self.sub_t = self.times[mask]

        self.line_first, = self.ax_osc.plot(self.sub_t, np.array(self.waveforms[self.first_id])[mask],
                                            color="#3fb950", lw=2.0, label="First Sensor Hit")
        self.line_last, = self.ax_osc.plot(self.sub_t, np.array(self.waveforms[self.last_id])[mask],
                                           color="#f85149", lw=2.0, linestyle="--", label="Last Sensor Hit")
        self.t_cursor = self.ax_osc.axvline(0, color="#e3b341", lw=1.5, linestyle=":")
        self.osc_legend = self.ax_osc.legend(loc="upper right", fontsize=8.5, facecolor="#161b22", edgecolor="#30363d")

        self.update_titles()

    def update_titles(self):
        self.ax_array.set_title(f"Planar Wavefront Propagation & Capture: Source at {self.src_angle:.1f}° [RANDOM]",
                                fontsize=12.5, fontweight="bold", color="#58a6ff", pad=10)
        self.ax_spectrum.set_title(f"Bartlett Spatial Spectrum: Peak at {self.est_doa:.1f}° (True: {self.src_angle:.1f}°)",
                                   fontsize=11.5, fontweight="bold", color="#58a6ff", pad=10)
        self.ax_osc.set_title(f"Waveforms: Sensor {self.first_id} (Captured 1st, {self.delays[self.first_id]['delay_ms']:.2f} ms) vs Sensor {self.last_id} (Captured Last, {self.delays[self.last_id]['delay_ms']:.2f} ms)",
                              fontsize=11.0, fontweight="bold", color="#58a6ff", pad=10)

        # Update text badges for sensors
        for s in self.sensors:
            rank = self.delays[s["id"]]["rank"]
            t = self.sensor_texts[s["id"]]
            if rank <= 2:
                t.set_text(f"{s['id']}\n★#{rank}")
                t.set_bbox(dict(boxstyle="round,pad=0.2", facecolor="#e3b341", edgecolor="#ffffff", alpha=0.9))
            else:
                t.set_text(f"{s['id']}")
                t.set_bbox(dict(boxstyle="round,pad=0.2", facecolor="#30363d", edgecolor="#58a6ff", alpha=0.85))

    def update_plots_for_new_source(self):
        # Update source marker position
        src_dist = self.radius * 1.7
        src_rad = np.radians(self.src_angle)
        self.src_x = src_dist * np.cos(src_rad)
        self.src_y = src_dist * np.sin(src_rad)
        self.source_scat.set_offsets([[self.src_x, self.src_y]])

        # Update arrow annotation
        self.src_annot.set_position((self.src_x, self.src_y))
        self.src_annot.xy = (self.src_x * 0.75, self.src_y * 0.75)
        self.src_annot.set_text(f"Incoming Wave\n({self.src_angle:.1f}°)")

        # Update Bartlett Spectrum curve
        self.spectrum_line.set_ydata(self.power_db)
        self.true_line.set_xdata([self.src_angle])
        self.peak_point.set_data([self.est_doa], [0.0])

        # Update Oscilloscope waveforms
        t_max = min(12.0, self.times[-1])
        mask = self.times <= t_max
        self.line_first.set_ydata(np.array(self.waveforms[self.first_id])[mask])
        self.line_first.set_label(f"Sensor {self.first_id} (Captured 1st, {self.delays[self.first_id]['delay_ms']:.2f} ms)")
        self.line_last.set_ydata(np.array(self.waveforms[self.last_id])[mask])
        self.line_last.set_label(f"Sensor {self.last_id} (Captured Last, {self.delays[self.last_id]['delay_ms']:.2f} ms)")
        self.ax_osc.legend(loc="upper right", fontsize=8.5, facecolor="#161b22", edgecolor="#30363d")

        self.update_titles()

    def update(self, frame):
        if not self.anim_running:
            return self.wave_lines + [self.sensor_scat, self.scan_cursor, self.scan_dot, self.t_cursor, self.status_text]

        src_rad = np.radians(self.src_angle)
        prop_ux = np.cos(src_rad)
        prop_uy = np.sin(src_rad)
        perp_ux = -np.sin(src_rad)
        perp_uy = np.cos(src_rad)

        # 1. Update Wavefronts
        d_center = self.radius * 1.5 - ((frame % 60) / 60.0) * self.wavelength * 3.0
        for idx, line in enumerate(self.wave_lines):
            offset_dist = d_center - (idx - self.num_wavefronts // 2) * self.wavelength
            cx = offset_dist * prop_ux
            cy = offset_dist * prop_uy
            x_line = [cx - self.line_span * perp_ux, cx + self.line_span * perp_ux]
            y_line = [cy - self.line_span * perp_uy, cy + self.line_span * perp_uy]
            line.set_data(x_line, y_line)

            dist_to_center = abs(offset_dist)
            alpha = max(0.1, min(0.85, 1.0 - (dist_to_center / (self.radius * 1.8))))
            line.set_alpha(alpha)

        # 2. Dynamic Sensor Lighting: Sensors glow gold when a wave crest strikes them
        colors = []
        for m in range(self.M):
            tau_s = self.delays[m]["delay_ms"] / 1000.0
            time_now_s = (frame * 0.00015)
            wave_val = np.cos(2 * np.pi * 1000.0 * (time_now_s - tau_s))
            if wave_val > 0.6:
                colors.append("#e3b341") # Bright crest hit
            elif wave_val > 0.0:
                colors.append("#58a6ff") # Nominal
            else:
                colors.append("#21262d") # In trough
        self.sensor_scat.set_facecolor(colors)

        # 3. Update Bartlett Beam Scanning Cursor
        scan_angle = (frame * 2.0) % 360.0
        self.scan_cursor.set_xdata([scan_angle])
        curr_power = self.power_db[int(scan_angle)]
        self.scan_dot.set_data([scan_angle], [curr_power])

        # 4. Update Oscilloscope Cursor
        curr_t_ms = (frame % len(self.sub_t)) * (self.sub_t[-1] / len(self.sub_t))
        self.t_cursor.set_xdata([curr_t_ms])

        # 5. Status Text
        raw_err = abs(self.est_doa - self.src_angle)
        err = min(raw_err, 360.0 - raw_err)
        self.status_text.set_text(
            f"DYNAMIC SIGNAL DETECTION ENGINE:\n"
            f"• Source Direction (Random) : {self.src_angle:5.1f}°\n"
            f"• First Sensor Captured    : Sensor {self.first_id} (Lead: {abs(self.delays[self.first_id]['delay_ms']):.2f} ms)\n"
            f"• Scanning Azimuth Angle   : {scan_angle:5.1f}° (Power: {curr_power:5.1f} dB)\n"
            f"• Bartlett Peak Identified : {self.est_doa:5.1f}° (Error: {err:4.2f}°)\n"
            f"• Press 'R' or Click Button: Generate New Random Signal"
        )

        return self.wave_lines + [self.sensor_scat, self.scan_cursor, self.scan_dot, self.t_cursor, self.status_text]

    def run(self):
        self.anim = FuncAnimation(self.fig, self.update, frames=self.total_frames, interval=33, blit=False)

        # Save preview image
        preview_path = "docs/animated_capture_preview.png"
        self.fig.savefig(preview_path, dpi=200, facecolor=self.fig.get_facecolor(), bbox_inches="tight")

        if "--save-gif" in sys.argv:
            gif_path = "docs/wavefront_capture_simulation.gif"
            print(f"[INFO] Saving animation GIF to {gif_path} (60 frames)...")
            self.anim.save(gif_path, writer="pillow", fps=20)
            print(f"[INFO] Saved {gif_path}")
            plt.close(self.fig)
            return

        plt.show()

if __name__ == "__main__":
    dashboard = InteractiveBeamformingDashboard()
    dashboard.run()
