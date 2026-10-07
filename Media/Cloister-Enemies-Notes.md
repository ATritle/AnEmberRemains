# Hushed Cloisters: first enemy roster

These five playtest enemies extend the masked wardens, bound penitents and animated ceremonial armor specified in Story.md. They serve the Keepers of the Vigil beneath the cathedral; none are Glassworks constructs, later-region creatures, or replacements for Mother Caldris.

| Enemy | Role | Health | Hit damage | Movement | Readable threat |
|---|---|---:|---:|---:|---|
| Bound Penitent | Swarm, groups of 3–4 | 28 | 5 | 2.1 | Short chained-fist lunge; fragile alone |
| Vigil Warden | Standard melee | 76 | 12 | 1.8 | Masked jailer winds up a capture baton |
| Censer Acolyte | Ranged | 58 | 10 | 1.45 | Censer swing releases a slow, dodgeable ember; maintains distance |
| Reliquary Guard | Heavy melee | 125 | 18 | 1.15 | Hollow ceremonial armor with shield and mace; slower recovery |
| Vigil Justiciar | Rare elite, one per floor | 210 | 26 | 1.4 | Long polearm reach, pronounced overhead windup |

Movement is measured in world units per second. Values are initial playtest tuning. The shield is visual equipment in this pass, not a separate blocking mechanic.

Penitents are adult captives compelled by Vigil bindings, not willing villains or generic zombies. Wardens guard the cells and concealed routes. Acolytes maintain the order's coercive rites. Reliquary Guards are animated ceremonial suits defending confiscated relics. Justiciars are senior enforcers sent to contain intrusions, not campaign bosses.

The entry chamber is safe. Rooms are ordered by reachable distance from the entrance: penitent groups introduce combat; wardens join next; ranged pressure and heavy armor appear deeper; one Justiciar occupies the farthest encounter. Placements reject scenery, the entry area, the stair approach and overlapping enemies. Layout and encounter random streams are independent.

All five use eight screen-space directions. The Guard attack has twelve frames per direction; all other animations have six. Waking enemies cannot attack. Attacks lock their direction at windup, hit once, and recover. Stagger interrupts a windup, with brief resistance to repeated stagger so channels do not permanently lock an elite. Death counts once, finishes its animation, rests, then fades. Wall and ice obstruction apply to movement and hostile projectiles; phase and ice sanctuary protect Mara.

The atlas reviewer is Media/Cloister-Enemies-Preview.html. Download and open locally; it is self-contained. Exact built-in image-generation prompts and original transparent source atlases are under ArtSource/Enemies/v1. Original artwork is not rewritten; measured transparent gutters define texture regions. Other actions use idle-derived uniform scale to avoid inflating crouches or corpses.

Run Play-Standalone.cmd for the combat playtest. Use -NoEnemies to retain environment-only exploration. Existing Mara/spell/environment capture flags remain isolated from encounters. -IsoReview -EnemyReview creates a staged in-dungeon roster screenshot; this is visual review, not an AI encounter.

Guard swing revision: twelve poses per direction replace the short thrust with a raised mace, downward swing, body lean and recovery. The mace remains on the anatomical right arm and shield on the left. A 0.95-second windup leads into impact, followed by 0.45 seconds of follow-through and recovery animation. Each Guard direction uses a fixed scale from its neutral pose, so raising the mace does not resize the body. All other actions retain six frames per direction. Guard-Swing-Preview.zip is the smaller Guard-only offline reviewer. Source sheets and exact built-in image-generation prompts are in ArtSource/Enemies/v3/Guard-Swing.

Download Guard-Swing-Preview.zip, extract it, and open Guard-Swing-Preview.html in a browser. It works offline. Select Attack, use Pause and Next frame, and compare all eight directions below. The twelve-frame east attack includes a one-frame impact hold. Reliquary-Guard-Attack.png shows the south-facing twelve-frame sequence.

Validation: final Windows Shipping build succeeded; packaged regression verification completed 705,996 checks with zero errors, including all attack frames, direction-specific impact timing, texture bounds, cooked assets and combat behavior.
