#!/usr/bin/env python3
"""Compare facet, homogeneous voxel, and heterogeneous voxel outputs."""
from __future__ import annotations

import csv
import json
import struct
import zlib
from pathlib import Path

import numpy as np
from osgeo import gdal

gdal.UseExceptions()

ROOT = Path(__file__).resolve().parent
RESULTS = ROOT / "results"
WIDTH = HEIGHT = 96
ANGLES = (0,)
FILE_PATTERN = "VZA={vza:.2f}_VAA=0.00_SZA=30.00_SAA=135.00.img"


def read_image(model: str, vza: int) -> np.ndarray:
    filename = FILE_PATTERN.format(vza=vza)
    path = (RESULTS / model / filename if model == "facet" else
            RESULTS / "size_sweep" / "2p50m" / model / filename)
    values = np.fromfile(path, dtype="<f4")
    expected = WIDTH * HEIGHT
    if values.size != expected:
        raise ValueError(f"{path}: expected {expected} float32 values, got {values.size}")
    return values.reshape(HEIGHT, WIDTH)


def summarize(vza: int) -> tuple[dict, dict[str, np.ndarray]]:
    images = {name: read_image(name, vza) for name in ("facet", "voxel", "hex")}
    facet = images["facet"]
    row = {"vza_deg": vza, "facet_mean": float(facet.mean())}
    for model in ("voxel", "hex"):
        delta = images[model] - facet
        row[f"{model}_mean"] = float(images[model].mean())
        row[f"{model}_mae"] = float(np.mean(np.abs(delta)))
        row[f"{model}_rmse"] = float(np.sqrt(np.mean(delta * delta)))
        row[f"{model}_bias"] = float(np.mean(delta))
        row[f"{model}_corr"] = float(np.corrcoef(images[model].ravel(), facet.ravel())[0, 1])
    row["hex_mae_improvement_pct"] = 100.0 * (
        1.0 - row["hex_mae"] / row["voxel_mae"]
    )
    row["hex_rmse_improvement_pct"] = 100.0 * (
        1.0 - row["hex_rmse"] / row["voxel_rmse"]
    )
    return row, images


def colorize(values: np.ndarray, vmax: float, error: bool = False) -> np.ndarray:
    x = np.clip(values / max(vmax, 1.0e-12), 0.0, 1.0)
    if error:
        rgb = np.stack((x, 0.65 * x * x, 0.08 * (1.0 - x)), axis=-1)
    else:
        red = np.clip(1.5 - np.abs(4.0 * x - 3.0), 0.0, 1.0)
        green = np.clip(1.5 - np.abs(4.0 * x - 2.0), 0.0, 1.0)
        blue = np.clip(1.5 - np.abs(4.0 * x - 1.0), 0.0, 1.0)
        rgb = np.stack((red, green, blue), axis=-1)
    return np.asarray(np.rint(rgb * 255.0), dtype=np.uint8)


def write_png(path: Path, rgb: np.ndarray) -> None:
    height, width, _ = rgb.shape

    def chunk(kind: bytes, payload: bytes) -> bytes:
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    scanlines = b"".join(b"\x00" + row.tobytes() for row in rgb)
    data = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(scanlines, 9))
        + chunk(b"IEND", b"")
    )
    path.write_bytes(data)


def write_geotiff(path: Path, values: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    dataset = gdal.GetDriverByName("GTiff").Create(
        str(path), WIDTH, HEIGHT, 1, gdal.GDT_Float32,
        options=["COMPRESS=LZW", "PREDICTOR=3"])
    if dataset is None:
        raise RuntimeError(f"cannot create {path}")
    pixel_size = 30.0 / WIDTH
    dataset.SetGeoTransform((-15.0, pixel_size, 0.0, 15.0, 0.0, -pixel_size))
    dataset.GetRasterBand(1).WriteArray(values.astype(np.float32, copy=False))
    dataset.FlushCache()
    dataset = None


def write_plot(all_images: dict[int, dict[str, np.ndarray]]) -> None:
    gap = 4
    canvas = np.full(
        (len(ANGLES) * HEIGHT + (len(ANGLES) - 1) * gap,
         5 * WIDTH + 4 * gap, 3),
        255, dtype=np.uint8,
    )
    for row_index, vza in enumerate(ANGLES):
        images = all_images[vza]
        radiance_max = max(float(images[name].max()) for name in ("facet", "voxel", "hex"))
        errors = (np.abs(images["voxel"] - images["facet"]),
                  np.abs(images["hex"] - images["facet"]))
        error_max = max(float(error.max()) for error in errors)
        panels = (
            colorize(images["facet"], radiance_max),
            colorize(images["voxel"], radiance_max),
            colorize(images["hex"], radiance_max),
            colorize(errors[0], error_max, error=True),
            colorize(errors[1], error_max, error=True),
        )
        y0 = row_index * (HEIGHT + gap)
        for col, panel in enumerate(panels):
            x0 = col * (WIDTH + gap)
            canvas[y0:y0 + HEIGHT, x0:x0 + WIDTH] = panel
    write_png(RESULTS / "comparison_vza0.png", canvas)


def main() -> None:
    rows = []
    all_images = {}
    for vza in ANGLES:
        row, images = summarize(vza)
        rows.append(row)
        all_images[vza] = images

    with (RESULTS / "comparison_metrics.csv").open(
        "w", newline="", encoding="utf-8"
    ) as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    (RESULTS / "comparison_metrics.json").write_text(
        json.dumps(rows, indent=2, ensure_ascii=False), encoding="utf-8"
    )
    write_plot(all_images)
    tif_dir = RESULTS / "tif_preview"
    images = all_images[0]
    write_geotiff(tif_dir / "facet_vza0.tif", images["facet"])
    write_geotiff(tif_dir / "voxel_2p50m_vza0.tif", images["voxel"])
    write_geotiff(tif_dir / "hex_2p50m_vza0.tif", images["hex"])
    print(json.dumps(rows, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
