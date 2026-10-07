import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
for name in ('FlameAtlas','IceAtlas'):
    task=unreal.AssetImportTask()
    task.filename=str(root/f'ArtSource/Spells/v2/{name}.png')
    task.destination_path='/Game/Effects/Spells'
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    t=unreal.load_asset('/Game/Effects/Spells/'+name)
    assert t
    t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    t.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    t.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    t.set_editor_property('filter',unreal.TextureFilter.TF_BILINEAR)
    unreal.EditorAssetLibrary.save_loaded_asset(t)
