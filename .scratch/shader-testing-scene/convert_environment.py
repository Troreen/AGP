"""Build the shader scene's ordinary-mip HDR cubemap, without preparing IBL.

Run from the repository root after `python -m pip install numpy OpenEXR`:
    python .scratch/shader-testing-scene/convert_environment.py

The existing .hdr file contains OpenEXR data (identified by its magic bytes).
We decode its linear float RGB without exposure, tonemapping, or sRGB conversion.
"""

import argparse
import math
from pathlib import Path
import struct
import zlib

import numpy as np
import OpenEXR


def sample_panorama(panorama, direction):
    """Map +Y to north and +Z to the panorama center; wrap the longitude seam."""
    direction = direction / np.linalg.norm(direction, axis=-1, keepdims=True)
    height, width = panorama.shape[:2]
    longitude = np.arctan2(direction[..., 0], direction[..., 2])
    latitude = np.arccos(np.clip(direction[..., 1], -1.0, 1.0))
    x = (longitude / (2 * math.pi) + 0.5) * width - 0.5
    y = np.clip(latitude / math.pi * height - 0.5, 0, height - 1)
    x0 = np.floor(x).astype(np.int32)
    y0 = np.floor(y).astype(np.int32)
    blend_x = (x - x0)[..., None]
    blend_y = (y - y0)[..., None]
    top = panorama[y0, x0 % width] * (1 - blend_x)
    top += panorama[y0, (x0 + 1) % width] * blend_x
    bottom = panorama[np.minimum(y0 + 1, height - 1), x0 % width] * (1 - blend_x)
    bottom += panorama[np.minimum(y0 + 1, height - 1), (x0 + 1) % width] * blend_x
    return (top * (1 - blend_y) + bottom * blend_y).astype(np.float32)


def cube_faces(panorama, size):
    coordinate = (np.arange(size, dtype=np.float32) + 0.5) / size * 2 - 1
    u, v = np.meshgrid(coordinate, coordinate)
    one = np.ones_like(u)
    # D3D face order and face orientation: +X, -X, +Y, -Y, +Z, -Z.
    # v increases down each stored image. Both skybox and material reflections
    # sample this same world-space convention, so they remain aligned.
    directions = (
        (one, -v, -u), (-one, -v, u), (u, one, v),
        (u, -one, -v), (u, -v, one), (-u, -v, -one),
    )
    return [sample_panorama(panorama, np.stack(face, axis=-1)) for face in directions]


def write_dds(path, faces):
    size = faces[0].shape[0]
    mip_count = size.bit_length()
    # DDS_HEADER + DDS_HEADER_DXT10, R16G16B16A16_FLOAT (DXGI_FORMAT 10).
    # DDSD_CAPS|HEIGHT|WIDTH|PITCH|PIXELFORMAT|MIPMAPCOUNT, cubemap all faces.
    header = [124, 0x2100F, size, size, size * 8, 0, mip_count]
    header += [0] * 11
    header += [32, 4, int.from_bytes(b"DX10", "little"), 0, 0, 0, 0, 0]
    header += [0x401008, 0xFE00, 0, 0, 0]
    with path.open("wb") as output:
        output.write(b"DDS " + struct.pack("<31I", *header))
        output.write(struct.pack("<5I", 10, 3, 4, 1, 0))
        for face in faces:
            mip = np.concatenate((face, np.ones((*face.shape[:2], 1), np.float32)), axis=-1)
            for level in range(mip_count):
                output.write(mip.astype("<f2").tobytes())
                if level + 1 < mip_count:
                    height, width = mip.shape[:2]
                    # Ordinary box-filtered mips, not roughness-prefiltered IBL.
                    mip = mip.reshape(height // 2, 2, width // 2, 2, 4).mean(axis=(1, 3))


def write_preview(path, faces):
    # A labeled face order is documented in the README. This is a contact sheet,
    # tonemapped only for inspection; none of this processing enters the DDS.
    sheet = np.concatenate(faces, axis=1)
    sheet = sheet / (1 + sheet)
    sheet = np.power(np.clip(sheet, 0, 1), 1 / 2.2)
    sheet = (sheet * 255 + 0.5).astype(np.uint8)
    height, width = sheet.shape[:2]

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    rows = b"".join(b"\x00" + row.tobytes() for row in sheet)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">2I5B", width, height, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path("Content/Textures/snowy_hillside_02_4k.hdr"))
    parser.add_argument("--output", type=Path, default=Path("Content/Textures/snowy_hillside_02_cube.dds"))
    parser.add_argument("--size", type=int, default=512)
    args = parser.parse_args()
    if args.size < 1 or args.size & (args.size - 1):
        parser.error("--size must be a positive power of two")
    with OpenEXR.File(str(args.source)) as source:
        channels = source.channels()
        packed = channels.get("RGBA", channels.get("RGB"))
        panorama = packed.pixels[..., :3].copy()
    if not np.isfinite(panorama).all() or panorama.min() < 0:
        raise ValueError("Expected finite, nonnegative linear environment radiance")
    if panorama.max() > np.finfo(np.float16).max:
        raise ValueError("Source exceeds the float16 DDS range")
    faces = cube_faces(panorama, args.size)
    write_dds(args.output, faces)
    preview = Path(__file__).with_name("environment-faces.png")
    write_preview(preview, faces)
    print(f"Source: {panorama.shape[1]}x{panorama.shape[0]}, RGB range {panorama.min():.6g}..{panorama.max():.6g}")
    print(f"DDS: {args.output}, {args.size}x{args.size}, 6 faces, {args.size.bit_length()} mips, R16G16B16A16_FLOAT")
    print(f"Cubemap RGB range {min(face.min() for face in faces):.6g}..{max(face.max() for face in faces):.6g}")
    print(f"Bytes: {args.output.stat().st_size}; preview: {preview}")


if __name__ == "__main__":
    main()
