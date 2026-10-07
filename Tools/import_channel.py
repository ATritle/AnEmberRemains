import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
task=unreal.AssetImportTask()
task.filename=str(root/'ArtSource/MaraVey/Channel-v1/Mara_Channel.png')
task.destination_path='/Game/Art/MaraVey'
task.destination_name='Mara_Channel'
task.automated=True
task.replace_existing=True
task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
t=unreal.load_asset('/Game/Art/MaraVey/Mara_Channel')
assert t
t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
t.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
t.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
t.set_editor_property('filter',unreal.TextureFilter.TF_BILINEAR)
unreal.EditorAssetLibrary.save_loaded_asset(t)
