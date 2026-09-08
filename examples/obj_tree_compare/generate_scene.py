#!/usr/bin/env python3
"""Generate a deterministic randomly distributed multi-tree OBJ comparison scene."""
import argparse
import json
import math
import random
from pathlib import Path

SEED = 20260901
POSITION_SEED = 20260902
TREE_COUNT = 25
TREE_POSITION_LIMIT = 11.5
MIN_TREE_SPACING = 3.2
LEAVES_PER_TREE = 320
SCENE_SIZE = 30.0
SCENE_HEIGHT = 8.0
VOXEL_SIZE = 2.5
CROWN_RX, CROWN_RY, CROWN_RZ = 1.45, 2.25, 1.45
CROWN_CENTER_Y = 3.4
LEAF_HALF_LENGTH, LEAF_HALF_WIDTH = 0.14, 0.055


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def mul(a, value):
    return tuple(x * value for x in a)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def normalize(a):
    length = math.sqrt(sum(x * x for x in a))
    return tuple(x / length for x in a)


def leaf_position(rng, tree_x, tree_z, phase, leaf_index):
    lobes = ((-0.43, -0.18, -0.30), (0.40, -0.05, 0.30),
             (0.00, 0.40, 0.00), (0.12, 0.05, -0.43))
    lx, ly, lz = lobes[leaf_index % len(lobes)]
    cp, sp = math.cos(phase), math.sin(phase)
    lx, lz = lx * cp - lz * sp, lx * sp + lz * cp
    for _ in range(100):
        x = lx + rng.gauss(0.0, 0.24)
        y = ly + rng.gauss(0.0, 0.25)
        z = lz + rng.gauss(0.0, 0.24)
        if x * x + y * y + z * z <= 0.95:
            return (tree_x + x * CROWN_RX,
                    CROWN_CENTER_Y + y * CROWN_RY,
                    tree_z + z * CROWN_RZ)
    return tree_x, CROWN_CENTER_Y, tree_z


def leaf_axes(rng, phase):
    # Spherical leaf-angle distribution: E(|n·d|)=0.5 for every direction,
    # exactly matching the canopy G=0.5 used by VoxelRT and HexRT.
    cos_tilt = rng.random()
    tilt = math.acos(cos_tilt)
    azimuth = rng.uniform(0.0, 2.0 * math.pi)
    normal = (math.sin(tilt) * math.cos(azimuth), cos_tilt,
              math.sin(tilt) * math.sin(azimuth))
    tangent = normalize((-math.sin(azimuth), 0.0, math.cos(azimuth)))
    return tangent, normalize(cross(normal, tangent))


def random_tree_positions():
    """Deterministic Poisson-like placement without a row/column pattern."""
    rng = random.Random(POSITION_SEED)
    positions = []
    for _ in range(200000):
        candidate = (rng.uniform(-TREE_POSITION_LIMIT, TREE_POSITION_LIMIT),
                     rng.uniform(-TREE_POSITION_LIMIT, TREE_POSITION_LIMIT))
        if all(math.hypot(candidate[0] - x, candidate[1] - z) >= MIN_TREE_SPACING
               for x, z in positions):
            positions.append(candidate)
            if len(positions) == TREE_COUNT:
                return positions
    raise RuntimeError("unable to place all trees with the requested spacing")


