#!/usr/bin/env python3
"""Generate, run, and analyze the OBJ tree voxel-size sweep."""
from __future__ import annotations

import argparse
import csv
import json
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[1]
RESULTS = ROOT / "results"
SWEEP_ROOT = RESULTS / "size_sweep"
CONFIG_ROOT = ROOT / "size_sweep_configs"
EXE = "C:/work/bin_x64/Debug/histream.exe"
WINDOWS_ROOT = "C:/work/histream/examples/obj_tree_compare"
SIZES = (0.1, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0)
ANGLES = (0,)
WIDTH = HEIGHT = 96
IMAGE_NAME = "VZA={vza:.2f}_VAA=0.00_SZA=30.00_SAA=135.00.img"


def size_label(size: float) -> str:
    return f"{size:.2f}".replace(".", "p") + "m"


def make_config(size: float, model: str) -> tuple[Path, str]:
    label = size_label(size)
    source = ROOT / ("Input_voxel.xml" if model == "voxel" else "Input_hex.xml")
    tree = ET.parse(source)
    voxel_node = tree.find("./Scene/voxelSize")
    out_node = tree.find("./Control/outDir")
    if voxel_node is None or out_node is None:
        raise RuntimeError(f"missing voxelSize/outDir in {source}")
    voxel_node.text = f"{size:.6g}"
    out_node.text = f"{WINDOWS_ROOT}/results/size_sweep/{label}/{model}"
    CONFIG_ROOT.mkdir(parents=True, exist_ok=True)
    config_path = CONFIG_ROOT / f"Input_{model}_{label}.xml"
    tree.write(config_path, encoding="utf-8", xml_declaration=True)
    config_win = f"{WINDOWS_ROOT}/size_sweep_configs/{config_path.name}"
    (SWEEP_ROOT / label / model).mkdir(parents=True, exist_ok=True)
    return config_path, config_win


def run_model(size: float, model: str) -> None:
    config_path, config_win = make_config(size, model)
    mode = "eVoxelRT" if model == "voxel" else "eHexRT"
    label = size_label(size)
    print(f"RUN size={size:.2f} m model={model}", flush=True)
    result = subprocess.run(
        ["cmd.exe", "/d", "/c", EXE, mode, config_win],
        cwd=PROJECT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    log_path = SWEEP_ROOT / label / f"{model}.log"
    log_path.write_bytes(result.stdout)
    if result.returncode != 0:
        raise RuntimeError(
            f"{mode} failed for {size:.2f} m with exit code {result.returncode}; "
            f"see {log_path}"
        )
    print(f"DONE size={size:.2f} m model={model}", flush=True)


def read_image(path: Path) -> np.ndarray:
    image = np.fromfile(path, dtype="<f4")
    if image.size != WIDTH * HEIGHT:
        raise ValueError(f"{path}: expected {WIDTH * HEIGHT} values, got {image.size}")
    return image.reshape(HEIGHT, WIDTH)


def analyze() -> list[dict]:
    rows = []
    for size in SIZES:
        label = size_label(size)
        for vza in ANGLES:
            filename = IMAGE_NAME.format(vza=vza)
            facet = read_image(RESULTS / "facet" / filename)
            voxel = read_image(SWEEP_ROOT / label / "voxel" / filename)
            hex_image = read_image(SWEEP_ROOT / label / "hex" / filename)
            row = {
                "voxel_size_m": size,
                "vza_deg": vza,
                "facet_mean": float(facet.mean()),
            }
            for model, image in (("voxel", voxel), ("hex", hex_image)):
                delta = image - facet
                row[f"{model}_mean"] = float(image.mean())
                row[f"{model}_mae"] = float(np.mean(np.abs(delta)))
                row[f"{model}_rmse"] = float(np.sqrt(np.mean(delta * delta)))
                row[f"{model}_bias"] = float(np.mean(delta))
                row[f"{model}_corr"] = float(
                    np.corrcoef(image.ravel(), facet.ravel())[0, 1]
                )
            row["hex_mae_improvement_pct"] = 100.0 * (
                1.0 - row["hex_mae"] / row["voxel_mae"]
            )
            row["hex_rmse_improvement_pct"] = 100.0 * (
                1.0 - row["hex_rmse"] / row["voxel_rmse"]
            )
            rows.append(row)

    with (SWEEP_ROOT / "size_sweep_metrics.csv").open(
        "w", newline="", encoding="utf-8"
    ) as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (SWEEP_ROOT / "size_sweep_metrics.json").write_text(
        json.dumps(rows, indent=2), encoding="utf-8"
    )
    vza0_rows = [row for row in rows if row["vza_deg"] == 0]
    with (SWEEP_ROOT / "size_sweep_vza0_metrics.csv").open(
        "w", newline="", encoding="utf-8"
    ) as stream:
        writer = csv.DictWriter(stream, fieldnames=list(vza0_rows[0]))
        writer.writeheader()
        writer.writerows(vza0_rows)
    (SWEEP_ROOT / "size_sweep_vza0_metrics.json").write_text(
        json.dumps(vza0_rows, indent=2), encoding="utf-8"
    )
    write_summary(rows)
    write_svg(rows)
    return rows


def write_summary(rows: list[dict]) -> None:
    summary = []
    for size in SIZES:
        selected = [row for row in rows if row["voxel_size_m"] == size]
        voxel_mae = sum(row["voxel_mae"] for row in selected) / len(selected)
        hex_mae = sum(row["hex_mae"] for row in selected) / len(selected)
        voxel_rmse = sum(row["voxel_rmse"] for row in selected) / len(selected)
        hex_rmse = sum(row["hex_rmse"] for row in selected) / len(selected)
        summary.append({
            "voxel_size_m": size,
            "mean_voxel_mae": voxel_mae,
            "mean_hex_mae": hex_mae,
            "hex_mae_improvement_pct": 100.0 * (1.0 - hex_mae / voxel_mae),
            "mean_voxel_rmse": voxel_rmse,
            "mean_hex_rmse": hex_rmse,
            "hex_rmse_improvement_pct": 100.0 * (1.0 - hex_rmse / voxel_rmse),
        })
    with (SWEEP_ROOT / "size_sweep_summary.csv").open(
        "w", newline="", encoding="utf-8"
    ) as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summary[0]))
        writer.writeheader()
        writer.writerows(summary)
    (SWEEP_ROOT / "size_sweep_summary.json").write_text(
        json.dumps(summary, indent=2), encoding="utf-8"
    )

