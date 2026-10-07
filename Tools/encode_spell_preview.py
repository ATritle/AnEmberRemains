"""Encode actual packaged-game screenshots, not an effects mockup."""
import bpy
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
folder=args[0] if args else 'SpellCaptureFinal'
name=args[1] if len(args)>1 else 'Spell-Preview.mp4'
frames=sorted((root/f'Saved/{folder}/Saved/SpellPreviewFrames').glob('Frame*.png'))
assert len(frames)>100,len(frames)
scene=bpy.context.scene
scene.render.resolution_x=1280
scene.render.resolution_y=800
scene.render.resolution_percentage=100
scene.render.fps=15
seq=scene.sequence_editor_create()
strip=seq.strips.new_image('Packaged gameplay',str(frames[0]),channel=1,frame_start=1)
for f in frames[1:]:strip.elements.append(f.name)
scene.frame_start=1
scene.frame_end=len(frames)
scene.render.image_settings.media_type='VIDEO'
scene.render.image_settings.file_format='FFMPEG'
scene.render.ffmpeg.format='MPEG4'
scene.render.ffmpeg.codec='H264'
scene.render.ffmpeg.constant_rate_factor='MEDIUM'
scene.render.ffmpeg.ffmpeg_preset='GOOD'
scene.view_settings.view_transform='Standard'
scene.render.filepath=str(root/'Builds/HushedCloisters'/name)
bpy.ops.render.render(animation=True)
