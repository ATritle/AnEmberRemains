"""Import cleaned/padded character sheets. Original sources stay saved."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
for direction in ('N','NE','E','SE','S','SW','W','NW'):
    name='Mara_'+direction
    task=unreal.AssetImportTask()
    task.filename=str(root/'ArtSource/MaraVey/v1/Clean'/f'{name}.png')
    task.destination_path='/Game/Art/MaraVey'
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset('/Game/Art/MaraVey/'+name)
    assert texture, name
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('filter',unreal.TextureFilter.TF_BILINEAR)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('MARA_IMPORT_COMPLETE: 8 directional sheets, 288 isolated frames')
