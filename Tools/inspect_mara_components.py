from pathlib import Path
import numpy as np
from PIL import Image

def components(alpha, return_labels=False):
    mask=alpha>32
    h,w=mask.shape
    result=[]
    labels=np.zeros((h,w),dtype=np.int32);next_id=0
    for y in range(h):
        for x in np.flatnonzero(mask[y]):
            if not mask[y,x]: continue
            stack=[(int(x),y)];mask[y,x]=False
            next_id+=1
            x0=x1=int(x);y0=y1=y;count=0
            while stack:
                px,py=stack.pop();count+=1
                labels[py,px]=next_id
                x0=min(x0,px);x1=max(x1,px);y0=min(y0,py);y1=max(y1,py)
                for nx,ny in ((px-1,py),(px+1,py),(px,py-1),(px,py+1)):
                    if 0<=nx<w and 0<=ny<h and mask[ny,nx]:
                        mask[ny,nx]=False;stack.append((nx,ny))
            if count>1000:result.append((count,x0,y0,x1+1,y1+1)+((next_id,) if return_labels else ()))
    return (result,labels) if return_labels else result

if __name__=='__main__':
    for p in (Path(__file__).resolve().parents[1]/'ArtSource/MaraVey/v1').glob('Mara_*.png'):
        found=components(np.array(Image.open(p))[:,:,3])
        print(p.name,len(found),sorted(found,key=lambda b:b[2]))
