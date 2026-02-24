SeaFloor System — Implementation Plan
Architecture
Replace the current Plants class with a new SeaFloor class. SeaFloor owns a collection of GroundElement structs, each representing one item on the sea bed. The Aquarium class holds a single SeaFloor instance instead of std::vector<std::unique_ptr<Plants>>.
Ground Elements (7 types)
1. Substrate (sand layer)
Bottom 2 rows of the screen
Per-pixel color: base tan/brown (15, 10, 5) with slight random variation per pixel (+/- a few on each channel)
Generated once, stored as a small color array (2 x screenWidth)
Completely static, drawn first (behind everything else on the ground)
2. Rocks
3-5 placed randomly along the bottom
Small filled shapes: 3-6px wide, 2-4px tall
Drawn as a few filled pixel rows (wider at base, narrower at top — crude trapezoid)
Gray/brown palette, each rock gets a random shade at generation
Fully static
3. Short Moss / Carpet Plants
4-6 small clusters scattered along the bottom, sometimes on/next to rocks
2-3px tall, 3-5px wide
Dark green pixel clusters with slight shade variation per pixel
Static (no sway) — they're ground cover
4. Seagrass Blades
5-8 individual blades placed randomly
Single-pixel-wide vertical lines, 5-10px tall
Drawn pixel-by-pixel from bottom up with a small horizontal sin offset that increases with height (sway)
Each blade has its own phase offset and slightly different green hue
Gentle slow sway via sin(millis() / period + phaseOffset)
5. Branching Plants (refined version of current Plants)
2-4 placed along the bottom (fewer than current 9, but more varied)
Keep the existing branch/node algorithm but with improvements:
Fewer branches per plant (4-8 instead of 10-16) since there are other element types now
Color variety: stems in darker greens/teals instead of near-black; flower colors randomly chosen from a palette (pink, orange, blue, white, yellow, magenta) instead of always yellow
Same humidity-driven size scaling
Same sway behavior
6. Coral
2-4 coral formations
Two sub-types:
Brain coral: Small filled circle/oval (2-3px radius), solid warm color (pink, orange, purple). Static.
Branching coral: Small forked pixel structure (3-5px tall), drawn as 2-3 short lines diverging from a base point. Rigid, no sway. Warm colors.
Each coral has a bloom timer (see Dynamic Changes below)
7. Anemone
1-2 only (rare, eye-catching)
A 2-3px wide base, with 3-5 tiny "tentacles" (1-2px each) that extend upward
Tentacles gently pulse: their length oscillates slowly (±1px) via sin wave
Bright accent colors at tentacle tips (green, magenta, cyan)
Total height: 4-6px
Element Struct
struct GroundElement {  uint8_t type;       // SUBSTRATE, ROCK, MOSS, SEAGRASS, PLANT, CORAL, ANEMONE  uint8_t x, y;       // base position  uint8_t width, height;  uint8_t variant;    // sub-type or visual variant  CRGB baseColor;  CRGB accentColor;   // flower/tip color  float phase;        // animation phase offset  float bloomTimer;   // for bloom events  uint8_t bloomState; // 0=normal, 1=blooming, 2=fading  // For branching plants: small inline array or index into branch data};
For branching plants specifically, we keep a small separate array of branch/node data (similar to current Branch struct), indexed by the element.
Draw Order
In Aquarium::update():
1. updateWater()           — background2. planktonField           — foreground (ambient)3. boidManager             — foreground (small schooling fish)4. updateFish()            — foreground (main creatures)5. seaFloor.draw()         — foreground (LAST — ground occludes fish near bottom)6. updateSensorData()      — foreground (text overlay)
Ground elements drawn back-to-front within seaFloor.draw():
Substrate (sand)
Rocks
Moss
Coral
Branching plants
Seagrass
Anemones
Dynamic Changes (gradual, cheap)
Bloom events:
Each coral and anemone has a bloomTimer that counts up in update()
Every ~20-40 minutes (randomized per element), one triggers a "bloom": its colors brighten/shift over ~30 seconds, hold for 2-3 minutes, then fade back over ~30 seconds
Implementation: just lerp the draw color between baseColor and a brighter bloomColor. One float per element.
Plant growth drift:
Branching plants and seagrass have a targetHeight derived from humidity (like current system)
Actual drawn height lerps slowly toward target (e.g., height += (target - height) * 0.001 per frame)
Result: plants visibly grow or shrink over minutes when humidity changes, not instantly
Flower color cycling:
Branching plant flowers already have per-plant accent colors
Over hours, the accent hue could slowly rotate (e.g., hue += 0.001 per update). Imperceptible in real-time, but the aquarium looks subtly different each day.
Algae spread (stretch goal):
A slow counter (incremented hourly) that occasionally adds a moss pixel near an existing rock
Very simple: pick a random rock, pick an adjacent empty pixel, add a moss-colored pixel
Capped at some maximum so it doesn't take over
Integration Changes to Aquarium.h
Remove #include "Plants.h" and std::vector<std::unique_ptr<Plants>> plantArray
Add #include "SeaFloor.h" and SeaFloor seaFloor
In constructor: initialize seaFloor with matrix pointer
In begin(): replace initializePlants() with seaFloor.generate()
In update(): replace updatePlants() with seaFloor.update(humidity) + seaFloor.draw()
In demo mode: same — just call seaFloor.update(demoHumidity) and seaFloor.draw()
File Structure
lib/Aquarium/SeaFloor.h — main class, element generation, update, draw dispatch
lib/Aquarium/Plants.h — keep for now or fold the branching plant drawing code into SeaFloor (single file is fine for this scope, branching plants are ~40 lines of draw code)
Settings to add to AquariumSettings.h
#define NUM_ROCKS 4#define NUM_MOSS_CLUSTERS 5#define NUM_SEAGRASS 7#define NUM_BRANCHING_PLANTS 3#define NUM_CORAL 3#define NUM_ANEMONES 1#define GROUND_SUBSTRATE_HEIGHT 2#define GROUND_MAX_HEIGHT 12         // no element taller than this#define BLOOM_INTERVAL_MIN_MS 1200000  // 20 minutes#define BLOOM_INTERVAL_MAX_MS 2400000  // 40 minutes#define BLOOM_DURATION_MS 180000       // 3 minutes#define BLOOM_FADE_MS 30000            // 30 second fade in/out
Execution Order
Create SeaFloor.h with the element struct and generation logic (substrate, rocks, moss — the static stuff first)
Add seagrass with sway animation
Port branching plants from current Plants.h with color improvements
Add coral (both variants) with bloom timer
Add anemones with tentacle pulse
Integrate into Aquarium.h (replace old plant system)
Add dynamic growth/bloom logic
Tune — adjust counts, colors, heights, timing on real hardware
Steps 1-3 get us a massive visual upgrade. Steps 4-5 add richness. Steps 6-7 are integration and polish. Step 8 is inevitable iteration.
Ready to start building?