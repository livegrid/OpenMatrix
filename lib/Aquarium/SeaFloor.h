#ifndef SEAFLOOR_H
#define SEAFLOOR_H

#include <Arduino.h>
#include <Matrix.h>
#include "AquariumSettings.h"

// Element types — stored in draw order during generate()
enum ElementType : uint8_t {
  ELEM_ROCK = 0,
  ELEM_MOSS,
  ELEM_CORAL_BRAIN,
  ELEM_CORAL_BRANCH,
  ELEM_CORAL_FAN,
  ELEM_PLANT,
  ELEM_SEAGRASS,
  ELEM_ANEMONE
};

// ─── Branching plant structs ───

struct BranchNode {
  float x, y;
};

struct BranchData {
  BranchNode nodes[MAX_NODES_PER_BRANCH];
  uint8_t nodeCount;
  float phaseOffset;
};

struct PlantData {
  BranchData branches[MAX_BRANCHES_PER_PLANT];
  uint8_t branchCount;
  float accentHue;
};

// ─── Anemone structs ───

struct TentacleData {
  int8_t xOffset;
  uint8_t baseLength;
  float phaseOffset;
  CRGB tipColor;
};

struct AnemoneData {
  TentacleData tentacles[MAX_TENTACLES];
  uint8_t tentacleCount;
};

// ─── Coral structs ───

// A single line segment in a coral structure (pre-generated at init)
struct CoralSegment {
  int8_t x0, y0;  // start offset from coral base
  int8_t x1, y1;  // end offset from coral base
  uint8_t depth;   // recursion depth (0=trunk, higher=thinner)
};

// Pre-generated geometry for one coral element
// Recursive branch: up to ~15 segments (trunk splits into 2-3, each splits again)
// Fan: up to ~10 rays
// Brain: up to ~6 overlapping circles stored as segments (x0,y0=center, x1=radius)

struct CoralData {
  CoralSegment segments[MAX_CORAL_SEGMENTS];
  uint8_t segmentCount;
};

// ─── Unified ground element ───

struct GroundElement {
  uint8_t type;
  uint8_t x, y;
  uint8_t width, height;
  CRGB baseColor;
  CRGB accentColor;    // tip/highlight color for coral
  float phase;

  // Bloom system (coral + anemone)
  uint8_t bloomState;
  unsigned long bloomTimerStart;
  unsigned long nextBloomTime;

  // Growth drift (seagrass + plants)
  float currentHeight;
  float targetHeight;

  // Index into type-specific data arrays
  uint8_t dataIndex;
};

static const uint8_t MAX_ELEMENTS = NUM_ROCKS + NUM_MOSS_CLUSTERS +
    NUM_SEAGRASS + NUM_BRANCHING_PLANTS + NUM_CORAL + NUM_ANEMONES;

class SeaFloor {
 private:
  Matrix* matrix;
  uint8_t screenWidth;
  uint8_t screenHeight;

  // Substrate: pre-generated sand colors
  CRGB substrateColors[GROUND_SUBSTRATE_HEIGHT][MAX_SCREEN_WIDTH];

  // All ground elements in draw order
  GroundElement elements[MAX_ELEMENTS];
  uint8_t elementCount;

  // Type-specific extra data
  PlantData plantData[NUM_BRANCHING_PLANTS > 0 ? NUM_BRANCHING_PLANTS : 1];
  AnemoneData anemoneData[NUM_ANEMONES > 0 ? NUM_ANEMONES : 1];
  CoralData coralData[NUM_CORAL > 0 ? NUM_CORAL : 1];

 public:
  SeaFloor(Matrix* m)
      : matrix(m), elementCount(0) {
    screenWidth = m->getXResolution();
    screenHeight = m->getYResolution();
  }

  void generate() {
    elementCount = 0;
    generateSubstrate();
    generateRocks();
    generateMoss();
    generateCoral();
    generatePlants();
    generateSeagrass();
    generateAnemones();
  }

