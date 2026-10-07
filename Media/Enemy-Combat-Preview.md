# Hushed Cloisters combat feedback

Download **Enemy-Combat-Preview.mp4** and play it locally. This is a 60-second capture of the packaged game with synchronized game audio, not a mockup. Each twelve-second chapter demonstrates spawn, idle, approach, actual attacks against Mara, stagger, another impact and death.

| Time | Enemy | Feedback |
|---|---|---|
| 0–12s | Bound Penitent | Binding rattle, short melee cue, cloth/dust impacts and fading bindings |
| 12–24s | Vigil Warden | Cloth windup, baton swing, muted physical impacts |
| 24–36s | Censer Acolyte | Ember seal, censer ignition, turbulent magical projectile, traveling light and burst on collision |
| 36–48s | Reliquary Guard | Heavy metal windup, mace swing, armor sparks, collapse and escaping enchantment |
| 48–60s | Vigil Justiciar | Violet binding seal, enchanted polearm sweep, cast illumination and dissipating magic |

The Acolyte uses magic rather than ammunition. The Justiciar retains its melee reach and enchants its weapon; its visual crescent does not deal extra ranged damage. Health, damage, speed and encounter populations retain their previous tuning.

Niagara effects use the same world-anchored, orthographically composited system as Mara. Animated seals, comet filaments, sweeps, sparks, dust and motes are new procedural material styles. Layered Niagara haze approximates volumetric scattering in this 2D renderer; it is not Unreal's 3D volumetric-fog pass. Casts, in-flight projectiles and impact bursts also illuminate nearby floor, walls and sprites, with line-of-sight checks limiting spill through walls.

Feedback is triggered by gameplay events. Stagger interrupts charging; blocked projectiles burst once; dead enemies cannot repeat death cues. Transient lights and sounds have budgets, decay and pause handling. Repeated channel hits throttle impact sounds. The staged counterstrikes and chapter resets exist only in the preview arena.

Run **Preview-Enemy-Combat.cmd** in the local project to open the looping arena. Normal play uses the same feedback. The arena needs no input and does not change the normal dungeon's layout.

For remote frame stepping, extract **Enemy-Combat-Preview.zip** and open **Enemy-Combat-Review.html** beside the included video. Select an enemy chapter, slow playback, or pause and step frames. The standalone MP4 also works in a normal video player.

Audio reuses the credited dungeon sound library with per-enemy pitch, gain and distance attenuation. See Dungeon-Audio-Credits.txt. Source: EnemyFeedback.cpp, EnemyArena.cpp, CloisterEnemies.cpp, CloisterLighting.cpp and SpellFX.cpp. Capture: -EnemyArena -EnemyCapture. Encoding: Tools/encode_enemy_combat.py.

The preview soundtrack is rebuilt from captured gameplay cue times using the same sound files, gain and pitch values, plus the dungeon ambience bed. This keeps audio aligned when offscreen frame capture slows rendering. Preview gain is normalized for listening; it does not change the game mix.

Validation: the Windows packaged build completed successfully. The packaged regression suite reported 706,005 checks and zero errors, including enemy feedback event counts, stagger interruption, projectile collision, sound throttling and transient light expiry. Existing approved directional sprite sheets were not edited for this update. This GitHub bundle contains review media; the updated playable build and source remain in the local project.
