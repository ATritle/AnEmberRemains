# Contour HUD playtest — October 7, 2026

Windows 64-bit playable build of **An Ember Remains**, including the selected round contour HUD.

Download **AER-Contour-Playtest-2026-10-07.zip**, extract the entire archive, and run **Play-An-Ember-Remains.cmd**. Unreal Editor is not required. Keep both included game folders beside the executable. The included Microsoft Visual C++ redistributable is available if the PC reports missing runtime DLLs.

This build includes:

- Compact lower-left health/mana instrument with six round spell buttons in three pairs following the mana contour.
- Live health depletion, spell cooldowns, phase readiness, hover hints, and clickable/holdable spell icons. F1 opens the control guide.
- Click-to-move with obstacle-aware paths, optional WASD, and Shift + left-click casting.
- Five enemy types, current animations, magical effects, cast/projectile lighting, layered haze and dungeon audio.
- A hidden staircase revealed when the floor's Vigil Justiciar dies and the stone coffer slides aside.

**Scope:** mana, level/XP and flasks are visual placeholders in this HUD playtest. Level stays at 1; mana does not limit casting; flasks are inactive. Use spell 6 (Mend) to heal. Inventory, campaign progression, and saving are not implemented yet. Health carries between floors; R resets the playtest.

Controls and audio credits are included in the ZIP. The packaged build is produced from the current local working project; this release does not represent a clean source checkout of its Git tag. The repository retains its existing preview-media publication scope.

Validation: Windows shipping package succeeded; regression suite reported **995,203 checks, zero errors**. The HUD was visually inspected at full and partial health, with spell and phase cooldowns active. Archive CRC verification passed.

Download size: **375,792,401 bytes** (about 376 MB).

SHA-256: `a6edb35e8bacbae3dd3e57687bad0409b29827d0d1118c51f309a00fc27801e5`

![Contour HUD in the playable build](https://raw.githubusercontent.com/ATritle/AnEmberRemains/main/Media/Contour-HUD-Playtest.png)
