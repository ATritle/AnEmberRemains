MARA VEY — STAFF CASTER — FIRST PLAYABLE ANIMATION SET

Eight original transparent atlases: N NE E SE S SW W NW.
Each sheet has six action rows, six frames left to right:
idle breathing / walk / run / dodge / roll / staff cast.
288 frames total. Cast release is frame index 3 (fourth pose).

Generated with built-in image_gen, based on the user's red-haired sorceress
inspiration. Prompt set and generated source paths: generation.json.
Opaque turnaround concepts were rejected and are not used in the game.
Selected directional source PNGs retain their original generated alpha.
Directional sheets were separately generated, not mirrored.

Do NOT slice these atlases uniformly. Animation row and column spacing varies.
frames.json stores cleaned-atlas rectangles, source rectangles, ground pivots and
constant per-direction scale. Tools/repack_mara.py isolates connected figures,
preserves their original RGBA edge pixels, and translates each into a padded cell.
Original sheets are never overwritten; cleaned atlases are in Clean/. Runtime
uses these cleaned atlases. No artwork resampling or mirroring is performed.
Tools/measure_mara.py is retired; use the repacker instead.
Clean/QA.json records pixel-preservation and padding checks for all 288 frames.
Clean/Animation-review.gif is a six-frame contact-sheet loop for visual review;
its common playback rate is not the different action timings used in the game.

This is a first playable pass for animation review, not a claim of final AAA
motion quality. Six-frame gait cycles may need additional in-betweens and manual
pose corrections after playtesting. No hurt/death/interaction animations were
requested in this batch. Other magic types and skill progression remain future work.

Controls: WASD move, Shift run, LMB cast, Alt dodge, Space roll.
The original TBAB adventurer assets remain untouched in Content/Art/V2.
