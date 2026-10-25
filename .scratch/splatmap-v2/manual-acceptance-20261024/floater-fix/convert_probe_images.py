"""Losslessly wrap actual renderer P6 RGB bytes in PNG for visual inspection."""
from pathlib import Path
import struct
import zlib


def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))


for source in (Path(__file__).parent / 'render').glob('*.ppm'):
    magic, dimensions, maximum, pixels = source.read_bytes().split(b'\n', 3)
    width, height = map(int, dimensions.split())
    assert magic == b'P6' and maximum == b'255' and len(pixels) == width * height * 3
    rows = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    result = b'\x89PNG\r\n\x1a\n'
    result += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    result += chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
    source.with_suffix('.png').write_bytes(result)
    print(source.name, width, height)