  void update(float humidity) {
    for (uint8_t i = 0; i < elementCount; i++) {
      updateBloom(elements[i]);
      updateGrowthDrift(elements[i], humidity);

      if (elements[i].type == ELEM_PLANT) {
        PlantData& pd = plantData[elements[i].dataIndex];
        pd.accentHue += 0.001f;
        if (pd.accentHue >= 256.0f) pd.accentHue -= 256.0f;
      }
    }
  }

  void draw() {
    drawSubstrate();
    for (uint8_t i = 0; i < elementCount; i++) {
      switch (elements[i].type) {
        case ELEM_ROCK:         drawRock(elements[i]); break;
        case ELEM_MOSS:         drawMoss(elements[i]); break;
        case ELEM_CORAL_BRAIN:  drawCoralBrain(elements[i]); break;
        case ELEM_CORAL_BRANCH: drawCoralBranch(elements[i]); break;
        case ELEM_CORAL_FAN:    drawCoralFan(elements[i]); break;
        case ELEM_PLANT:        drawPlant(elements[i]); break;
        case ELEM_SEAGRASS:     drawSeagrass(elements[i]); break;
        case ELEM_ANEMONE:      drawAnemone(elements[i]); break;
      }
    }
  }

 private:
  // ─── Helpers ───

  GroundElement& addElement() {
    GroundElement& el = elements[elementCount++];
    memset(&el, 0, sizeof(GroundElement));
    return el;
  }

  static uint8_t clampU8(int val) {
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (uint8_t)val;
  }

  // Lerp between two colors by t (0-255)
  static CRGB lerpColor(CRGB a, CRGB b, uint8_t t) {
    return CRGB(
      ((255 - t) * a.r + t * b.r) >> 8,
      ((255 - t) * a.g + t * b.g) >> 8,
      ((255 - t) * a.b + t * b.b) >> 8
    );
  }

  CRGB applyBloom(const GroundElement& el, CRGB base) {
    if (el.bloomState == 0) return base;

    unsigned long elapsed = millis() - el.bloomTimerStart;
    float intensity = 0.0f;

    switch (el.bloomState) {
      case 1: intensity = (float)elapsed / (float)BLOOM_FADE_MS; break;
      case 2: intensity = 1.0f; break;
      case 3: intensity = 1.0f - (float)elapsed / (float)BLOOM_FADE_MS; break;
    }
    intensity = constrain(intensity, 0.0f, 1.0f);

    uint8_t boost = (uint8_t)(intensity * 75);
    return CRGB(
      min(255, (int)base.r + boost),
      min(255, (int)base.g + boost),
      min(255, (int)base.b + boost)
    );
  }

  // ─── Update Helpers ───

  void updateBloom(GroundElement& el) {
    if (el.type != ELEM_CORAL_BRAIN && el.type != ELEM_CORAL_BRANCH &&
        el.type != ELEM_CORAL_FAN && el.type != ELEM_ANEMONE) return;

    unsigned long now = millis();
    switch (el.bloomState) {
      case 0:
        if (now >= el.nextBloomTime) {
          el.bloomState = 1;
          el.bloomTimerStart = now;
        }
        break;
      case 1:
        if (now - el.bloomTimerStart >= BLOOM_FADE_MS) {
          el.bloomState = 2;
          el.bloomTimerStart = now;
        }
        break;
      case 2:
        if (now - el.bloomTimerStart >= BLOOM_DURATION_MS) {
          el.bloomState = 3;
          el.bloomTimerStart = now;
        }
        break;
      case 3:
        if (now - el.bloomTimerStart >= BLOOM_FADE_MS) {
          el.bloomState = 0;
          el.nextBloomTime = now + random(BLOOM_INTERVAL_MIN_MS, BLOOM_INTERVAL_MAX_MS);
        }
        break;
    }
  }

