"""Build a separate runtime terrain scene; validate imported geometry coverage first."""

import argparse
import copy
import hashlib
import itertools
import json
import math
from pathlib import Path


IDENTITY = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
SOURCE_SCENE = "Content/ExportedScenes/lvl_splatmap_testing/Lvl_Splatmap_Testing_Level.json"
OUTPUT_SCENE = "Content/ExportedScenes/lvl_splatmap_runtime/Lvl_Splatmap_Runtime_Level.json"


def load_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def finite_values(values, count, label):
    if len(values) != count or any(not isinstance(value, (int, float)) or not math.isfinite(value) for value in values):
        raise ValueError(f"{label} requires {count} finite numbers")
    return values


def agp_matrix(exported_matrix):
    # Match UnrealSceneImporter::ReadCommonComponent and actor import exactly.
    finite_values(exported_matrix, 16, "exported transform")
    permutation = [1, 2, 0, 3]
    return [exported_matrix[row * 4 + column] for row in permutation for column in permutation]


def multiply(left, right):
    return [sum(left[row * 4 + inner] * right[inner * 4 + column] for inner in range(4))
            for row in range(4) for column in range(4)]


def transformed_bounds(bounds, transform):
    minimum = finite_values(bounds["min"], 3, "mesh minimum")
    maximum = finite_values(bounds["max"], 3, "mesh maximum")
    if any(low > high for low, high in zip(minimum, maximum)):
        raise ValueError("Mesh minimum exceeds maximum")
    corners = []
    for corner in itertools.product(*zip(minimum, maximum)):
        homogeneous = [*corner, 1]
        corners.append([sum(homogeneous[row] * transform[row * 4 + column] for row in range(4))
                        for column in range(3)])
    return {"min": [min(corner[axis] for corner in corners) for axis in range(3)],
            "max": [max(corner[axis] for corner in corners) for axis in range(3)]}


def material_parameters(args):
    min_x, min_y, max_x, max_y = args.bounds
    width, height = max_x - min_x, max_y - min_y
    parameters = [
        {"Name": "MaskU", "Type": 1, "Value": [1 / width, 0, 0, -min_x / width]},
        {"Name": "MaskV", "Type": 1, "Value": [0, 1 / height, 0, -min_y / height]},
        {"Name": "SurfaceMapping", "Type": 1,
         "Value": [1 / 800, 1 / 800, -args.surface_origin[0] / 800, -args.surface_origin[1] / 800]},
        {"Name": "NoiseControls", "Type": 1, "Value": [args.noise_strength, args.noise_patch_size, 0, 0]},
        {"Name": "DEBUG", "Type": 0, "Value": args.debug_mode},
    ]
    for layer in ("Dunes", "Rough", "Cliff"):
        for role, suffix in (("BaseColor", "BaseColor"), ("ORM", "OcclusionRoughnessMetallic"), ("Normal", "Normal")):
            parameters.append(texture_parameter(layer + role, f"T_8M_Snow_{layer}_{suffix}"))
    parameters.extend([texture_parameter("SplatMask", "T_Splatmap_LevelWeights"),
                       texture_parameter("BlendNoise", "T_Splatmap_TransitionNoise")])
    return parameters


def texture_parameter(role, name):
    # UnrealSceneStructs::MaterialType has Vector3d at 2; Texture is 3.
    return {"Name": role, "Type": 3, "Value": {"Name": name, "AssetImportPath": f"Content/Textures/{name}.dds"}}


def instance(parameters):
    return {"Name": "MI_Landscape_Splatmap_Runtime", "Parent": "MM_Landscape", "Parameters": copy.deepcopy(parameters)}


