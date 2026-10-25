"""Compare actual raw-mask renders where unrotated/rotated floater tops overlap."""
import json
from pathlib import Path

root = Path(__file__).parent


def read(name):
    magic, dimensions, maximum, data = (root / 'render' / name).read_bytes().split(b'\n', 3)
    width, height = map(int, dimensions.split())
    assert magic == b'P6' and maximum == b'255' and len(data) == width * height * 3
    return width, height, data


width, height, first = read('painted-0.ppm')
other_width, other_height, rotated = read('painted-35.ppm')
assert (width, height) == (other_width, other_height)
common = stable = 0
for index in range(0, len(first), 3):
    a, b = first[index:index + 3], rotated[index:index + 3]
    if max(a) > 5 and max(b) > 5:
        common += 1
        stable += all(abs(x - y) <= 1 for x, y in zip(a, b))
report = {'common_floater_pixels': common, 'same_mask_rgb_within_one_byte': stable,
          'stable_fraction': stable / common if common else 0,
          'note': 'Overlap of actual isolated elevated cube renders; silhouette/side edges can differ.'}
(root / '13-rotation-registration.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
assert common > 100 and report['stable_fraction'] > 0.95, 'Mask moved with mesh rotation'