  void updateGrowthDrift(GroundElement& el, float humidity) {
    if (el.type != ELEM_SEAGRASS && el.type != ELEM_PLANT) return;
    float scale = map(humidity, 0, 100, 50, 250) / 100.0f;
    el.targetHeight = el.height * scale;
    el.currentHeight += (el.targetHeight - el.currentHeight) * 0.001f;
  }

  // ─── Generation ───

  void generateSubstrate() {
    // Substrate generation disabled — no sand layer
  }

  void generateRocks() {
    for (uint8_t i = 0; i < NUM_ROCKS; i++) {
      GroundElement& el = addElement();
      el.type = ELEM_ROCK;
      el.x = screenWidth * i / NUM_ROCKS + random(2, screenWidth / NUM_ROCKS - 2);
      el.y = screenHeight - GROUND_SUBSTRATE_HEIGHT - 1;
      el.width = random(3, 7);
      el.height = random(2, 5);
      el.baseColor = CRGB(random(60, 90), random(55, 75), random(50, 65));
    }
  }

  void generateMoss() {
    for (uint8_t i = 0; i < NUM_MOSS_CLUSTERS; i++) {
      GroundElement& el = addElement();
      el.type = ELEM_MOSS;
      el.y = screenHeight - GROUND_SUBSTRATE_HEIGHT - 1;
      el.width = random(3, 6);
      el.height = random(2, 4);
      if (random(0, 2) == 0 && i < NUM_ROCKS) {
        el.x = elements[i].x + random(-3, 4);
      } else {
        el.x = random(0, screenWidth);
      }
      el.baseColor = CRGB(random(0, 15), random(50, 80), random(10, 25));
    }
  }

  // ─── Coral Generation ───

  void generateCoral() {
    for (uint8_t i = 0; i < NUM_CORAL; i++) {
      GroundElement& el = addElement();
      el.y = screenHeight - 1;  // bottom of screen
      el.x = random(8, screenWidth - 8);
      el.dataIndex = i;

      // Pick a rich coral hue — wider palette than before
      // red, orange, salmon, pink, magenta, purple, teal
      const uint8_t coralHues[] = {0, 12, 20, 230, 210, 195, 128};
      uint8_t baseHue = coralHues[random(0, 7)];
      hsv2rgb_rainbow(CHSV(baseHue, 200, 180), el.baseColor);
      // Accent is a brighter/shifted version for tips
      hsv2rgb_rainbow(CHSV(baseHue + 15, 255, 255), el.accentColor);

      // Bloom timer
      el.bloomState = 0;
      el.nextBloomTime = millis() + random(BLOOM_INTERVAL_MIN_MS, BLOOM_INTERVAL_MAX_MS);

      CoralData& cd = coralData[i];
      cd.segmentCount = 0;

      // Pick type: 0=recursive branch, 1=brain cluster, 2=fan
      uint8_t coralType = random(0, 3);

      if (coralType == 0) {
        el.type = ELEM_CORAL_BRANCH;
        el.height = random(6, 11);  // 6-10px tall
        el.width = random(5, 9);    // spread
        generateRecursiveBranch(cd, 0, 0, 0, -(int8_t)el.height, el.width, 0);
      } else if (coralType == 1) {
        el.type = ELEM_CORAL_BRAIN;
        el.width = random(4, 7);    // larger cluster radius for more impressive brain coral
        el.height = el.width;
        generateBrainCluster(cd, el.width);
      } else {
        el.type = ELEM_CORAL_FAN;
        el.height = random(6, 10);  // 6-9px tall
        el.width = random(5, 9);    // angular spread
        generateFanCoral(cd, el.height, el.width);
      }
    }
  }

