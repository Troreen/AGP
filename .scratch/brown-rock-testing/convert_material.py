"""Convert BrownRock08 PNG maps into mipmapped runtime DDS textures.

Run from the repository root with Python, Pillow and numpy:
    python .scratch/brown-rock-testing/convert_material.py <download-folder>
"""

import argparse
import io
from pathlib import Path
import struct

import numpy as np
from PIL import Image


def mip_chain(image, kind):
    yield image
    while image.width > 1 or image.height > 1:
        size = (max(1, image.width // 2), max(1, image.height // 2))
        if kind == "color":
            values = np.asarray(image, dtype=np.float32) / 255
            linear = np.where(values <= 0.04045, values / 12.92,
                              ((values + 0.055) / 1.055) ** 2.4)
            channels = [np.asarray(Image.fromarray(linear[..., c]).resize(size, Image.Resampling.BOX))
                        for c in range(3)]
            linear = np.stack(channels, axis=-1)
            values = np.where(linear <= 0.0031308, linear * 12.92,
                              1.055 * linear ** (1 / 2.4) - 0.055)
            image = Image.fromarray(np.uint8(np.clip(values * 255 + 0.5, 0, 255)))
        elif kind == "normal":
            image = image.resize(size, Image.Resampling.BOX)
            normal = np.asarray(image, dtype=np.float32) / 127.5 - 1
            normal /= np.maximum(np.linalg.norm(normal, axis=-1, keepdims=True), 1e-8)
            image = Image.fromarray(np.uint8(np.clip((normal + 1) * 127.5 + 0.5, 0, 255)))
        else:
            image = image.resize(size, Image.Resampling.BOX)
        yield image


def write_dds(path, image, kind):
    mips = list(mip_chain(image, kind))
    # DXGI: BC1_UNORM_SRGB, BC5_UNORM, BC1_UNORM, R16_UNORM.
    dxgi = {"color": 72, "normal": 83, "surface": 71, "height": 56}[kind]
    compressed = kind != "height"
    pitch = ((image.width + 3) // 4 * (16 if kind == "normal" else 8)
             * ((image.height + 3) // 4)) if compressed else image.width * 2
    header = [124, 0x21007 | (0x80000 if compressed else 8),
              image.height, image.width, pitch, 0, len(mips)]
    header += [0] * 11
    header += [32, 4, int.from_bytes(b"DX10", "little"), 0, 0, 0, 0, 0]
    header += [0x401008, 0, 0, 0, 0]
    with path.open("wb") as output:
        output.write(b"DDS " + struct.pack("<31I", *header))
        output.write(struct.pack("<5I", dxgi, 3, 0, 1, 0))
        for mip in mips:
            if compressed:
                encoded = io.BytesIO()
                mip.save(encoded, format="DDS", pixel_format="BC5" if kind == "normal" else "DXT1")
                output.write(encoded.getvalue()[148 if kind == "normal" else 128:])
            else:
                output.write(np.asarray(mip, dtype="<u2").tobytes())
    print(f"{path}: {image.size}, {len(mips)} mips, DXGI {dxgi}, {path.stat().st_size} bytes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--flip-normal-green", action="store_true")
    parser.add_argument("--output", type=Path, default=Path("Content/Textures/BrownRock08"))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    def load(suffix, mode=None):
        with Image.open(args.source / f"BrownRock08_2K_{suffix}.png") as source:
            return source.convert(mode) if mode else source.copy()

    color = load("BaseColor", "RGB")
    normal = load("Normal", "RGB")
    if args.flip_normal_green:
        r, g, b = normal.split()
        normal = Image.merge("RGB", (r, g.point(lambda v: 255 - v), b))
    surface = Image.merge("RGB", (load("AO", "L"), load("Roughness", "L"), load("Metallic", "L")))
    height = load("Height").convert("I")
    if any(image.size != color.size for image in (normal, surface, height)):
        raise ValueError("All material maps must have matching dimensions")
    for suffix, image, kind in (("BaseColor", color, "color"), ("Normal", normal, "normal"),
                                ("OcclusionRoughnessMetallic", surface, "surface"), ("Height", height, "height")):
        write_dds(args.output / f"BrownRock08_2K_{suffix}.dds", image, kind)


if __name__ == "__main__":
    main()
