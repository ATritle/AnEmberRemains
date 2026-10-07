# An Ember Remains

A dark-magic isometric dungeon crawler. Mara Vey descends into the Crucible to
free her sister Elin before the Great Kindling consumes the captive souls below
the cathedral. [Story](Design/Story.md) · [First-floor design](Design/HushedCloisters.md)

Independent UE 5.8 project; no dependency on The Beard and Blade installation,
campaign or release history. Version track: `0.1.0-dev`. No public playable release
is published yet. The current adventurer and enemies are temporary artwork.

Current scope: seeded Hushed Cloisters blockout, continuous isometric navigation,
smooth camera, explored map, room identities and a basic cinder-projectile input
prototype. Final Mara art, spell effects, story interactions and bosses are pending.

Build the AnEmberRemainsEditor target, then use Play-An-Ember-Remains.cmd (requires
UE 5.8). WASD: grid movement; Shift: sprint; LMB: cast; M: map; Esc: pause;
R: new map/reset; E: finish at the sanctum marker. `-IsoVerify` runs regression
tests; `-IsoReview` captures the blockout. User data is separate from the old game.