  // Recursive branching: trunk splits into sub-branches, each can split again
  // Segments stored as offsets from coral base position
  void generateRecursiveBranch(CoralData& cd, int8_t x0, int8_t y0,
                                int8_t x1, int8_t y1, uint8_t spread, uint8_t depth) {
    if (cd.segmentCount >= MAX_CORAL_SEGMENTS) return;
    if (depth > 2) return;  // max 3 levels (0, 1, 2)

    // Store this segment
    CoralSegment& seg = cd.segments[cd.segmentCount++];
    seg.x0 = x0; seg.y0 = y0;
    seg.x1 = x1; seg.y1 = y1;
    seg.depth = depth;

    // Branch length for children (shorter each level)
    int8_t segLen = (int8_t)(abs(y1 - y0) * 6 / 10);
    if (segLen < 2) segLen = 2;

    // How many sub-branches from the tip of this segment
    uint8_t numChildren = (depth == 0) ? random(2, 4) : random(1, 3);

    for (uint8_t c = 0; c < numChildren; c++) {
      if (cd.segmentCount >= MAX_CORAL_SEGMENTS) return;

      // Spread children outward from tip
      float childAngle;
      if (numChildren == 1) {
        // Single child: slight random deflection
        childAngle = -PI / 2.0f + (random(-30, 31) / 100.0f);
      } else {
        // Fan children across an arc
        float arcStart = -PI / 2.0f - (spread * 0.04f);
        float arcEnd   = -PI / 2.0f + (spread * 0.04f);
        childAngle = arcStart + (arcEnd - arcStart) * c / (numChildren - 1);
        childAngle += (random(-15, 16) / 100.0f);  // jitter
      }

      int8_t cx1 = x1 + (int8_t)(cos(childAngle) * segLen);
      int8_t cy1 = y1 + (int8_t)(sin(childAngle) * segLen);

      generateRecursiveBranch(cd, x1, y1, cx1, cy1, spread / 2, depth + 1);
    }
  }

  // Brain coral: cluster of overlapping larger circles for a bumpy organic mass
  void generateBrainCluster(CoralData& cd, uint8_t radius) {
    // Generate 4-7 overlapping circles, larger radii, tighter cluster
    uint8_t numBlobs = random(4, 8);
    for (uint8_t b = 0; b < numBlobs && cd.segmentCount < MAX_CORAL_SEGMENTS; b++) {
      CoralSegment& seg = cd.segments[cd.segmentCount++];
      // Position blobs in a tight cluster — use smaller offset range for more overlap
      float angle = random(0, 628) / 100.0f;
      float dist = random(0, radius * 40) / 100.0f;  // tighter: 0 to 0.4*radius
      seg.x0 = (int8_t)(cos(angle) * dist);
      seg.y0 = (int8_t)(sin(angle) * dist) - radius;  // offset upward from base
      seg.x1 = random(2, 4);  // sub-circle radius 2-3px (bigger than before)
      seg.y1 = 1;  // flag: 1 = filled circle
      seg.depth = b;  // index for color variation
    }
  }

  // Fan coral: radiating lines from a base point, like a sea fan / gorgonian
  void generateFanCoral(CoralData& cd, uint8_t height, uint8_t spread) {
    uint8_t numRays = random(5, 9);  // 5-8 radiating rays
    // Arc from roughly -30° to +30° off vertical (upward)
    float arcStart = -PI / 2.0f - (spread * 0.035f);
    float arcEnd   = -PI / 2.0f + (spread * 0.035f);

    for (uint8_t r = 0; r < numRays && cd.segmentCount < MAX_CORAL_SEGMENTS; r++) {
      CoralSegment& seg = cd.segments[cd.segmentCount++];
      seg.x0 = 0;
      seg.y0 = 0;

      // Evenly spread across the arc with small jitter
      float rayAngle = arcStart + (arcEnd - arcStart) * r / (numRays - 1);
      rayAngle += (random(-8, 9) / 100.0f);

      // Vary length per ray for organic look
      uint8_t rayLen = height - random(0, 3);
      seg.x1 = (int8_t)(cos(rayAngle) * rayLen);
      seg.y1 = (int8_t)(sin(rayAngle) * rayLen);
      seg.depth = 0;
    }
  }

