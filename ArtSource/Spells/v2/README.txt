Spell visual revision - 2026-10-07

Artwork generated with the built-in image-generation tool. Original outputs
are Flame-source.png and Ice-source.png. Exact prompts are alongside them.
The user's video informed movement/turbulence; no frames were extracted into
runtime textures. The attached ice image was a visual generation reference.

prepare_spell_art_v2.py preserves source artwork, feathers flame tile edges,
adds safe texture padding, aligns ice bases, and measures 48 cast-crystal pivots.
FlameAtlas.png: 16-frame RGBA flipbook, interpolated continuously in Niagara.
IceAtlas.png: four fractured chunk clusters, layered/grown around the caster.
Staff-anchor-review.jpg: magenta markers show measured cast crystal positions.

Runtime: actual Niagara components with per-effect materials. Flame/smoke and
ice use translucent materials; captured RGB and inverse-opacity are composited
premultiplied, not additive. This preserves dark smoke and solid ice. Rear ice
is composited behind the character; front ice covers the lower legs naturally.

1/LMB arcane bolt and held 2/4 channels use measured staff-tip anchors.
3 creates a stationary six-second sanctuary. Incoming damage is blocked while
inside its 1.45-tile safe core. Ice at 1.7 tiles blocks traversal/projectiles.
Press 3 again to shatter without resetting cooldown. Placement skips scenery
and occupied enemy bases; the magical safe core remains protected.
5 Frost Nova: 45 damage within 3.2 tiles, line-of-sight checked; enemies slowed
55% for 2.5 seconds independently of hit flash. Visible cold-cloud shockwave.
3/5/6 intentionally produce ground/self effects after a staff-crystal flash.

No new enemy population or sound pack added in this visual revision.
The original game's project and assets are untouched.
