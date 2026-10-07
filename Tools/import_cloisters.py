"""Import original generated images without resampling, cropping or alpha edits."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir())
for name in ('Floor','Wall','Props','Arch','Life','Rats','WallLife','Notes','Stairs'):
    task=unreal.AssetImportTask()
    task.filename=str(root/'ArtSource'/'Cloisters'/f'{name}.png')
    task.destination_path='/Game/Art/Cloisters'
    task.destination_name='Cloister'+name
    task.automated=True
    task.replace_existing=True
    task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(f'/Game/Art/Cloisters/Cloister{name}')
    assert texture, name
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('filter',unreal.TextureFilter.TF_BILINEAR)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('CLOISTER_IMPORT_COMPLETE: 9 original textures, alpha preserved')