  void generatePlants() {
    const uint8_t accentHues[] = {0, 20, 32, 140, 200, 210};

    for (uint8_t i = 0; i < NUM_BRANCHING_PLANTS; i++) {
      GroundElement& el = addElement();
      el.type = ELEM_PLANT;
      el.x = screenWidth * (i + 1) / (NUM_BRANCHING_PLANTS + 1) + random(-5, 6);
      el.y = screenHeight + 7;
      el.dataIndex = i;
      el.height = 10;
      el.currentHeight = 10.0f;
      el.targetHeight = 10.0f;

      uint8_t stemHue = random(85, 130);
      CRGB stemColor;
      hsv2rgb_rainbow(CHSV(stemHue, 180, 80), stemColor);
      el.baseColor = stemColor;

      uint8_t chosenHue = accentHues[random(0, 6)];
      CRGB accent;
      hsv2rgb_rainbow(CHSV(chosenHue, 220, 255), accent);
      el.accentColor = accent;

      PlantData& pd = plantData[i];
      pd.accentHue = (float)chosenHue;
      pd.branchCount = random(4, 9);

      const float branchSizeBase = 3.0f;

      for (uint8_t b = 0; b < pd.branchCount; b++) {
        BranchData& branch = pd.branches[b];
        branch.phaseOffset = random(0, 500) / 100.0f;
        float startAngle = random((int)(PI * 1000), (int)(TWO_PI * 1000)) / 1000.0f;
        branch.nodes[0].x = cos(startAngle) * branchSizeBase;
        branch.nodes[0].y = sin(startAngle) * branchSizeBase;
        branch.nodeCount = random(4, 9);
        for (uint8_t j = 1; j < branch.nodeCount; j++) {
          branch.nodes[j] = buildNode(branch.nodes[j - 1], branchSizeBase);
        }
      }
    }
  }

  void generateSeagrass() {
    for (uint8_t i = 0; i < NUM_SEAGRASS; i++) {
      GroundElement& el = addElement();
      el.type = ELEM_SEAGRASS;
      el.x = screenWidth * i / NUM_SEAGRASS + random(0, screenWidth / NUM_SEAGRASS);
      el.y = screenHeight - GROUND_SUBSTRATE_HEIGHT;
      el.height = random(5, 11);
      el.currentHeight = (float)el.height;
      el.targetHeight = (float)el.height;
      el.phase = random(0, 628) / 100.0f;

      uint8_t hue = random(75, 105);
      CRGB color;
      hsv2rgb_rainbow(CHSV(hue, 200, 150), color);
      el.baseColor = color;
    }
  }

  void generateAnemones() {
    const uint8_t tipHues[] = {96, 200, 128};

    for (uint8_t i = 0; i < NUM_ANEMONES; i++) {
      GroundElement& el = addElement();
      el.type = ELEM_ANEMONE;
      el.x = random(10, screenWidth - 10);
      el.y = screenHeight - GROUND_SUBSTRATE_HEIGHT - 1;
      el.width = random(2, 4);
      el.height = random(4, 7);
      el.dataIndex = i;
      el.baseColor = CRGB(80, 40, 60);

      el.bloomState = 0;
      el.nextBloomTime = millis() + random(BLOOM_INTERVAL_MIN_MS, BLOOM_INTERVAL_MAX_MS);

      AnemoneData& ad = anemoneData[i];
      ad.tentacleCount = random(3, 6);

      for (uint8_t t = 0; t < ad.tentacleCount; t++) {
        TentacleData& tent = ad.tentacles[t];
        tent.xOffset = (int8_t)map(t, 0, ad.tentacleCount - 1,
                                    -(int)el.width / 2, (int)el.width / 2);
        tent.baseLength = random(3, 6);
        tent.phaseOffset = random(0, 628) / 100.0f;

        CRGB tipColor;
        hsv2rgb_rainbow(CHSV(tipHues[random(0, 3)], 255, 255), tipColor);
        tent.tipColor = tipColor;
      }
    }
  }

