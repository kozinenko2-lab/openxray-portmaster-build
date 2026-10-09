#!/usr/bin/env python3
"""Generate the distributable, original-file-free AirXonix replacement pack.

All pixels, PCM, SFX and level records emitted here are synthesized by this
script.  No bytes are copied from AirXonix.wrp.exe, BMPPACK, MUSIC/*.mus or
SOUNDINF.  The filenames/dimensions follow the native engine's compatibility
contracts so the same renderer/gameplay code can consume either resource set.
"""
from __future__ import annotations
import argparse, math, os, struct, hashlib, zipfile
from pathlib import Path
from typing import Dict, Tuple, List
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

SR=22050
PACK_VERSION="r208"
COUNTS=[7,15,20,20,20]
STARTS=[0,7,22,42,62]
MODE_NAMES=["ПРОСТАЯ","КЛАССИКА","МОДЕРН","ХАРД","ЭКСТРИМ"]
WAVEPACK=[
("haha",46169),("bon3",23136),("min0",2560),("fir1",11482),("up01",15799),
("levl",53263),("bon1",18082),("comp",75595),("bvzr",10827),("fire",21222),
("game",53235),("bol2",1988),("bon0",12003),("bon2",22426),("bol1",654),
("clc1",1352),("clc2",1480),("bfly",34080),("vint",40855),("sfly",5543),
("gove",17795),("efly",15175),("bpop",8128),("fir2",29076),("tick",20047),
("bonu",14577),("cow2",11593),("cow4",11977),("cow1",6062),("beep",3026),
("cow3",12965),("time",13456),("bour",1843),("welk",17868),("slow",16315),
("life",12586),("strt",17299),("byeb",13844),("acce",20275),("lets",22287),
("aaaa",19334),("that",23675),("out!",13238),("ohoh",11891),("oyoy",4278),
("cool",9882),("sur1",22646),("whip",12512),("cmex",22303),("yes1",13517)]

SPECIAL_DIMS={
"BALL":(32,32),"XONI":(32,32),"SPEE":(32,32),"MONY":(32,32),"HEAR":(32,32),"CLCK":(32,32),
"CNT2":(192,24),"CNT3":(256,36),"LEVL":(64,24),"SCOR":(24,24),"PERC":(24,24),"PAUS":(64,24),
"XON1":(256,32),"VZRV":(16,16),"VZR1":(64,64),"SHAD":(16,16),"GOVE":(256,48),"COMP":(256,48),
"cmp2":(256,48),"ABOR":(256,32),"GAME":(128,48),"gam2":(128,48),"RAM3":(128,92),
"IN2$":(128,48),"IN2T":(128,48),"IN2L":(128,48),"IN2S":(128,48),"IN2A":(128,48),
"TOU2":(256,48),"LEV2":(256,64),"1111":(128,128),"LOGO":(256,256),"fnt4":(256,256),
"on++":(64,38),"off+":(64,38),"TEMP":(256,4),"LAXY":(64,64),"AAAA":(64,64),"RRRR":(64,64),
"M101":(256,48),"M102":(256,48),"M103":(256,48),"M104":(256,48),"M106":(256,48),
"M250":(256,42),"M260":(256,42),"M240":(256,42),"M210":(256,42),"M220":(256,42),"M230":(256,42),
}
THEME_NAMES={"0034","TST1","0057","VOL3","TST2","0212","SKY7","TST3","0007","VOL9","TST4","0214",
"SKY3","TST8","0040","VOL2","0205","0117","SK02","0031","VOL0","0049","SK01","0052","VOL4","0106","0217"}
TEXTURE_NAMES=sorted(set(SPECIAL_DIMS)|THEME_NAMES)

FONT_CANDIDATES=[
"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
"/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
"/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf",
]
def font(size:int):
    for p in FONT_CANDIDATES:
        if os.path.isfile(p): return ImageFont.truetype(p,size=size)
    return ImageFont.load_default()

def seed(name:str)->int:
    return int.from_bytes(hashlib.sha256(name.encode()).digest()[:4],"little")

def dims(name:str)->Tuple[int,int]: return SPECIAL_DIMS.get(name,(64,64))

def transparent(w,h): return Image.new("RGBA",(w,h),(0,0,0,0))

