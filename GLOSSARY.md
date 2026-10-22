# AGP domain language

Terms used when discussing authored terrain surfaces and their runtime rendering.

## Terrain surfaces

**Surface layer**:
A terrain surface appearance, such as Snow Dunes, Snow Rough or Snow Cliff,
with its base color, surface properties and normal detail.

**Painted layer weight**:
The authored strength of one surface layer at a position on the terrain.
Several layers can have nonzero weights at the same position.

**Splat mask**:
An image containing layer weights used to distribute surface layers over terrain.
Its channels represent weights rather than the final visible surface color.
_Avoid_: terrain heightmap

**Ground floater**:
A separate ground-detail mesh intended to share the terrain's material
distribution and texture placement where their surfaces overlap.