  BranchNode buildNode(BranchNode prev, float branchSizeBase) {
    float theta = atan2(prev.y, prev.x);
    float target = 3.0f * PI / 2.0f;
    float diff = target - theta;
    if (diff > PI) diff -= TWO_PI;
    else if (diff < -PI) diff += TWO_PI;
    float newAngle = theta + diff * 0.5f;

    BranchNode node;
    node.x = cos(newAngle) * branchSizeBase + prev.x;
    node.y = sin(newAngle) * branchSizeBase + prev.y;
    return node;
  }

  // ─── Drawing ───

  void drawSubstrate() {
    // Substrate disabled — no ground layer
  }

  void drawRock(const GroundElement& el) {
    int16_t baseX = el.x - el.width / 2;
    for (int8_t row = 0; row < el.height; row++) {
      uint8_t inset = (row * el.width) / (el.height * 3);
      uint8_t rowWidth = el.width - 2 * inset;
      int16_t rowY = el.y - row;
      for (uint8_t px = 0; px < rowWidth; px++) {
        int16_t drawX = baseX + inset + px;
        uint8_t variation = ((drawX * 7 + rowY * 13) & 0x0F);
        CRGB color = el.baseColor;
        color.nscale8(240 + variation);
        matrix->foreground->drawPixel(drawX, rowY, color);
      }
    }
  }

  void drawMoss(const GroundElement& el) {
    int16_t baseX = el.x - el.width / 2;
    for (int8_t row = 0; row < el.height; row++) {
      for (uint8_t col = 0; col < el.width; col++) {
        if (((col + row) * 11 + el.x) % 5 == 0) continue;
        int16_t drawY = el.y - row;
        CRGB color = el.baseColor;
        uint8_t shade = 200 + ((col * 7 + row * 3) & 0x3F);
        if (shade > 255) shade = 255;
        color.nscale8(shade);
        matrix->foreground->drawPixel(baseX + col, drawY, color);
      }
    }
  }

  // ─── Coral Drawing ───

  // Recursive branching coral: draw pre-generated segments with color gradient
  // Trunk is thicker (drawn with drawCircleArray for organic width),
  // branches get thinner at deeper levels, color shifts from base to accent at tips
  void drawCoralBranch(const GroundElement& el) {
    const CoralData& cd = coralData[el.dataIndex];
    CRGB baseCol = applyBloom(el, el.baseColor);
    CRGB tipCol = applyBloom(el, el.accentColor);

    for (uint8_t s = 0; s < cd.segmentCount; s++) {
      const CoralSegment& seg = cd.segments[s];

      int16_t px0 = el.x + seg.x0;
      int16_t py0 = el.y + seg.y0;
      int16_t px1 = el.x + seg.x1;
      int16_t py1 = el.y + seg.y1;

      // Color: lerp from base (depth 0) to accent (depth 2)
      uint8_t colorT = (seg.depth * 127);  // 0, 127, 254
      CRGB segColor = lerpColor(baseCol, tipCol, colorT);

      if (seg.depth == 0) {
        // Trunk: draw as 3 parallel lines for a thick, connected stem
        // This creates a visually continuous trunk from base to branch points
        float dx = (float)(px1 - px0);
        float dy = (float)(py1 - py0);
        float len = sqrt(dx * dx + dy * dy);
        if (len < 0.5f) len = 0.5f;
        // Perpendicular offset for thickness (1 pixel each side)
        int8_t offX = (int8_t)round(-dy / len);
        int8_t offY = (int8_t)round(dx / len);
        // Center line
        matrix->foreground->drawLine(px0, py0, px1, py1, segColor);
        // Offset lines for thickness
        matrix->foreground->drawLine(px0 + offX, py0 + offY, px1 + offX, py1 + offY, segColor);
        matrix->foreground->drawLine(px0 - offX, py0 - offY, px1 - offX, py1 - offY, segColor);
        // Small circle at base for grounding
        matrix->foreground->fillCircle(px0, py0, 1, segColor);
      } else if (seg.depth == 1) {
        // Mid branches: regular line connecting from trunk tip
        matrix->foreground->drawLine(px0, py0, px1, py1, segColor);
        // Small dot at tip for visual weight
        matrix->foreground->drawPixel(px1, py1, tipCol);
      } else {
        // Fine tips: single line, brighter
        matrix->foreground->drawLine(px0, py0, px1, py1, tipCol);
      }
    }
  }