def generate_obj(output_dir):
    rng = random.Random(SEED)
    lines = ["# randomly distributed 25-tree leaf-facet scene",
             "mtllib multi_tree.mtl", "o Crown", "usemtl Crown", "s off"]
    vertex_index = 1
    trees = []
    for tree_id, (tree_x, tree_z) in enumerate(random_tree_positions(), start=1):
        phase = math.radians((tree_id * 37) % 180)
        trees.append({"id": tree_id, "x": tree_x, "z": tree_z})
        lines.append(f"# tree {tree_id:02d}")
        for leaf_index in range(LEAVES_PER_TREE):
            center = leaf_position(rng, tree_x, tree_z, phase, leaf_index)
            tangent, bitangent = leaf_axes(rng, phase)
            points = (
                add(add(center, mul(tangent, -LEAF_HALF_LENGTH)), mul(bitangent, -LEAF_HALF_WIDTH)),
                add(add(center, mul(tangent, LEAF_HALF_LENGTH)), mul(bitangent, -LEAF_HALF_WIDTH)),
                add(add(center, mul(tangent, LEAF_HALF_LENGTH)), mul(bitangent, LEAF_HALF_WIDTH)),
                add(add(center, mul(tangent, -LEAF_HALF_LENGTH)), mul(bitangent, LEAF_HALF_WIDTH)),
            )
            for p in points:
                lines.append(f"v {p[0]:.6f} {p[1]:.6f} {p[2]:.6f}")
            lines.append(f"f {vertex_index} {vertex_index + 1} {vertex_index + 2}")
            lines.append(f"f {vertex_index} {vertex_index + 2} {vertex_index + 3}")
            vertex_index += 4
    (output_dir / "multi_tree.obj").write_text("\n".join(lines) + "\n", encoding="ascii")
    (output_dir / "multi_tree.mtl").write_text(
        "newmtl Crown\nKd 0.08 0.42 0.10\nKa 0.01 0.05 0.01\n", encoding="ascii")
    (output_dir / "object_position.txt").write_text("15 15 0\n", encoding="ascii")
    return trees, vertex_index - 1


