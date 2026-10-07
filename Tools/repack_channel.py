"""Isolate measured connected figures, discard haze, align planted feet and staff tips."""
from pathlib import Path
import json
import numpy as np
from PIL import Image
from inspect_mara_components import components
root=Path(__file__).resolve().parents[1]
folder=root/'ArtSource/MaraVey/Channel-v1'
folder.mkdir(parents=True,exist_ok=True)
source=Image.open(folder/'source.png').convert('RGBA')
a=np.array(source)
# Only the translucent generated halo is rejected; retain RGB of every silhouette pixel.
found,labels=components(np.where(a[:,:,3]>128,255,0).astype('uint8'),True)
assert len(found)==24, len(found)
found.sort(key=lambda b:(b[2]+b[4])/2)
sheet=Image.new('RGBA',(768,2048))
metrics=[]
for d in range(8):
    row=sorted(found[d*3:d*3+3],key=lambda b:b[1])
    heights=[]
    for b in row:
        mask=labels==b[-1]
        hair=mask&(a[:,:,0]>a[:,:,1]*1.5)&(a[:,:,0]>90)
        yy=np.where(hair)[0]
        heights.append(b[4]-int(yy.min()) if len(yy) else b[4]-b[2])
    scale=120/np.median(heights)
    rowmetrics=[]
    for f,b in enumerate(row):
        _,x0,y0,x1,y1,label=b
        owner=labels==label
        # One-pixel padding preserves antialiasing, without reconnecting the haze.
        pad=np.pad(owner,1)
        dilated=np.logical_or.reduce([pad[dy:dy+a.shape[0],dx:dx+a.shape[1]] for dy,dx in [(0,1),(1,0),(1,1),(1,2),(2,1)]])
        x0=max(0,x0-1);y0=max(0,y0-1);x1=min(a.shape[1],x1+1);y1=min(a.shape[0],y1+1)
        part=a[y0:y1,x0:x1].copy()
        part[:,:,3]=np.where(dilated[y0:y1,x0:x1],part[:,:,3],0)
        # Feet pivot remains on a common baseline; fixed per-row horizontal pivot.
        mask=owner[y0:y1,x0:x1]
        feet=np.where(mask[-18:])[1]
        px=float(np.median(feet)) if len(feet) else (x1-x0)/2
        py=y1-y0-2
        teal=mask&(part[:,:,1]>part[:,:,0]*1.35)&(part[:,:,2]>part[:,:,0]*1.3)&(part[:,:,1]>70)
        ty,tx=np.where(teal)
        assert len(tx)>3,(d,f)
        tip=(float(np.median(tx))-px,float(np.median(ty))-py)
        sprite=Image.fromarray(part).resize((round(part.shape[1]*scale),round(part.shape[0]*scale)),Image.Resampling.LANCZOS)
        dest=(f*256+128-round(px*scale),d*256+225-round(py*scale))
        assert dest[0]>=f*256+4 and dest[1]>=d*256+4
        assert dest[0]+sprite.width<=(f+1)*256-4 and dest[1]+sprite.height<=(d+1)*256-4
        sheet.alpha_composite(sprite,dest)
        rowmetrics.append([round(tip[0]*scale,3),round(tip[1]*scale,3)])
    metrics.append(rowmetrics)
sheet.save(folder/'Mara_Channel.png')
(folder/'anchors.json').write_text(json.dumps(metrics,indent=2))
header='#pragma once\nnamespace MaraChannel { static constexpr float Tips[8][3][2]={\n'
header+=',\n'.join('{'+','.join('{'+f'{x}f,{y}f'+'}' for x,y in row)+'}' for row in metrics)
header+='\n}; }\n'
(root/'Source/AnEmberRemains/MaraChannelMetrics.h').write_text(header)
preview=Image.new('RGBA',sheet.size,(24,29,34,255));preview.alpha_composite(sheet);preview.convert('RGB').save(folder/'review.png')
print('CHANNEL: 24 isolated poses; planted foot pivots; measured crystal anchors; 768x2048')