  // Brain coral: cluster of overlapping circles with outline rings for texture
  // Layered fill + outline creates the bumpy, organic brain-like appearance
  void drawCoralBrain(const GroundElement& el) {
    const CoralData& cd = coralData[el.dataIndex];
    CRGB baseCol = applyBloom(el, el.baseColor);
    CRGB tipCol = applyBloom(el, el.accentColor);

    // First pass: fill all circles to create the solid mass
    for (uint8_t s = 0; s < cd.segmentCount; s++) {
      const CoralSegment& seg = cd.segments[s];
      int16_t cx = el.x + seg.x0;
      int16_t cy = el.y + seg.y0;
      uint8_t r = seg.x1;

      // Alternate colors between base and accent for mottled look
      uint8_t colorShift = (seg.depth * 73) & 0xFF;
      CRGB blobColor = lerpColor(baseCol, tipCol, colorShift);
      matrix->foreground->fillCircle(cx, cy, r, blobColor);
    }

    // Second pass: draw outline rings in a darker shade for brain-fold texture
    CRGB outlineCol = baseCol;
    outlineCol.nscale8(140);  // darker outline
    for (uint8_t s = 0; s < cd.segmentCount; s++) {
      const CoralSegment& seg = cd.segments[s];
      int16_t cx = el.x + seg.x0;
      int16_t cy = el.y + seg.y0;
      uint8_t r = seg.x1;
      if (r >= 2) {
        matrix->foreground->drawCircle(cx, cy, r, outlineCol);
      }
    }

    // Highlight spot on top of the cluster for dimension
    matrix->foreground->drawPixel(el.x, el.y - el.width + 1, tipCol);
  }

  // Fan coral: radiating rays from base with color gradient along each ray
  // Like a gorgonian sea fan — flat, spreading structure
  void drawCoralFan(const GroundElement& el) {
    const CoralData& cd = coralData[el.dataIndex];
    CRGB baseCol = applyBloom(el, el.baseColor);
    CRGB tipCol = applyBloom(el, el.accentColor);

    for (uint8_t s = 0; s < cd.segmentCount; s++) {
      const CoralSegment& seg = cd.segments[s];

      int16_t px0 = el.x + seg.x0;
      int16_t py0 = el.y + seg.y0;
      int16_t px1 = el.x + seg.x1;
      int16_t py1 = el.y + seg.y1;

      // Draw each ray as a pixel-by-pixel line with color gradient
      // base color at bottom, accent at tip
      int16_t dx = abs(px1 - px0);
      int16_t dy = abs(py1 - py0);
      int16_t steps = max(dx, dy);
      if (steps == 0) steps = 1;

      for (int16_t step = 0; step <= steps; step++) {
        float t = (float)step / (float)steps;
        int16_t px = px0 + (int16_t)((px1 - px0) * t);
        int16_t py = py0 + (int16_t)((py1 - py0) * t);
        uint8_t colorT = (uint8_t)(t * 255);
        CRGB pixColor = lerpColor(baseCol, tipCol, colorT);
        matrix->foreground->drawPixel(px, py, pixColor);
      }
    }

    // Draw a small base/stalk for grounding
    matrix->foreground->drawPixel(el.x, el.y, baseCol);
    matrix->foreground->drawPixel(el.x, el.y - 1, baseCol);
  }

