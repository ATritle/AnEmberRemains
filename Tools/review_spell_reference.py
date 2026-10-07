import bpy
import sys
from pathlib import Path
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
if not args:
    raise SystemExit('Usage: blender -b --python Tools/review_spell_reference.py -- <reference-video>')
root=Path(__file__).resolve().parents[1]/'Saved/SpellReference-v2'
root.mkdir(parents=True,exist_ok=True)
scene=bpy.context.scene
seq=scene.sequence_editor_create()
clip=seq.strips.new_movie('Reference',str(Path(args[0]).resolve()),channel=1,frame_start=1)
scene.render.resolution_x=960
scene.render.resolution_y=540
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
print('REFERENCE_FRAMES',clip.frame_final_duration,'FPS',clip.fps)
for i in range(8):
    scene.frame_set(1+int((clip.frame_final_duration-2)*(i+.5)/8))
    scene.render.filepath=str(root/f'reference-{i}.png')
    bpy.ops.render.render(write_still=True)