def write_svg(rows: list[dict]) -> None:
    width, height = 980, 460
    margin_x, margin_y = 70, 62
    panel_w, panel_h, gap = 390, 300, 70
    x_min, x_max = min(SIZES), max(SIZES)
    panel_rows = sorted(rows, key=lambda row: row["voxel_size_m"])
    parts = [
        f"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"{width}\" height=\"{height}\" viewBox=\"0 0 {width} {height}\">",
        "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>",
        "<style>text{font-family:Arial,sans-serif;fill:#222}.axis{stroke:#333;stroke-width:1}.grid{stroke:#ddd;stroke-width:1}.voxel{stroke:#d1495b;fill:none;stroke-width:3}.hex{stroke:#0077b6;fill:none;stroke-width:3}.point-voxel{fill:#d1495b}.point-hex{fill:#0077b6}</style>",
        "<text x=\"490\" y=\"27\" text-anchor=\"middle\" font-size=\"19\">VZA=0°: voxel-size sensitivity relative to facet reference</text>",
    ]
    for panel_index, (metric, title) in enumerate((("mae", "MAE"), ("rmse", "RMSE"))):
        x0 = margin_x + panel_index * (panel_w + gap)
        y0 = margin_y
        bottom = y0 + panel_h
        y_max = max(max(row[f"voxel_{metric}"], row[f"hex_{metric}"])
                    for row in panel_rows) * 1.08
        parts.append(f"<text x=\"{x0 + panel_w / 2}\" y=\"{y0 - 12}\" text-anchor=\"middle\" font-size=\"16\">{title}</text>")
        for tick in range(6):
            value = y_max * tick / 5
            y = bottom - panel_h * tick / 5
            parts.append(f"<line class=\"grid\" x1=\"{x0}\" y1=\"{y:.2f}\" x2=\"{x0 + panel_w}\" y2=\"{y:.2f}\"/>")
            parts.append(f"<text x=\"{x0 - 9}\" y=\"{y + 4:.2f}\" text-anchor=\"end\" font-size=\"11\">{value:.3f}</text>")
        for size in SIZES:
            x = x0 + panel_w * np.log(size / x_min) / np.log(x_max / x_min)
            parts.append(f"<line class=\"grid\" x1=\"{x:.2f}\" y1=\"{y0}\" x2=\"{x:.2f}\" y2=\"{bottom}\"/>")
            parts.append(f"<text x=\"{x:.2f}\" y=\"{bottom + 20}\" text-anchor=\"middle\" font-size=\"11\">{size:g}</text>")
        parts.append(f"<line class=\"axis\" x1=\"{x0}\" y1=\"{bottom}\" x2=\"{x0 + panel_w}\" y2=\"{bottom}\"/>")
        parts.append(f"<line class=\"axis\" x1=\"{x0}\" y1=\"{y0}\" x2=\"{x0}\" y2=\"{bottom}\"/>")
        for model, cls in (("voxel", "voxel"), ("hex", "hex")):
            points = []
            for row in panel_rows:
                x = x0 + panel_w * np.log(row["voxel_size_m"] / x_min) / np.log(x_max / x_min)
                y = bottom - panel_h * row[f"{model}_{metric}"] / y_max
                points.append((x, y))
            points_text = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
            parts.append(f"<polyline class=\"{cls}\" points=\"{points_text}\"/>")
            for x, y in points:
                parts.append(f"<circle class=\"point-{cls}\" cx=\"{x:.2f}\" cy=\"{y:.2f}\" r=\"3.5\"/>")
        parts.append(f"<text x=\"{x0 + panel_w / 2}\" y=\"{bottom + 43}\" text-anchor=\"middle\" font-size=\"13\">Voxel size (m)</text>")
    parts.extend([
        "<line class=\"voxel\" x1=\"265\" y1=\"430\" x2=\"305\" y2=\"430\"/>",
        "<text x=\"313\" y=\"435\" font-size=\"13\">VoxelRT homogeneous</text>",
        "<line class=\"hex\" x1=\"555\" y1=\"430\" x2=\"595\" y2=\"430\"/>",
        "<text x=\"603\" y=\"435\" font-size=\"13\">HexRT heterogeneous</text>",
        "</svg>",
    ])
    (SWEEP_ROOT / "size_sweep_mae_rmse.svg").write_text("\n".join(parts), encoding="utf-8")
    (SWEEP_ROOT / "size_sweep_mae.svg").unlink(missing_ok=True)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--analyze-only", action="store_true")
    args = parser.parse_args()
    if not args.analyze_only:
        for size in SIZES:
            for model in ("voxel", "hex"):
                run_model(size, model)
    rows = analyze()
    print(json.dumps([row for row in rows if row["vza_deg"] == 0], indent=2))


if __name__ == "__main__":
    main()
