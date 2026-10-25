from io_scene_fbx import parse_fbx
from pathlib import Path
import json

root = Path('C:/Users/tarik/Documents/GitHub/AGP/.scratch/splatmap-v2/manual-acceptance-20261024')
source = Path('C:/Users/tarik/Documents/GitHub/AGP-Content/.scratch/splatmap-v2/validation/manual-acceptance-20261024/fresh-fbx-01/Lvl_Splatmap_Testing2.fbx')
reports = []
for path in [source, root/'11-blender-corrected.fbx']:
    data, version = parse_fbx.parse(str(path))
    report={'path':str(path),'version':version,'uv_elements':[],'global_settings':[]}
    def visit(node):
        if node.id in {b'LayerElementUV', b'LayerElementBumpUV'}:
            report['uv_elements'].append({'type':node.id.decode(),'index':list(node.props), 'children':[
                {'id':c.id.decode(), 'props': [list(p[:12]) if hasattr(p,'typecode') else str(p) for p in c.props],
                 'array_lengths':[len(p) for p in c.props if hasattr(p,'typecode')]}
                for c in node.elems]})
        if node.id == b'GlobalSettings':
            report['global_settings']=[str(c.props) for e in node.elems for c in e.elems]
        for child in node.elems: visit(child)
    visit(data)
    reports.append(report)
(root/'16-fbx-uv-structure.json').write_text(json.dumps(reports,indent=2))
print(json.dumps(reports,indent=2))
