
#!/usr/bin/env python3
"""Analyze the VZA=0 thermal random-tree voxel-size sweep."""
from __future__ import annotations

import csv
import json
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
RESULTS = ROOT / "results" / "temperature_random"
SIZES = (0.10, 0.25, 0.50, 0.75, 1.00, 1.50, 2.00, 2.50, 3.00, 4.00, 5.00)
WIDTH = HEIGHT = 96
IMAGE_NAME = "VZA=0.00_VAA=0.00_SZA=30.00_SAA=135.00.img"
WAVELENGTH_UM = 10.5
PLANCK_C1 = 11910.439340652 * 10000.0
PLANCK_C2 = 14388.291040407


def size_label(size: float) -> str:
    return f"{size:.2f}".replace(".", "p") + "m"


def read_image(path: Path) -> np.ndarray:
    radiance = np.fromfile(path, dtype="<f4")
    expected = WIDTH * HEIGHT
    if radiance.size != expected:
        raise ValueError(f"{path}: expected {expected} values, got {radiance.size}")
    if not np.all(np.isfinite(radiance)) or np.any(radiance <= 0.0):
        raise ValueError(f"{path}: contains invalid radiance values")
    radiance = radiance.reshape(HEIGHT, WIDTH).astype(np.float64)
    return PLANCK_C2 / (
        WAVELENGTH_UM
        * np.log(PLANCK_C1 / (radiance * WAVELENGTH_UM**5) + 1.0)
    )


def calculate_metrics() -> list[dict[str, float]]:
    facet = read_image(RESULTS / "facet" / IMAGE_NAME)
    rows: list[dict[str, float]] = []
    for size in SIZES:
        label = size_label(size)
        voxel = read_image(RESULTS / label / "voxel" / IMAGE_NAME)
        hex_image = read_image(RESULTS / label / "hex" / IMAGE_NAME)
        row: dict[str, float] = {
            "voxel_size_m": size,
            "facet_mean_K": float(facet.mean()),
        }
        for model, image in (("voxel", voxel), ("hex", hex_image)):
            delta = image - facet
            row[f"{model}_mean_K"] = float(image.mean())
            row[f"{model}_mae_K"] = float(np.mean(np.abs(delta)))
            row[f"{model}_rmse_K"] = float(np.sqrt(np.mean(delta * delta)))
            row[f"{model}_bias_K"] = float(np.mean(delta))
            row[f"{model}_corr"] = float(
                np.corrcoef(image.ravel(), facet.ravel())[0, 1]
            )
        row["hex_rmse_improvement_pct"] = 100.0 * (
            1.0 - row["hex_rmse_K"] / row["voxel_rmse_K"]
        )
        rows.append(row)
    return rows


def write_metrics(rows: list[dict[str, float]]) -> None:
    with (RESULTS / "temperature_random_vza0_metrics.csv").open(
        "w", newline="", encoding="utf-8"
    ) as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (RESULTS / "temperature_random_vza0_metrics.json").write_text(
        json.dumps(rows, indent=2), encoding="utf-8"
    )


def write_figure(rows: list[dict[str, float]]) -> None:
    sizes = np.asarray([row["voxel_size_m"] for row in rows])
    facet_mean = rows[0]["facet_mean_K"]
    fig, axes = plt.subplots(1, 2, figsize=(11.2, 4.3), constrained_layout=True)

    axes[0].plot(
        sizes, [row["voxel_rmse_K"] for row in rows],
        "o-", linewidth=2, label="Voxel homogeneous"
    )
    axes[0].plot(
        sizes, [row["hex_rmse_K"] for row in rows],
        "o-", linewidth=2, label="Hex heterogeneous"
    )
    axes[0].set_ylabel("RMSE (K)")
    axes[0].set_title("Temperature error relative to facets")
    axes[0].legend()

    axes[1].axhline(
        facet_mean, color="black", linestyle="--", linewidth=1.5,
        label=f"Facet ({facet_mean:.2f} K)"
    )
    axes[1].plot(
        sizes, [row["voxel_mean_K"] for row in rows],
        "o-", linewidth=2, label="Voxel homogeneous"
    )
    axes[1].plot(
        sizes, [row["hex_mean_K"] for row in rows],
        "o-", linewidth=2, label="Hex heterogeneous"
    )
    axes[1].set_ylabel("Scene mean temperature (K)")
    axes[1].set_title("Mean temperature")
    axes[1].legend()

    for axis in axes:
        axis.set_xscale("log")
        axis.set_xticks(sizes, [f"{size:g}" for size in sizes], rotation=35)
        axis.set_xlabel("Voxel size (m)")
        axis.grid(True, alpha=0.3)

    fig.suptitle("Random 25-tree OBJ scene, VZA=0 deg, 10.5 um")
    fig.savefig(RESULTS / "temperature_random_size_sweep.png", dpi=180)
    plt.close(fig)


def main() -> None:
    rows = calculate_metrics()
    write_metrics(rows)
    write_figure(rows)
    print(json.dumps(rows, indent=2))


if __name__ == "__main__":
    main()