def overhead_camera():
    # Construct the AGP camera basis directly: local +Z looks toward the origin.
    position = [-3500, 3200, 0]
    distance = math.sqrt(sum(value * value for value in position))
    forward = [-value / distance for value in position]
    right = [0, 0, -1]
    up = [-forward[1], forward[0], 0]
    agp = [*right, 0, *up, 0, *forward, 0, *position, 1]
    # Inverse of the importer permutation restores exported Unreal row-major axes.
    permutation = [2, 0, 1, 3]
    exported = [agp[row * 4 + column] for row in permutation for column in permutation]
    return {"Name": "TerrainVerificationCamera", "Archetype": "CameraActor", "Transform": exported,
            "Tags": ["TerrainVerification"],
            "Components": [{"Name": "CameraComponent0", "Parent": "", "TypeID": 11, "Tags": [],
                            "Transform": IDENTITY.copy(), "FOV": 90}]}


def floater(name, position_x, yaw, plane_bounds, parameters):
    span_x = plane_bounds["max"][0] - plane_bounds["min"][0]
    span_z = plane_bounds["max"][2] - plane_bounds["min"][2]
    if span_x <= 0 or span_z <= 0 or abs(plane_bounds["max"][1] - plane_bounds["min"][1]) > 0.01:
        raise ValueError("SM_Plane must be a horizontal imported AGP XZ plane")
    # Build exported Unreal XY yaw/scale; imported AGP ground axes are ZX.
    scale_x, scale_y = 800 / span_z, 800 / span_x
    cosine, sine = math.cos(yaw), math.sin(yaw)
    transform = [scale_x * cosine, scale_x * sine, 0, 0,
                 -scale_y * sine, scale_y * cosine, 0, 0,
                 0, 0, 1, 0, position_x, 0, 80, 1]
    return {"Name": name, "Archetype": "StaticMeshActor", "Transform": transform, "Tags": ["TerrainVerification"],
            "Components": [{"Name": "StaticMeshComponent0", "Parent": "", "TypeID": 1, "Tags": [],
                            "Transform": IDENTITY.copy(), "Mesh": "SM_Plane",
                            "ContentPath": "/Game/Meshes/Debug/SM_Plane.SM_Plane",
                            "AssetImportPath": "Content/Meshes/Debug/SM_Plane.fbx", "Materials": [instance(parameters)]}]}


def validate_coverage(scene, mesh_bounds, mask_bounds):
    results = []
    for actor in scene["Actors"]:
        for component in actor["Components"]:
            if component["TypeID"] != 1:
                continue
            mesh = component["Mesh"]
            if mesh not in mesh_bounds:
                raise ValueError(f"Missing imported geometry bounds for {mesh}")
            # The engine uses component-local * actor-world. Parent is source metadata.
            transform = multiply(agp_matrix(component["Transform"]), agp_matrix(actor["Transform"]))
            world = transformed_bounds(mesh_bounds[mesh], transform)
            min_x, min_y, max_x, max_y = mask_bounds
            # AGP Z/X are Unreal X/Y: mask coverage is independent of terrain height.
            covered = (world["min"][2] >= min_x - 0.001 and world["max"][2] <= max_x + 0.001
                       and world["min"][0] >= min_y - 0.001 and world["max"][0] <= max_y + 0.001)
            if not covered:
                raise ValueError(f"{actor['Name']}/{component['Name']} geometry exceeds splat mask coverage: {world}")
            results.append({"actor": actor["Name"], "mesh": mesh, "world_agp_bounds": world, "covered": True})
    if not results:
        raise ValueError("No static mesh geometry was checked")
    return results


