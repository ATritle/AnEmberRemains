"""Technical size/format exports of imagegen masters; no repainting or compositing."""
from pathlib import Path
import shutil
from PIL import Image

root = Path(__file__).resolve().parents[1]
art = root / 'ArtSource/Branding/v1'
out = root / 'Media/Branding'
out.mkdir(parents=True, exist_ok=True)
title = Image.open(art / 'title-master.png').convert('RGB')
for name, size in [('title-1920x1080.jpg', (1920,1080)), ('thumbnail-1280x720.jpg',(1280,720)), ('thumbnail-640x360.jpg',(640,360))]:
    title.resize(size, Image.Resampling.LANCZOS).save(out/name, quality=94)
shutil.copy2(art/'logo-master.png', out/'title-logo.png')
icon = Image.open(art/'icon-master.png').convert('RGBA')
assert icon.getchannel('A').getextrema()[0] == 0, 'Icon needs transparent exterior'
assert Image.open(out/'title-logo.png').convert('RGBA').getchannel('A').getextrema()[0] == 0
for size in (16,24,32,48,64,128,256,512):
    icon.resize((size,size),Image.Resampling.LANCZOS).save(out/f'icon-{size}.png')
sizes = [(s,s) for s in (16,24,32,48,64,128,256)]
icon.save(out/'AnEmberRemains.ico',sizes=sizes)
ico = Image.open(out/'AnEmberRemains.ico')
assert ico.ico.sizes() == set(sizes)
windows = root/'Build/Windows'
windows.mkdir(parents=True, exist_ok=True)
shutil.copy2(out/'AnEmberRemains.ico', windows/'Application.ico')
# UE project thumbnail is a conventional static asset, not a runtime texture.
title.resize((192,108), Image.Resampling.LANCZOS).save(root/'AnEmberRemains.png')
print('Exported branding PNG/JPG sizes, transparent wordmark, project thumbnail and 7-size Windows ICO.')