def input_xml(windows_dir, result_name):
    output_dir = f"{windows_dir}/results/{result_name}"
    return f"""<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<Information>
  <Control>
    <rayTracingDepth>6</rayTracingDepth><isTemperature>0</isTemperature>
    <isAlbedo>0</isAlbedo><isImage>1</isImage><isDisplay>0</isDisplay>
    <GPU>0</GPU><outDir>{output_dir}</outDir>
  </Control>
  <Geometry>
    <Sensor><sensor id="1" name="tree_compare">
      <pixelResolutionX>96</pixelResolutionX><pixelResolutionY>96</pixelResolutionY>
      <bandNum>1</bandNum><controlBand>850</controlBand>
      <viewAngle>
        <viewAngle type="custom" num="2">
          <viewAngles id="1">0,0</viewAngles><viewAngles id="2">50,0</viewAngles>
        </viewAngle>
        <viewAngle type="BRF"><SPP>0</SPP><CSPP>0</CSPP><vzaMax>50</vzaMax><vzaStep>10</vzaStep></viewAngle>
        <viewAngle type="albedo"><enabled>0</enabled><angleNum>100</angleNum></viewAngle>
      </viewAngle>
    </sensor></Sensor>
    <Light><light id="1" name="sun">
      <lightAngle num="1"><lightAngle id="1">30,135</lightAngle></lightAngle>
      <directScatteringRatio>0.8</directScatteringRatio><skyTemperature>250</skyTemperature>
    </light></Light>
  </Geometry>
  <Attribute>
    <Spectral>
      <spectral type="custom" id="1" name="leaf">
        <reflectance>0.45</reflectance><transmittance>0.35</transmittance>
        <tau_TIR>0.01</tau_TIR><ref_TIR>0.01</ref_TIR>
      </spectral>
      <spectral type="custom" id="2" name="soil">
        <reflectance>0.24</reflectance><transmittance>0.0</transmittance>
        <tau_TIR>0.0</tau_TIR><ref_TIR>0.05</ref_TIR>
      </spectral>
    </Spectral>
    <Thermal>
      <thermal id="1" name="leaf_temperature"><sunlitTemperature>300</sunlitTemperature><shadedTemperature>298</shadedTemperature></thermal>
      <thermal id="2" name="soil_temperature"><sunlitTemperature>305</sunlitTemperature><shadedTemperature>295</shadedTemperature></thermal>
    </Thermal>
    <Canopy><canopy id="1" name="tree_canopy">
      <lai>2.0</lai><density>0.5</density><hc>5.7</hc><G>0.5</G>
      <LIDFa>-0.35</LIDFa><LIDFb>-0.15</LIDFb><hspot>0.2</hspot><leafwidth>0.22</leafwidth>
    </canopy></Canopy>
    <Biochemistry>
      <LeafBio><leafBio id="1" name="leafbio">
        <Vcmax>60</Vcmax><m>9</m><BallBerry>0.01</BallBerry><Type>3</Type>
        <kV>0.6396</kV><Rdparam>0.015</Rdparam><Tparam>0.2,0.3,288,313,328</Tparam>
        <Tyear>25</Tyear><beta>0.507</beta><kNPQs>0</kNPQs><qLs>1</qLs>
        <stressfactor>1</stressfactor><Tcor>0</Tcor>
      </leafBio></LeafBio>
      <SoilSet><soilSet id="1" name="soilset">
        <method>1</method><rss>2000</rss><cs>1180</cs><rhos>1800</rhos>
        <lambdas>1.55</lambdas><Tsoil>25</Tsoil><SMC>0.25</SMC><Satwater>0.45</Satwater>
      </soilSet></SoilSet>
    </Biochemistry>
  </Attribute>
  <Scene>
    <sceneSizeX>{SCENE_SIZE}</sceneSizeX><sceneSizeY>{SCENE_SIZE}</sceneSizeY>
    <Height>{SCENE_HEIGHT}</Height><voxelSize>{VOXEL_SIZE}</voxelSize>
    <voxelFillThreshold>0.0</voxelFillThreshold>
    <bgSpectral>soil</bgSpectral><bgThermal>soil_temperature</bgThermal>
    <bgBioName>soilset</bgBioName><bgBioType>soilSet</bgBioType>
    <Object type="face"><object id="1" objName="multi_tree">
      <fileName>{windows_dir}/multi_tree.obj</fileName><meshNames>Crown</meshNames>
      <spectralNames>leaf</spectralNames><thermalNames>leaf_temperature</thermalNames>
      <canopyNames>tree_canopy</canopyNames><bioNames>leafbio</bioNames><bioTypes>leafBio</bioTypes>
      <types>Vegetation</types><shapeTypes>ellipsoid</shapeTypes><shapes>3.0,3.0,5.0</shapes>
      <objectPosition>{windows_dir}/object_position.txt</objectPosition>
    </object></Object>
  </Scene>
</Information>
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--windows-dir", default="C:/work/histream/examples/obj_tree_compare")
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    trees, vertex_count = generate_obj(args.output_dir)
    for config_name, result_name in (
        ("Input_facet.xml", "facet"), ("Input_voxel.xml", "voxel"), ("Input_hex.xml", "hex")
    ):
        (args.output_dir / config_name).write_text(
            input_xml(args.windows_dir.rstrip("/"), result_name), encoding="utf-8")
    manifest = {
        "seed": SEED, "position_seed": POSITION_SEED,
        "distribution": "poisson_random", "minimum_tree_spacing_m": MIN_TREE_SPACING,
        "tree_count": len(trees), "leaves_per_tree": LEAVES_PER_TREE,
        "leaf_count": len(trees) * LEAVES_PER_TREE, "vertex_count": vertex_count,
        "triangle_count": len(trees) * LEAVES_PER_TREE * 2,
        "scene_size_m": [SCENE_SIZE, SCENE_SIZE, SCENE_HEIGHT],
        "crown_diameter_m": [CROWN_RX * 2, CROWN_RZ * 2],
        "crown_height_m": CROWN_RY * 2, "voxel_size_m": VOXEL_SIZE,
        "view_angles_deg": [[0, 0], [50, 0]], "trees": trees,
    }
    (args.output_dir / "scene_manifest.json").write_text(
        json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps({key: manifest[key] for key in (
        "tree_count", "leaf_count", "vertex_count", "triangle_count",
        "voxel_size_m", "view_angles_deg")}, indent=2))


if __name__ == "__main__":
    main()