def build(args):
    finite_values(args.bounds, 4, "mask bounds")
    finite_values(args.surface_origin, 2, "surface origin")
    if args.bounds[0] >= args.bounds[2] or args.bounds[1] >= args.bounds[3]:
        raise ValueError("Mask maximum must exceed minimum")
    if not math.isfinite(args.noise_strength) or not 0 <= args.noise_strength <= 1:
        raise ValueError("Noise strength must be between zero and one")
    if not math.isfinite(args.noise_patch_size) or args.noise_patch_size < 1:
        raise ValueError("Noise patch size must be at least one centimetre")
    source = args.source or args.content_root / SOURCE_SCENE
    output = args.output or args.content_root / OUTPUT_SCENE
    provenance_path = output.with_suffix(".provenance.json")
    if source.resolve() == output.resolve():
        raise ValueError("Output must not replace the source scene")
    if not args.force and (output.exists() or provenance_path.exists()):
        raise ValueError("Output exists; review it and use --force to replace generated outputs")
    bounds_document = load_json(args.geometry_bounds_json)
    if bounds_document.get("coordinate_system") != "AGP":
        raise ValueError("Geometry bounds must use imported local AGP XYZ coordinates")
    geometry = bounds_document["meshes"]
    scene = load_json(source)
    parameters = material_parameters(args)
    masters = [material for material in scene["MasterMaterials"] if material["Name"] == "MM_Landscape"]
    if len(masters) != 1:
        raise ValueError("Source must contain exactly one MM_Landscape master")
    masters[0]["Parameters"] = copy.deepcopy(parameters)
    replacements = 0
    for actor in scene["Actors"]:
        for component in actor["Components"]:
            if component.get("Mesh") == "SM_Landscape_Test":
                actor["Name"] = "SM_Landscape_Splatmap"
                component.update({"Mesh": "SM_Landscape_Splatmap",
                                  "ContentPath": "/Game/Meshes/Landscapes/Splatmap/SM_Landscape_Splatmap.SM_Landscape_Splatmap",
                                  "AssetImportPath": "Content/Meshes/Landscapes/Splatmap/SM_Landscape_Splatmap.fbx",
                                  "Materials": [instance(parameters)]})
                replacements += 1
    if replacements != 1:
        raise ValueError("Source must contain exactly one SM_Landscape_Test component")
    scene["Actors"].extend([floater("GroundFloater_Aligned", -1000, 0, geometry["SM_Plane"], parameters),
                            floater("GroundFloater_Rotated45", 1000, math.pi / 4, geometry["SM_Plane"], parameters),
                            overhead_camera()])
    coverage = validate_coverage(scene, geometry, args.bounds)
    provenance = {"source_scene": SOURCE_SCENE if args.source is None else source.name,
                  "source_scene_sha256": checksum(source),
                  "geometry_bounds_source": args.geometry_bounds_json.name,
                  "geometry_bounds_sha256": checksum(args.geometry_bounds_json),
                  "coordinate_mapping": "Unreal XY = AGP ZX; centimetres; row-vector component * actor transforms",
                  "mask_bounds_unreal_xy": args.bounds, "surface_origin_unreal_xy": args.surface_origin,
                  "surface_repeat_cm": 800, "noise_strength": args.noise_strength,
                  "noise_patch_size_cm": args.noise_patch_size, "debug_mode": args.debug_mode,
                  "geometry_coverage": coverage,
                  "camera": {"unreal_position": [0, -3500, 3200], "target_unreal": [0, 0, 0], "horizontal_fov": 90},
                  "limitations": ["AABB coverage verifies bounds, not mask image registration or visible appearance.",
                                  "Floaters are elevated inspection fixtures; matching terrain height remains a visual check."]}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(scene, indent="\t") + "\n", encoding="utf-8")
    provenance_path.write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared {output.name}; validated {len(coverage)} geometry bounds against the level mask")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--content-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--geometry-bounds-json", type=Path, required=True)
    parser.add_argument("--bounds", nargs=4, type=float, default=[-2800, -2800, 2800, 2800], metavar=("MINX", "MINY", "MAXX", "MAXY"))
    parser.add_argument("--surface-origin", nargs=2, type=float, default=[-2800, -2800], metavar=("X", "Y"))
    parser.add_argument("--noise-strength", type=float, default=0)
    parser.add_argument("--noise-patch-size", type=float, default=200)
    parser.add_argument("--debug-mode", type=int, choices=range(5), default=0)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()
    try:
        build(args)
    except (ValueError, KeyError, OSError) as error:
        parser.exit(1, f"Terrain scene preparation failed: {error}\n")


if __name__ == "__main__":
    main()
