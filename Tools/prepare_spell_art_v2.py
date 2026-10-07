from pathlib import Path
import json,re
import numpy as np
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
out=root/'ArtSource/Spells/v2'
out.mkdir(parents=True,exist_ok=True)
for name,n in [('Flame',4),('Ice',2)]:
    im=Image.open(out/f'{name}-source.png').convert('RGBA')
    cell=512;atlas=Image.new('RGBA',(n*cell,n*cell))
    for y in range(n):
        for x in range(n):
            part=im.crop((round(x*im.width/n),round(y*im.height/n),round((x+1)*im.width/n),round((y+1)*im.height/n)))
            a=np.array(part)
            if name=='Flame':
                # Feather cell edges before padding: no neighboring frame bleed.
                yy,xx=np.mgrid[:a.shape[0],:a.shape[1]]
                edge=np.minimum.reduce([xx,yy,a.shape[1]-1-xx,a.shape[0]-1-yy])
                a[:,:,3]=(a[:,:,3]*np.clip(edge/7,0,1)).astype('uint8')
                part=Image.fromarray(a).resize((440,440),Image.Resampling.LANCZOS)
                atlas.alpha_composite(part,(x*cell+36,y*cell+36))
            else:
                # Preserve opaque fractured ice; reject low-alpha background debris.
                a[:,:,3]=np.where(a[:,:,3]>40,a[:,:,3],0)
                part=Image.fromarray(a);box=part.getbbox();assert box
                part=part.crop(box);s=min(440/part.width,444/part.height)
                part=part.resize((round(part.width*s),round(part.height*s)),Image.Resampling.LANCZOS)
                atlas.alpha_composite(part,(x*cell+(cell-part.width)//2,y*cell+480-part.height))
    atlas.save(out/f'{name}Atlas.png')
    bg=Image.new('RGBA',atlas.size,(28,32,40,255));bg.alpha_composite(atlas);bg.convert('RGB').save(out/f'{name}-review.jpg')
# Extract staff crystal centers from the same measured crops used by the renderer.
header=(root/'Source/AnEmberRemains/MaraArtMetrics.h').read_text()
scales=[float(x) for x in re.search(r'Scale\[\]\s*=\s*\{([^}]+)',header)[1].replace('f','').split(',') if x.strip()]
rows=re.findall(r'\{([\d.]+)f?,([\d.]+)f?,([\d.]+)f?,([\d.]+)f?,([\d.]+)f?,([\d.]+)f?\}',header)
assert len(rows)==288,len(rows)
tips=[];contact=Image.new('RGB',(6*180,8*170),(24,29,34));draw=ImageDraw.Draw(contact)
for d,direction in enumerate(('N','NE','E','SE','S','SW','W','NW')):
    sheet=Image.open(root/f'ArtSource/MaraVey/v1/Clean/Mara_{direction}.png').convert('RGBA');dtips=[]
    for f in range(6):
        x,y,w,h,px,py=map(float,rows[d*36+30+f]);crop=sheet.crop((int(x),int(y),int(x+w),int(y+h)));a=np.array(crop)
        rgb=a[:,:,:3].astype(float)
        mask=(a[:,:,3]>128)&(rgb[:,:,1]>rgb[:,:,0]*1.35)&(rgb[:,:,2]>rgb[:,:,0]*1.25)&(rgb[:,:,1]>70)
        yy,xx=np.where(mask);assert len(xx)>3,(direction,f)
        tx,ty=float(np.median(xx)),float(np.median(yy));kx=scales[d]*.92;ky=scales[d]*1.18
        dtips.append([round((tx-px)*kx,3),round((ty-py)*ky,3)])
        resized=crop.resize((round(w*kx),round(h*ky)),Image.Resampling.LANCZOS)
        dest=(round(f*180+90-px*kx),round(d*170+155-py*ky));contact.paste(resized,dest,resized)
        qx=f*180+90+dtips[-1][0];qy=d*170+155+dtips[-1][1];draw.ellipse((qx-3,qy-3,qx+3,qy+3),outline=(255,40,255),width=1)
    tips.append(dtips)
contact.save(out/'Staff-anchor-review.jpg')
text='#pragma once\nnamespace MaraCast { static constexpr float Tips[8][6][2]={\n'
text+=',\n'.join('{'+','.join('{'+f'{x}f,{y}f'+'}' for x,y in row)+'}' for row in tips)+'\n}; }\n'
(root/'Source/AnEmberRemains/MaraCastMetrics.h').write_text(text)
(out/'staff-anchors.json').write_text(json.dumps(tips,indent=2))
print('Prepared flame/ice atlases and 48 cast crystal anchors')
