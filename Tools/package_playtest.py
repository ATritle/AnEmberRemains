"""Create a self-contained Windows review ZIP; omit build logs and debug symbols."""
from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
import hashlib,json,subprocess
root=Path(__file__).resolve().parents[1]
source=root/'Builds/HushedCloisters/Windows'
name='AER-Playtest-2026-10-08'
out=root/'Builds/Distributions';out.mkdir(parents=True,exist_ok=True)
archive=out/(name+'.zip')
readme='''AN EMBER REMAINS — CONTOUR HUD PLAYTEST — 2026-10-07

WINDOWS 64-BIT
Extract the entire ZIP to a folder. Run Play-An-Ember-Remains.cmd.
Keep the Engine and AnEmberRemains folders beside the executable.
Unreal Editor is not required. If Windows reports missing Microsoft Visual C++
runtime DLLs, run Prerequisites/vc_redist.x64.exe, then launch again.

CONTROLS
Title: click Begin; Sound toggles audio; Quit exits. Arrows/Enter navigate.
Prologue: Space/Next completes the writing; press again to advance.
Esc/Skip skips to Mara descending the stairs.

Left-click an enemy: target and attack. Left-click explored ground: move. WASD overrides the route. Shift: run.
Shift + left-click: cast Arcane Bolt without moving.
1 Arcane Bolt; hold RMB (or 2) Flamethrower; 3 Ice Ring / press again to shatter;
hold 4 Lightning; 5 Frost Nova; 6 Mend.
Round spell icons: click to cast toward your last world aim. For Flame or
Lightning, hold the mouse button and move the cursor back into the world to aim.
Space: phase toward the cursor, or held WASD direction. Alt: dodge.
E: inspect a clue or descend when standing at the revealed first step.
M: map. Esc: pause. R: reset to a new floor-one layout. F10: mute. F1: help.

WHAT IS INCLUDED
Original branded title screen with Niagara flame, animated buttons and clicks.
Three illustrated scenes with stroke-by-stroke handwriting, followed by Mara
walking down the stone staircase into the first dungeon room.
Updated green primary orb, forgiving target hover/hitboxes, continuous channel
audio, stone footsteps, rare rat squeaks and randomized flame-death vocals.
Low foreground walls, mist beyond the boundaries and proximity-based clue cues.
Compact lower-left contour HUD with six round spell buttons in three curved
pairs. Health drains live; spell and phase cooldowns use graphical overlays.
Hover for spell names/keys or the current health value. No permanent bottom bar.
Click-to-move paths avoid scenery and ice, with a destination marker.
Five enemy types, their animations, magic effects, cast/projectile lighting,
layered haze and dungeon audio. Defeat the floor's Vigil Justiciar to slide
the sealed stone coffer aside and reveal the stairs.

SCOPE OF THIS TEST
Mana, level/XP and flask slots are VISUAL LAYOUT PREVIEWS, not progression
systems. Level stays at 1; mana does not limit casting; flasks are inactive.
Use Mend (6) for healing. There is no inventory or campaign save yet.
Health carries into the next floor; R starts a fresh playtest.
The floor elite is not the planned chapter boss, Mother Caldris.

THINGS TO TRY
Check icon size/readability, partial-health fill, cooldowns, channeling via keys
and icons, moving around furniture, and the elite/coffer reveal.
Preview-Enemy-Combat.cmd runs the existing hands-free enemy showcase.
Audio attribution is included in Audio-Credits.txt; script-font attribution
is in Font-Credits.txt.
'''
launch='@echo off\r\ncd /d "%~dp0"\r\nstart "An Ember Remains" "AnEmberRemains.exe" -windowed -ResX=1280 -ResY=800\r\n'
preview=launch.replace('-windowed','-EnemyArena -windowed')
assert (source/'AnEmberRemains.exe').is_file()
assert list(source.rglob('*.ucas'))
with ZipFile(archive,'w',ZIP_DEFLATED,compresslevel=6) as z:
    for path in sorted(source.rglob('*')):
        if not path.is_file() or path.suffix.lower() in {'.pdb','.log'} or path.name.startswith('Manifest_'):continue
        if 'Saved' in path.relative_to(source).parts:continue
        z.write(path,name+'/'+path.relative_to(source).as_posix())
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    z.writestr(name+'/BUILD-INFO.json',json.dumps({'repository_base_commit':commit,'source_state':'local working-tree snapshot; source changes not yet published','build':'2026-10-08 opening playtest','regression_checks':1020243,'regression_errors':0},indent=2)+'\n')
    z.writestr(name+'/READ-ME-FIRST.txt',readme)
    z.writestr(name+'/Play-An-Ember-Remains.cmd',launch)
    z.writestr(name+'/Preview-Enemy-Combat.cmd',preview)
    redist=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Extras/Redist/en-us/vc_redist.x64.exe')
    assert redist.is_file();z.write(redist,name+'/Prerequisites/vc_redist.x64.exe')
with ZipFile(archive) as z:
    assert z.testzip() is None
    assert not any(n.endswith('.pdb') for n in z.namelist())
    entries=len(z.namelist())
digest=hashlib.file_digest(archive.open('rb'),'sha256').hexdigest()
(out/(name+'.sha256.txt')).write_text(digest+'  '+archive.name+'\n')
print(json.dumps({'zip':str(archive),'bytes':archive.stat().st_size,'files':entries,'sha256':digest}))
