
#!/usr/bin/env python3
"""Create spatial temperature/error maps for selected random-tree voxel sizes."""
from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
RESULTS = ROOT / "results" / "temperature_random"
SIZES = (0.10, 0.75, 2.00, 4.00)
WIDTH = HEIGHT = 96
IMAGE_NAME = "VZA=0.00_VAA=0.00_SZA=30.00_SAA=135.00.img"
WAVELENGTH_UM = 10.5
PLANCK_C1 = 11910.439340652 * 10000.0
PLANCK_C2 = 14388.291040407


def size_label(size: float) -> str:
    return f"{size:.2f}".replace(".", "p") + "m"


def read_temperature(path: Path) -> np.ndarray:
    radiance = np.fromfile(path, dtype="<f4")
    if radiance.size != WIDTH * HEIGHT:
        raise ValueError(f"{path}: expected {WIDTH * HEIGHT} values, got {radiance.size}")
    radiance = radiance.reshape(HEIGHT, WIDTH).astype(np.float64)
    if not np.all(np.isfinite(radiance)) or np.any(radiance <= 0.0):
        raise ValueError(f"{path}: contains invalid radiance")
    return PLANCK_C2 / (
        WAVELENGTH_UM
        * np.log(PLANCK_C1 / (radiance * WAVELENGTH_UM**5) + 1.0)
    )


def main() -> None:
    facet = read_temperature(RESULTS / "facet" / IMAGE_NAME)
    images: dict[float, tuple[np.ndarray, np.ndarray]] = {}
    all_temperatures = [facet]
    all_abs_errors: list[np.ndarray] = []
    all_improvements: list[np.ndarray] = []

    gradient_y, gradient_x = np.gradient(facet)
    gradient = np.hypot(gradient_x, gradient_y)
    edge_threshold = np.percentile(gradient, 80.0)
    edge_mask = gradient >= edge_threshold

    summary: list[dict[str, float]] = []
    for size in SIZES:
        label = size_label(size)
        voxel = read_temperature(RESULTS / label / "voxel" / IMAGE_NAME)
        hex_image = read_temperature(RESULTS / label / "hex" / IMAGE_NAME)
        images[size] = (voxel, hex_image)
        all_temperatures.extend((voxel, hex_image))

        voxel_abs = np.abs(voxel - facet)
        hex_abs = np.abs(hex_image - facet)
        improvement = voxel_abs - hex_abs
        all_abs_errors.extend((voxel_abs, hex_abs))
        all_improvements.append(improvement)
        summary.append({
            "voxel_size_m": size,
            "voxel_rmse_K": float(np.sqrt(np.mean((voxel - facet) ** 2))),
            "hex_rmse_K": float(np.sqrt(np.mean((hex_image - facet) ** 2))),
            "hex_closer_pixel_pct": float(100.0 * np.mean(hex_abs < voxel_abs)),
            "voxel_edge_mae_K": float(voxel_abs[edge_mask].mean()),
            "hex_edge_mae_K": float(hex_abs[edge_mask].mean()),
            "voxel_nonedge_mae_K": float(voxel_abs[~edge_mask].mean()),
            "hex_nonedge_mae_K": float(hex_abs[~edge_mask].mean()),
        })

    temp_stack = np.stack(all_temperatures)
    temp_min, temp_max = np.percentile(temp_stack, (0.5, 99.5))
    error_max = float(np.percentile(np.stack(all_abs_errors), 99.0))
    improvement_max = float(
        np.percentile(np.abs(np.stack(all_improvements)), 99.0)
    )

    fig, axes = plt.subplots(
        len(SIZES), 6, figsize=(15.5, 10.2),
        constrained_layout=True, squeeze=False
    )
    temperature_map = error_map = improvement_map = None
    for row, size in enumerate(SIZES):
        voxel, hex_image = images[size]
        voxel_abs = np.abs(voxel - facet)
        hex_abs = np.abs(hex_image - facet)
        improvement = voxel_abs - hex_abs
        panels = (
            (facet, "Facet", "inferno", temp_min, temp_max),
            (voxel, "Voxel", "inferno", temp_min, temp_max),
            (hex_image, "Hex", "inferno", temp_min, temp_max),
            (voxel_abs, "|Voxel - Facet|", "magma", 0.0, error_max),
            (hex_abs, "|Hex - Facet|", "magma", 0.0, error_max),
            (
                improvement,
                "|Voxel error| - |Hex error|",
                "RdBu_r",
                -improvement_max,
                improvement_max,
            ),
        )
        for column, (data, title, cmap, vmin, vmax) in enumerate(panels):
            image = axes[row, column].imshow(
                data, cmap=cmap, vmin=vmin, vmax=vmax, interpolation="nearest"
            )
            axes[row, column].set_xticks([])
            axes[row, column].set_yticks([])
            if row == 0:
                axes[row, column].set_title(title, fontsize=10)
            if column == 0:
                axes[row, column].set_ylabel(
                    f"{size:.2f} m", rotation=90, fontsize=10
                )
            if column == 0:
                temperature_map = image
            elif column == 3:
                error_map = image
            elif column == 5:
                improvement_map = image

    fig.colorbar(
        temperature_map, ax=axes[:, :3], location="bottom",
        shrink=0.7, pad=0.03, label="Brightness temperature (K)"
    )
    fig.colorbar(
        error_map, ax=axes[:, 3:5], location="bottom",
        shrink=0.7, pad=0.03, label="Absolute error (K)"
    )
    fig.colorbar(
        improvement_map, ax=axes[:, 5], location="bottom",
        shrink=0.9, pad=0.03,
        label="Error reduction (K): red = Hex closer, blue = Voxel closer"
    )
    fig.suptitle("Random 25-tree scene: spatial differences at VZA=0 deg")
    fig.savefig(
        RESULTS / "temperature_random_spatial_differences.png",
        dpi=180, bbox_inches="tight"
    )
    plt.close(fig)

    with (RESULTS / "temperature_random_spatial_difference_summary.csv").open(
        "w", newline="", encoding="utf-8"
    ) as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summary[0]))
        writer.writeheader()
        writer.writerows(summary)


if __name__ == "__main__":
    main()