def fit_font(text:str, box:Tuple[int,int], max_size:int=28):
    w,h=box
    for sz in range(max_size,7,-1):
        f=font(sz); bb=f.getbbox(text)
        if bb[2]-bb[0] <= w-8 and bb[3]-bb[1] <= h-4:return f
    return font(8)

def centered_text(im:Image.Image,text:str,fill=(235,245,255,255),stroke=(0,15,35,255),max_size=28):
    d=ImageDraw.Draw(im); f=fit_font(text,im.size,max_size)
    bb=d.textbbox((0,0),text,font=f,stroke_width=1)
    x=(im.width-(bb[2]-bb[0]))//2-bb[0]; y=(im.height-(bb[3]-bb[1]))//2-bb[1]
    d.text((x,y),text,font=f,fill=fill,stroke_width=1,stroke_fill=stroke)

def radial_icon(name:str,w:int,h:int,kind:str):
    im=transparent(w,h); a=np.zeros((h,w,4),np.uint8); yy,xx=np.mgrid[0:h,0:w]
    cx=(w-1)*.5;cy=(h-1)*.5;r=max(1,min(w,h)*.46);q=np.sqrt((xx-cx)**2+(yy-cy)**2)/r
    pal={"cyan":(30,220,245),"gold":(255,190,35),"purple":(180,70,255),"red":(245,65,80),"orange":(255,110,25)}
    base=pal[kind]; mask=q<=1; light=np.clip(1.22-q*.72 + ((-xx+yy)/(w+h))*.32,0,1)
    for c,v in enumerate(base):a[...,c]=np.where(mask,np.clip(v*(.38+.72*light),0,255),0)
    a[...,3]=np.where(mask,np.clip((1-q)*420,60,255),0)
    im=Image.fromarray(a,"RGBA"); d=ImageDraw.Draw(im)
    if name=="XONI":
        # A small clean-room hover-disc: four stabiliser fins around a bright core.
        for ang in (0,math.pi/2,math.pi,3*math.pi/2):
            ca,sa=math.cos(ang),math.sin(ang)
            p=[(cx+ca*r*.22-sa*r*.18,cy+sa*r*.22+ca*r*.18),(cx+ca*r*.92,cy+sa*r*.92),(cx+ca*r*.22+sa*r*.18,cy+sa*r*.22-ca*r*.18)]
            d.polygon(p,fill=(70,230,255,205),outline=(225,255,255,240))
        d.ellipse((w*.28,h*.28,w*.72,h*.72),fill=(18,116,155,245),outline=(235,255,255,255),width=max(1,w//18))
        d.ellipse((w*.41,h*.41,w*.59,h*.59),fill=(205,250,255,255))
    elif name=="BALL":
        # Spiked flying orb silhouette, distinct from the player disc.
        for i in range(8):
            ang=i*math.pi/4; ca,sa=math.cos(ang),math.sin(ang)
            p1=(cx+ca*r*.58-sa*r*.10,cy+sa*r*.58+ca*r*.10); tip=(cx+ca*r*.98,cy+sa*r*.98); p2=(cx+ca*r*.58+sa*r*.10,cy+sa*r*.58-ca*r*.10)
            d.polygon([p1,tip,p2],fill=(255,128,38,235))
        d.ellipse((w*.24,h*.24,w*.76,h*.76),fill=(175,62,25,245),outline=(255,213,125,255),width=max(1,w//18))
        d.arc((w*.30,h*.28,w*.70,h*.68),205,320,fill=(255,250,220,220),width=2)
    elif name=="MONY":
        pts=[]
        for i in range(10):
            ang=-math.pi/2+i*math.pi/5;rr=r*(.50 if i%2 else .86);pts.append((cx+math.cos(ang)*rr,cy+math.sin(ang)*rr))
        d.polygon(pts,fill=(255,229,90,240),outline=(255,255,224,255))
    elif name=="SPEE":
        for off in (-4,3): d.polygon([(w*.22,h*.25+off),(w*.67,h*.5+off),(w*.22,h*.75+off),(w*.38,h*.5+off)],fill=(245,225,255,235),outline=(255,255,255,220))
    elif name=="HEAR":
        pts=[(w*.5,h*.84),(w*.14,h*.43),(w*.18,h*.25),(w*.32,h*.15),(w*.5,h*.31),(w*.68,h*.15),(w*.82,h*.25),(w*.86,h*.43)]
        d.polygon(pts,fill=(255,82,110,245));d.ellipse((w*.17,h*.14,w*.52,h*.49),fill=(255,82,110,245));d.ellipse((w*.48,h*.14,w*.83,h*.49),fill=(255,82,110,245));d.arc((w*.24,h*.18,w*.58,h*.50),190,300,fill=(255,235,240,210),width=1)
    elif name=="CLCK":
        d.ellipse((w*.20,h*.20,w*.80,h*.80),fill=(18,92,122,235),outline=(245,255,255,250),width=max(1,w//14));d.line((cx,cy,cx,cy-h*.23),fill=(255,255,255,255),width=2);d.line((cx,cy,cx+w*.19,cy+h*.09),fill=(255,255,255,255),width=2);d.ellipse((cx-2,cy-2,cx+2,cy+2),fill=(255,255,255,255))
    return im

def environment(name,w,h):
    """Generate visually distinct clean-room floor/sky materials.

    The compatibility FourCC names are retained, but every pixel is synthesized
    here. r206 deliberately gives each resource family a recognizable theme so
    standalone stages no longer look like diagnostic checkerboards.
    """
    rng=np.random.default_rng(seed(name)); yy,xx=np.mgrid[0:h,0:w]
    phase=(seed(name)%4096)/4096.0*math.tau
    u=xx/max(1,w-1); v=yy/max(1,h-1)

    if name.startswith("SKY"):
        # Deep blue/violet star field with a soft procedural nebula.
        neb=(np.sin(xx*.085+phase)+np.cos(yy*.071-phase)+np.sin((xx+yy)*.037+phase*.7))/3.0
        glow=np.exp(-(((u-.68)/.42)**2+((v-.32)/.34)**2))
        r=7+18*(1-v)+24*np.maximum(neb,0)+35*glow
        g=15+35*(1-v)+18*np.maximum(neb,0)+32*glow
        b=42+92*(1-v)+45*np.maximum(neb,0)+75*glow
        arr=np.stack([r,g,b,np.full_like(r,255)],2).clip(0,255).astype(np.uint8)
        im=Image.fromarray(arr,"RGBA"); d=ImageDraw.Draw(im)
        for _ in range(max(18,w*h//210)):
            x=int(rng.integers(0,w)); y=int(rng.integers(0,h)); c=int(rng.integers(155,256))
            d.point((x,y),fill=(c,c,min(255,c+18),255))
            if rng.random()<.08 and x+1<w:d.point((x+1,y),fill=(c//2,c//2,c,180))
        return im

    if name.startswith("SK"):
        # Cold crystalline surface: blue ice with diagonal fracture lines.
        n=np.sin((xx+yy)*.16+phase)*.45+np.cos((xx-yy)*.11-phase)*.35
        grid=((xx+2*yy+seed(name)%13)%23)<1
        r=24+20*n+grid*38; g=70+35*n+grid*55; b=105+55*n+grid*70
        arr=np.stack([r,g,b,np.full_like(r,255)],2).clip(0,255).astype(np.uint8)
        return Image.fromarray(arr,"RGBA")

    if name.startswith("VOL"):
        # Dark volcanic stone with glowing fissures.
        rock=np.sin(xx*.23+phase)+np.cos(yy*.19-phase)+.55*np.sin((xx-yy)*.12)
        crack=np.abs(np.sin(xx*.105+np.sin(yy*.08+phase)*2.1))
        hot=np.clip((.12-crack)*9.0,0,1)
        r=31+18*rock+205*hot; g=25+11*rock+67*hot; b=35+14*rock+16*hot
        arr=np.stack([r,g,b,np.full_like(r,255)],2).clip(0,255).astype(np.uint8)
        return Image.fromarray(arr,"RGBA")

    if name.startswith("TST"):
        # Neon circuit-grid material for the technological themes.
        base=np.sin(xx*.12+phase)*5+np.cos(yy*.10-phase)*5
        gx=((xx+seed(name)%9)%16)<1; gy=((yy+(seed(name)>>4)%11)%16)<1
        trace=gx|gy
        r=12+base+trace*14; g=37+base+trace*75; b=48+base+trace*96
        nodes=(((xx%32)==0)&((yy%32)==0))
        r+=nodes*50; g+=nodes*95; b+=nodes*105
        arr=np.stack([r,g,b,np.full_like(r,255)],2).clip(0,255).astype(np.uint8)
        return Image.fromarray(arr,"RGBA")

    # Numbered resources become distinct metallic/energy panels selected by a
    # deterministic hue family.  This avoids shipping copied floor textures.
    families=[((20,44,58),(42,120,145)),((38,30,62),(102,68,154)),((25,52,39),(55,132,88)),((56,37,28),(145,91,50))]
    lo,hi=families[seed(name)%len(families)]
    wave=(np.sin(xx*.14+phase)+np.cos(yy*.12-phase)+np.sin((xx+yy)*.055+phase))*.12+.5
    bevel=((xx%16)==0)|((yy%16)==0)
    r=lo[0]+(hi[0]-lo[0])*wave+bevel*22
    g=lo[1]+(hi[1]-lo[1])*wave+bevel*26
    b=lo[2]+(hi[2]-lo[2])*wave+bevel*30
    arr=np.stack([r,g,b,np.full_like(r,255)],2).clip(0,255).astype(np.uint8)
    return Image.fromarray(arr,"RGBA")

def banner(text,w,h,accent=(40,210,245,255)):
    im=transparent(w,h)
    # Vertical dark glass gradient + neon rim.  Rendering into the final RGBA
    # surface keeps the pack libpng-free while looking less like a debug label.
    arr=np.zeros((h,w,4),np.uint8)
    for y in range(h):
        t=y/max(1,h-1)
        arr[y,:,0]=np.uint8(5+7*(1-t)); arr[y,:,1]=np.uint8(17+16*(1-t)); arr[y,:,2]=np.uint8(31+24*(1-t)); arr[y,:,3]=225
    im=Image.fromarray(arr,"RGBA"); d=ImageDraw.Draw(im)
    d.rounded_rectangle((2,2,w-3,h-3),radius=max(4,h//5),outline=accent,width=max(1,h//14))
    if h>=24:
        d.line((8,5,w-9,5),fill=(min(255,accent[0]+70),min(255,accent[1]+35),min(255,accent[2]+20),120),width=1)
    for y in range(7,h-5,5):d.line((7,y,w-8,y),fill=(accent[0],accent[1],accent[2],18))
    centered_text(im,text,(243,250,255,255),(0,5,15,255),min(28,max(10,h-8)))
    return im

def make_font_atlas():
    im=transparent(256,256);d=ImageDraw.Draw(im);f=font(17)
    for idx in range(160):
        if idx<96: ch=chr(idx+0x20)
        elif idx<128: ch=bytes([0xC0+idx-96]).decode('cp1251')
        else: ch=bytes([0xE0+idx-128]).decode('cp1251')
        col,row=idx%16,idx//16
        x0=round((0.005859375+col*0.06191406399011612)*256)
        y0=round((0.009765625+row*0.09847655892372131)*256)
        x1=min(255,round((0.005859375+col*0.06191406399011612+0.060546875)*256))
        y1=min(255,round((0.009765625+row*0.09847655892372131+0.09765625)*256))
        if ch==' ':continue
        bb=d.textbbox((0,0),ch,font=f,stroke_width=0)
        tw,th=bb[2]-bb[0],bb[3]-bb[1]
        x=x0+(x1-x0-tw)//2-bb[0];y=y0+(y1-y0-th)//2-bb[1]
        d.text((x,y),ch,font=f,fill=(245,250,255,255))
    return im

def make_texture(name:str):
    w,h=dims(name)
    if name=="fnt4":return make_font_atlas()
    if name in THEME_NAMES:return environment(name,w,h)
    if name in {"BALL","XONI","SPEE","MONY","HEAR","CLCK"}:
        return radial_icon(name,w,h,{"BALL":"orange","XONI":"cyan","SPEE":"purple","MONY":"gold","HEAR":"red","CLCK":"cyan"}[name])
    if name=="SHAD":
        im=transparent(w,h);a=np.zeros((h,w,4),np.uint8);yy,xx=np.mgrid[0:h,0:w];q=((xx-(w-1)/2)/(w*.5))**2+((yy-(h-1)/2)/(h*.5))**2;a[...,3]=np.where(q<1,np.clip((1-q)*125,0,125),0);return Image.fromarray(a,"RGBA")
    if name in {"VZRV","VZR1"}:
        im=transparent(w,h);d=ImageDraw.Draw(im);rng=np.random.default_rng(seed(name));cx,cy=w/2,h/2
        for i in range(18):
            ang=float(rng.random()*math.tau);r=float(rng.uniform(w*.12,w*.46));x=cx+math.cos(ang)*r;y=cy+math.sin(ang)*r
            d.line((cx,cy,x,y),fill=(255,int(rng.integers(100,230)),30,220),width=max(1,w//24))
        d.ellipse((w*.3,h*.3,w*.7,h*.7),fill=(255,230,120,230));return im.filter(ImageFilter.GaussianBlur(max(.4,w/80)))
    if name=="1111":
        im=transparent(w,h);yy,xx=np.mgrid[0:h,0:w];cx=w*.36;cy=h*.32;dx=(xx-cx)/(w*.43);dy=(yy-cy)/(h*.43);q=dx*dx+dy*dy;fall=np.clip(1-q,0,1);a=(220*fall**2).astype(np.uint8);arr=np.zeros((h,w,4),np.uint8);arr[...,0]=(90+155*fall).astype(np.uint8);arr[...,1]=(125+130*fall).astype(np.uint8);arr[...,2]=255;arr[...,3]=a;return Image.fromarray(arr,'RGBA')
    if name=="LOGO":
        # LOGO is the rotating title coin used by 0x411EF0/main-menu title,
        # not a generic resource-pack badge. Keep clean-room artwork original
        # while displaying the game's own title legibly.
        im=transparent(w,h);d=ImageDraw.Draw(im)
        d.ellipse((18,18,w-19,h-19),fill=(7,28,48,245),outline=(60,230,255,255),width=8)
        d.ellipse((42,42,w-43,h-43),outline=(170,80,255,230),width=4)
        f_air=font(34); f_x=font(62)
        def center_line(text,y,f,fill):
            bb=d.textbbox((0,0),text,font=f,stroke_width=2);tw=bb[2]-bb[0]
            d.text(((w-tw)//2-bb[0],y),text,font=f,fill=fill,stroke_width=2,stroke_fill=(0,8,20,255))
        center_line("AIR",58,f_air,(150,235,255,255))
        center_line("XONIX",102,f_x,(245,255,255,255))
        return im
    if name=="LAXY":
        im=transparent(w,h);d=ImageDraw.Draw(im);d.rounded_rectangle((3,12,w-4,h-13),radius=8,fill=(10,24,44,235),outline=(170,80,255,230),width=2);centered_text(im,"PORT",(230,245,255,255),(0,0,0,255),18);return im
    if name=="TEMP":
        im=Image.new('RGBA',(w,h));px=im.load();
        for x in range(w):
            c=int(80+175*x/max(1,w-1))
            for y in range(h):px[x,y]=(30,c,255,255)
        return im
    if name=="RAM3":return transparent(w,h)
    if name in {"AAAA","RRRR"}:
        return radial_icon("BALL",w,h,"cyan" if name=="AAAA" else "purple")
    if name=="CNT2":
        im=transparent(w,h);d=ImageDraw.Draw(im);f=font(19)
        for n in range(10):
            x=16+n*16;d.text((x+2,0),str(n),font=f,fill=(245,250,255,255),stroke_width=1,stroke_fill=(0,20,40,255))
        return im
    if name=="CNT3":
        im=transparent(w,h);d=ImageDraw.Draw(im);f=font(25)
        for n in range(10):d.text((4+n*24,2),str(n),font=f,fill=(245,250,255,255))
        return im
    labels={
      "LEVL":"ЭТАП","SCOR":"★","PERC":"%","PAUS":"ПАУЗА","XON1":"AIR GRID",
      "GOVE":"ИГРА ОКОНЧЕНА","COMP":"ЭТАП ПРОЙДЕН","cmp2":"ЭТАП ПРОЙДЕН",
      "ABOR":"ПРЕРВАТЬ ИГРУ?","GAME":"ЭТАП","gam2":"ИГРА","IN2$":"СЧЁТ",
      "IN2T":"ВРЕМЯ","IN2L":"ЖИЗНЬ","IN2S":"ЗАМЕДЛЕНИЕ","IN2A":"УСКОРЕНИЕ",
      "TOU2":"ОБЪЕКТЫ И БОНУСЫ","LEV2":"ЭТАП",
      "M101":"ИГРА","M102":"НАСТРОЙКИ","M103":"РЕКОРДЫ","M104":"ИНФОРМАЦИЯ","M106":"ВЫХОД",
      "M250":"СКОРОСТЬ","M260":"ЗВУКИ","M240":"МУЗЫКА","M210":"РЕЧЬ","M220":"УПРАВЛЕНИЕ","M230":"ВОЗВРАТ",
      "on++":"ДА","off+":"НЕТ"
    }
    if name in labels:return banner(labels[name],w,h,(45,215,245,255) if not name.startswith('M2') else (170,90,255,255))
    return banner(name,w,h)

def save_tga(im:Image.Image,path:Path):
    path.parent.mkdir(parents=True,exist_ok=True)
    im.convert('RGBA').save(path,format='TGA',compression=None)

def _edge_fade(sig:np.ndarray,attack_ms=12.0,release_ms=12.0):
    """Fade raw PCM edges to the centre value so looping/one-shots do not click."""
    out=np.array(sig,dtype=np.float64,copy=True)
    if out.ndim==1:out=out[:,None]
    a=max(1,min(len(out)//4,int(SR*attack_ms/1000.0)))
    r=max(1,min(len(out)//4,int(SR*release_ms/1000.0)))
    out[:a]*=np.linspace(0.0,1.0,a)[:,None]
    out[-r:]*=np.linspace(1.0,0.0,r)[:,None]
    return out

def synth_track(track:int,seconds=18.0):
    # Each id has a different harmonic personality.  00 is the menu pulse and
    # 07 is intentionally sparse/ambient for Information, matching the native
    # screen-level music routing without copying any original notes or PCM.
    n=int(SR*seconds);t=np.arange(n,dtype=np.float64)/SR
    bpms=[96,118,124,132,138,146,152,70,158,166];bpm=bpms[track];beat=t*bpm/60.0
    roots=[45,48,50,43,47,40,42,52,38,36]
    scales=[[0,3,7,10],[0,4,7,11],[0,3,7,12],[0,5,7,10],[0,2,7,9]]
    root=roots[track]; scale=scales[track%len(scales)]
    step=np.floor(beat*2).astype(int); note=np.array([scale[i%len(scale)]+12*((i//len(scale))%2) for i in (step%10)])
    freq=440.0*2**((root+note-69)/12); phase=np.cumsum(2*np.pi*freq/SR)
    arp=np.sin(phase)+.29*np.sin(2*phase)+.10*np.sin(3*phase)
    # A second, quieter voice gives gameplay tracks a more arcade-like shimmer.
    lead_freq=freq*(2.0 if track%2 else 1.5); lead=np.sign(np.sin(np.cumsum(2*np.pi*lead_freq/SR)))*.32
    bass_step=np.floor(beat/2).astype(int); bass_note=np.array([scale[i%len(scale)]-12 for i in (bass_step%len(scale))]);bass_f=440*2**((root+bass_note-69)/12);bass=np.sin(np.cumsum(2*np.pi*bass_f/SR))
    pad_f=440*2**((root-12-69)/12);pad=np.sin(2*np.pi*pad_f*t+.45*np.sin(2*np.pi*.13*t))
    frac=beat-np.floor(beat);kick=np.sin(2*np.pi*(49+74*np.exp(-frac*18))*t)*np.exp(-frac*18)
    snfrac=(beat-.5)-np.floor(beat-.5);noise=np.random.default_rng(0xA170+track).normal(0,1,n);snare=noise*np.exp(-snfrac*30)*(snfrac<.17)
    hatsfrac=(beat*2)-np.floor(beat*2);hats=np.random.default_rng(0xB170+track).normal(0,1,n)*np.exp(-hatsfrac*48)*.08
    mix=.22*arp+.07*lead+.17*bass+.075*pad+.075*kick+.021*snare+.018*hats
    if track==0:
        mix=.19*arp+.055*lead+.11*bass+.13*pad+.030*kick
    if track==7:
        chord=np.zeros(n)
        for sem in (0,5,9,12):
            f=440*2**((root+sem-69)/12);chord+=np.sin(2*np.pi*f*t+sem*.17)
        bell=np.sin(2*np.pi*(440*2**((root+12-69)/12))*t)*(.5+.5*np.sin(2*np.pi*.071*t))
        mix=.085*chord+.065*np.sin(2*np.pi*(440*2**((root-12-69)/12))*t)+.025*bell
    pan=.20*np.sin(2*np.pi*beat/16+track*.41);left=mix*(1-pan);right=mix*(1+pan)
    stereo=np.stack([left,right],1);stereo=_edge_fade(stereo,14,18)
    mx=max(.001,float(np.max(np.abs(stereo))));stereo=np.clip(stereo/mx*.78,-1,1)
    return np.round(128+stereo*112).astype(np.uint8).reshape(-1)

def env_decay(n,rate=5.0):
    return np.exp(-np.linspace(0,rate,n))

def synth_sfx(name:str,n:int,index:int):
    t=np.arange(n,dtype=np.float64)/SR;rng=np.random.default_rng(0x5100+index);e=env_decay(n,4.5)
    if name in {"min0","bol1","bol2","bour","clc1","clc2","tick","beep"}:
        f=220+index*19;sig=np.sin(2*np.pi*f*t+3*np.sin(2*np.pi*37*t))*e
    elif name.startswith("bon") or name in {"life","time","slow","acce","up01","levl","comp"}:
        f=380+index*11;sig=(np.sin(2*np.pi*f*t)+.55*np.sin(2*np.pi*f*1.5*t))*e
        sig+=.35*np.sin(2*np.pi*(f*2.0)*t)*np.exp(-np.linspace(0,8,n))
    elif name in {"fire","fir1","fir2","bpop","bvzr","gove","game","cmex"}:
        sig=(.65*rng.normal(0,1,n)+.35*np.sin(2*np.pi*(80+180*e)*t))*e
    elif name in {"sfly","efly","bfly","vint","whip"}:
        f=100+120*np.sin(2*np.pi*(2+index%4)*t);sig=np.sin(2*np.pi*f*t)*e
    else:
        f=150+35*np.sin(2*np.pi*(3+index%5)*t)+22*np.sin(2*np.pi*7*t);sig=(np.sin(2*np.pi*f*t)+.25*np.sin(4*np.pi*f*t))*e
    sig=_edge_fade(sig,2.0,8.0).reshape(-1);mx=max(.001,float(np.max(np.abs(sig))));sig=np.clip(sig/mx*.78,-1,1)
    return np.round(128+sig*110).astype(np.uint8)

def make_level(mode:int,i:int):
    speed=min(18,8+mode*2+i//4); crawler_speed=min(18,8+mode*2+i//5)
    total=min(8,2+mode+i//5)
    pat=(i+mode)%3
    if pat==0:a,b=total,0
    elif pat==1:a,b=0,max(1,total-1)
    else:a,b=(total+1)//2,total//2
    crawlers=min(6,1+mode+i//6)
    homing=0;eraser=0
    if mode>=2 and (i+mode)%5==1:homing=min(14,5+mode*2+i//6)
    if mode>=2 and (i+2*mode)%6==2:eraser=min(15,6+mode*2+i//7)
    if mode>=3 and i%11==10:homing=min(15,8+mode);eraser=min(15,8+mode)
    shapes=[]
    templates=[
      [],[(1,32,32,5)],[(2,32,32,8)],[(1,18,18,4),(1,46,46,4)],
      [(2,18,46,6),(2,46,18,6)],[(1,16,32,3),(1,48,32,3),(2,32,32,5)],
      [(2,12,12,4),(2,52,12,4),(2,12,52,4),(2,52,52,4)],
      [(1,20,20,3),(1,44,20,3),(1,20,44,3),(1,44,44,3),(2,32,32,4)],
    ]
    chosen=templates[(i+mode*2)%len(templates)]
    for j in range(5):shapes.append(chosen[j] if j<len(chosen) else (0,0,0,0))
    typ=[s[0] for s in shapes];xs=[s[1] for s in shapes];ys=[s[2] for s in shapes];rs=[s[3] for s in shapes]
    return bytes([speed,a,b,crawler_speed,crawlers,homing,eraser,(mode*29+i*17)&0x7f,*typ,*xs,*ys,*rs])

def write_soundinf(path:Path):
    data=bytearray(0x1CA4);struct.pack_into('<I',data,0,5)
    for m,name in enumerate(MODE_NAMES):
        raw=name.encode('cp1251')[:11]+b'\0';data[4+m*12:4+m*12+len(raw)]=raw
        struct.pack_into('<I',data,0x64+m*4,COUNTS[m]);struct.pack_into('<I',data,0x84+m*4,STARTS[m])
    idx=0
    for m,count in enumerate(COUNTS):
        for i in range(count):data[0xA4+idx*28:0xA4+(idx+1)*28]=make_level(m,i);idx+=1
    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)

def write_wavepack(path:Path):
    b=bytearray()
    for name,n in WAVEPACK:b+=name[::-1].encode('ascii')+struct.pack('<I',n)
    b+=b'\xff\xff\xff\xff'+struct.pack('<I',512)+b'\0'*8
    assert len(b)==416;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b)

def build_atlas(tile_dir:Path,tiles:List[Tuple[str,int,int]],out:Path):
    im=transparent(256,256)
    for name,x,y in tiles:
        t=Image.open(tile_dir/f'{name}.tga').convert('RGBA');im.alpha_composite(t,(x,y))
    save_tga(im,out)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('root',type=Path,nargs='?',default=Path('assets'));args=ap.parse_args();root=args.root
    tex=root/'textures';music=root/'music';raw=root/'raw';atl=root/'cleanroom'/'atlases'
    for name in TEXTURE_NAMES:save_tga(make_texture(name),tex/f'{name}.tga')
    music.mkdir(parents=True,exist_ok=True)
    for i in range(10):(music/f'{i:02d}.mus').write_bytes(synth_track(i).tobytes())
    bank=bytearray()
    for i,(name,n) in enumerate(WAVEPACK):bank+=synth_sfx(name,n,i).tobytes()
    assert len(bank)==sum(n for _,n in WAVEPACK)==891221;(music/'29.MUS').write_bytes(bank)
    write_soundinf(raw/'SOUNDINF.bin');write_wavepack(raw/'WAVEPACK.bin')
    build_atlas(tex,[("BALL",0,0),("XONI",32,0),("SPEE",0,32),("MONY",32,32),("XON1",0,224),("VZRV",240,32),("CNT2",64,0),("HEAR",64,32),("CLCK",96,32),("LEVL",128,32),("SCOR",192,32),("PERC",216,32),("PAUS",64,112),("VZR1",192,160),("SHAD",0,64)],atl/'atlas3_gameplay.tga')
    build_atlas(tex,[("GOVE",0,0),("COMP",0,48),("ABOR",0,96),("GAME",0,128),("RAM3",128,128),("CNT3",0,220)],atl/'atlas4_stage.tga')
    build_atlas(tex,[("GOVE",0,0),("cmp2",0,48),("ABOR",0,96),("gam2",0,128),("RAM3",128,128),("CNT3",0,220)],atl/'atlas4_complete.tga')
    build_atlas(tex,[("IN2$",0,0),("IN2T",128,0),("IN2L",0,48),("IN2S",128,48),("IN2A",0,96),("TOU2",0,144),("LEV2",0,192)],atl/'atlas7_info.tga')
    manifest=root/'cleanroom'/'MANIFEST.txt';manifest.parent.mkdir(parents=True,exist_ok=True)
    manifest.write_text("AirXonix Native clean-room resource pack %s\nGenerated assets only; no bytes copied from the commercial executable or its MUSIC/BMPPACK/SOUNDINF resources.\nTextures: %d TGA tiles + 4 reference atlases\nMusic: 10 synthesized raw U8 stereo 22050 Hz loops with click-safe edges\nSFX: synthesized 29.MUS bank, 50 samples, exact compatibility lengths\nLevels: 82 newly generated records across 5 modes in SOUNDINF.bin\n"%(PACK_VERSION,len(TEXTURE_NAMES)),encoding='utf-8')
    # Deterministic integrity list for the distributable replacement pack.
    sums=[]
    for p in sorted(x for x in root.rglob('*') if x.is_file() and x.name!='SHA256SUMS.txt'):
        rel=p.relative_to(root).as_posix(); sums.append(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {rel}")
    (root/'cleanroom'/'SHA256SUMS.txt').write_text("\n".join(sums)+"\n",encoding='ascii')
    # Runtime distribution uses one STORE-only ZIP.  The native VFS deliberately
    # supports method 0 so handheld builds need no zlib and resource names such
    # as music/ never collide with an original loose game directory.
    archive=root.parent/'AirXonix-cleanroom.zip'
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_STORED,allowZip64=False) as z:
        for fp in sorted(x for x in root.rglob('*') if x.is_file()):
            z.write(fp,fp.relative_to(root).as_posix(),compress_type=zipfile.ZIP_STORED)
    print('generated',root,'files',len(sums)+1,'archive',archive)
if __name__=='__main__':main()