  // ─── Other Drawing ───

  void drawSeagrass(const GroundElement& el) {
    float timeVal = millis() / 5000.0f;
    uint8_t drawHeight = (uint8_t)constrain(el.currentHeight, 1.0f, (float)GROUND_MAX_HEIGHT);

    for (uint8_t row = 0; row < drawHeight; row++) {
      float sway = sin(timeVal + el.phase) * (row * 0.3f);
      int16_t drawX = el.x + (int16_t)sway;
      int16_t drawY = el.y - row;

      CRGB color = el.baseColor;
      uint8_t brightScale = 180 + (row * 75 / drawHeight);
      color.nscale8(brightScale);

      matrix->foreground->drawPixel(drawX, drawY, color);
    }
  }

  void drawPlant(const GroundElement& el) {
    const PlantData& pd = plantData[el.dataIndex];
    float sizeFactor = el.currentHeight / 10.0f;
    float timeBase = millis() / 10000.0f;

    for (uint8_t b = 0; b < pd.branchCount; b++) {
      const BranchData& branch = pd.branches[b];
      float branchSin = sin(timeBase + branch.phaseOffset);
      float glowFactor = branchSin - 0.8f;

      int16_t n0x = (int16_t)(branch.nodes[0].x * sizeFactor) + el.x;
      int16_t n0y = (int16_t)(branch.nodes[0].y * sizeFactor) + el.y;
      matrix->foreground->drawLine(n0x, n0y, (int16_t)el.x, (int16_t)el.y, el.baseColor);

      for (uint8_t j = 1; j < branch.nodeCount; j++) {
        float sway = branchSin * (0.8f * j);

        int16_t prevX = (int16_t)(branch.nodes[j - 1].x * sizeFactor + sway) + el.x;
        int16_t prevY = (int16_t)(branch.nodes[j - 1].y * sizeFactor) + el.y;
        int16_t curX  = (int16_t)(branch.nodes[j].x * sizeFactor + sway) + el.x;
        int16_t curY  = (int16_t)(branch.nodes[j].y * sizeFactor) + el.y;

        matrix->foreground->drawLine(prevX, prevY, curX, curY, el.baseColor);

        if (j == branch.nodeCount - 1 && glowFactor > 0) {
          uint8_t intensity = (uint8_t)(glowFactor * 1000);
          CRGB flowerColor;
          hsv2rgb_rainbow(CHSV((uint8_t)pd.accentHue, 220, intensity), flowerColor);
          matrix->foreground->fillCircle(curX, curY, 1, flowerColor);
        }
      }
    }
  }

  void drawAnemone(const GroundElement& el) {
    const AnemoneData& ad = anemoneData[el.dataIndex];
    CRGB bodyColor = applyBloom(el, el.baseColor);

    int16_t baseX = el.x - el.width / 2;
    matrix->foreground->fillRect(baseX, el.y - 1, el.width, 2, bodyColor);

    float timeVal = millis() / 3000.0f;
    for (uint8_t t = 0; t < ad.tentacleCount; t++) {
      const TentacleData& tent = ad.tentacles[t];

      float pulse = sin(timeVal + tent.phaseOffset);
      uint8_t drawLength = tent.baseLength + (int8_t)(pulse);
      if (drawLength < 1) drawLength = 1;

      int16_t tentX = el.x + tent.xOffset;
      int16_t tentBaseY = el.y - 2;

      for (uint8_t p = 0; p < drawLength; p++) {
        int16_t py = tentBaseY - p;
        uint8_t blend = (p * 255) / drawLength;
        CRGB color = lerpColor(bodyColor, tent.tipColor, blend);
        matrix->foreground->drawPixel(tentX, py, color);
      }
    }
  }
};

#endif  // SEAFLOOR_H
