import bpy,json
from pathlib import Path
root=Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
rows=[]
for filename in ['10-source-inspection.blend','17-reimport-inspection.blend']:
    bpy.ops.wm.open_mainfile(filepath=str(root/filename))
    obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    mesh=obj.data
    lengths=[n.vector.length for n in mesh.corner_normals]
    dots=[mesh.polygons[loop.polygon_index].normal.dot(mesh.corner_normals[loop.index].vector)
          for loop in []]  # Polygon loop ownership is recorded explicitly below.
    dots=[]
    for polygon in mesh.polygons:
        dots.extend(polygon.normal.dot(mesh.corner_normals[i].vector) for i in polygon.loop_indices)
    unit=bpy.context.scene.unit_settings.scale_length
    positions=[obj.matrix_world@v.co*unit for v in mesh.vertices]
    rows.append({'file':filename,'vertices':len(mesh.vertices),'triangles':len(mesh.polygons),
                 'uv_layers':[l.name for l in mesh.uv_layers],
                 'normal_length_min':min(lengths),'normal_length_max':max(lengths),
                 'face_corner_dot_min':min(dots),'face_corner_dot_mean':sum(dots)/len(dots),
                 'opposed_corner_normals':sum(v<0 for v in dots),
                 'physical_bounds_m':[[min(p[a] for p in positions) for a in range(3)],
                                      [max(p[a] for p in positions) for a in range(3)]]})
(root/'26-blender-surface-qa.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows,indent=2))
