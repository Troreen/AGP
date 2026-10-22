"""Run in Unreal Python; export corrected mesh and named paint without saving assets.

Call export_terrain(landscape, corrected_mesh, output_directory, bounds).
The mesh must already have the intended -90 yaw baked by its import pipeline and
corrected source UVs. This tool exports its existing geometry; it does not rotate
it again. bounds is (minX, minY, maxX, maxY) in Unreal centimetres.
"""

import json
from pathlib import Path
import unreal


_pending_exports = []


def export_terrain(landscape, corrected_mesh, output_directory, bounds, resolution=512):
    """Schedule a named-layer bake and export after three editor frame yields."""
    if not isinstance(landscape, unreal.Landscape):
        raise TypeError('Expected a native Landscape actor')
    if not isinstance(corrected_mesh, unreal.StaticMesh):
        raise TypeError('Expected the corrected StaticMesh')
    minimum_x, minimum_y, maximum_x, maximum_y = bounds
    if maximum_x <= minimum_x or maximum_y <= minimum_y or resolution < 2:
        raise ValueError('Invalid mask coverage or resolution')

    output = Path(output_directory).resolve()
    names = ['SM_Landscape_Splatmap.fbx', 'T_Splatmap_LevelWeights.png', 'terrain-export.json']
    if any((output / name).exists() for name in names):
        raise FileExistsError('Use a new output folder; existing exports are preserved')
    output.mkdir(parents=True, exist_ok=True)

    # Bake final named layers, not component-dependent internal texture channels.
    target = unreal.RenderingLibrary.create_render_target2d(
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
        resolution, resolution, unreal.TextureRenderTargetFormat.RTF_RGBA8,
        unreal.LinearColor(0, 0, 0, 0), False, False)
    extent = unreal.Box2D(unreal.Vector2D(minimum_x, minimum_y), unreal.Vector2D(maximum_x, maximum_y))
    layers = ['Snow_Dunes', 'Snow_Rough', 'Snow_Cliff']
    if not landscape.render_weightmaps(unreal.Transform(), extent, layers, target):
        unreal.RenderingLibrary.release_render_target2d(target)
        raise RuntimeError('Landscape weight bake failed')

    state = {'frames': 0, 'handle': None, 'target': target, 'status': 'pending'}
    _pending_exports.append(state)

    def finish_export(delta_seconds):
        state['frames'] += 1
        if state['frames'] < 3:
            return
        unreal.unregister_slate_post_tick_callback(state['handle'])
        try:
            unreal.RenderingLibrary.export_render_target(
                landscape, target, str(output), 'T_Splatmap_LevelWeights.png')
            mask_path = output / names[1]
            if not mask_path.is_file() or mask_path.stat().st_size == 0:
                raise RuntimeError('Weightmap PNG export did not produce a nonempty file')
            options = unreal.FbxExportOption()
            options.ascii = False
            options.collision = False
            options.level_of_detail = False
            options.export_source_mesh = True
            options.vertex_color = True
            # TGAFBX must receive the same axis convention as scene transforms.
            # Default -Y-front export rotates the mesh relative to its actor.
            options.force_front_x_axis = True
            task = unreal.AssetExportTask()
            task.object = corrected_mesh
            task.filename = str(output / names[0])
            task.automated = True
            task.prompt = False
            task.replace_identical = False
            task.selected = False
            task.exporter = unreal.StaticMeshExporterFBX()
            task.options = options
            if not unreal.Exporter.run_asset_export_task(task):
                raise RuntimeError(str(list(task.errors)))
            if not (output / names[0]).is_file() or (output / names[0]).stat().st_size == 0:
                raise RuntimeError('Mesh export did not produce a nonempty FBX')
            metadata = {
                'mesh': corrected_mesh.get_path_name(),
                'landscape': landscape.get_path_name(),
                'layers_rgb': layers,
                'bounds_unreal_xy_cm': list(bounds),
                'resolution': resolution,
                'mask_u': [1 / (maximum_x - minimum_x), 0, 0, -minimum_x / (maximum_x - minimum_x)],
                'mask_v': [0, 1 / (maximum_y - minimum_y), 0, -minimum_y / (maximum_y - minimum_y)],
                'pixel_centres': 'min + (pixel + 0.5) * extent / resolution',
                'save_source_assets': False,
                'force_front_x_axis': True,
                'normal_note': 'Final world-projected shading requires runtime visual verification',
            }
            (output / names[2]).write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
            state['status'] = 'complete'
            unreal.log('Terrain runtime export complete: ' + str(output))
        except Exception as error:
            state['status'] = 'failed'
            state['error'] = str(error)
            raise
        finally:
            unreal.RenderingLibrary.release_render_target2d(target)
            _pending_exports.remove(state)

    state['callback'] = finish_export
    state['handle'] = unreal.register_slate_post_tick_callback(finish_export)
    return state
