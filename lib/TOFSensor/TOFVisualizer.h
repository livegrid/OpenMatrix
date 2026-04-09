#pragma once

#include <Arduino.h>
#include "TOFSensor.h"
#include "../Matrix/Matrix.h"

enum class TofDistanceHint : uint8_t { NoPerson, TooClose, TooFar, Ok };

class TOFVisualizer {
private:
    TOFSensor* sensor;
    Matrix* matrix;

    // Active distance band (mm): used for blob, heatmap masking, in-band streaks, stance "in range"
    // checks, and hand-raise min/max. Defaults come from TOF_MIN_DETECTION_DIST / TOF_MAX_DETECTION_DIST
    // in TOFSensor.h; main.cpp may also call setDistanceRange().
    int16_t minDistance;
    int16_t maxDistance;

    // -------------------------------------------------------------------------
    // Tweakable parameters (p5js lab equivalents — no web UI, edit here)
    // -------------------------------------------------------------------------

    // If true: each cell is median of last kFlickerHistoryLen frames (less sparkle, more lag).
    // If false: use raw grid for analysis/draw.
    static constexpr bool kUseFlickerFilter = true;

    // How many frames to keep for flicker median (only used when kUseFlickerFilter is true).
    static constexpr uint8_t kFlickerHistoryLen = 3;

    // Heatmap: dim cells whose depth is NOT strictly between minDistance and maxDistance.
    static constexpr bool kHideOutsideActiveBand = true;

    // If true: a cell only counts toward the centroid blob after staying in-band for
    // kStableFramesRequired consecutive sensor frames (reduces flicker).
    static constexpr bool kUseStableCentroidGate = true;

    // Streak length required when stable centroid gate is on (see advanceInBandStreaks).
    static constexpr uint8_t kStableFramesRequired = 3;

    // Palm: minimum (ref − cellDepth) in mm; ref = trimmed median of valid depths. Higher = stricter
    // (needs a clearer "reach" toward the sensor). Lower = easier palm triggers.
    static constexpr int16_t kPalmDeltaMm = 400;
    // Palm candidate must also be this much closer than local neighbors (anti-noise).
    static constexpr int16_t kPalmLocalContrastMm = 120;

    // Stance "sweet spot": optimum is (minDistance + maxDistance) / 2. If stance is inside the band
    // but farther than kOff from that midpoint, hint becomes Step back / Step forward (see stepDistanceHint).
    // Also floored at 50 mm inside the implementation.
    static constexpr int16_t kDistanceCenterSlopMm = 300;

    // Palm pre-gate: need at least this many cells with 0 < depth < 4000 mm on the grid.
    static constexpr int kPalmMinValidCells = 8;

    // Palm pre-gate: max(depth) − min(depth) across valid cells must be >= this (scene has depth variety).
    static constexpr int kPalmMinDepthSpreadMm = 80;
    // Need this many body-anchor cells near stance depth before palm can trigger.
    static constexpr int kPalmMinBodyAnchorCells = 4;
    // "Near stance" window for body-anchor count.
    static constexpr int16_t kPalmBodyAnchorWindowMm = 220;

    // Palm output debounce: need this many consecutive good frames to show palm; this many bad to clear.
    static constexpr uint8_t kPalmOnFrames = 2;
    static constexpr uint8_t kPalmOffFrames = 2;

    // Largest blob: connected in-band (and streak-gated) component must have at least this many cells.
    static constexpr int kBlobMinCells = 3;

    // Hands raised: compare in-band mass in top kHandRaiseTopRows rows to a learned baseline.
    static constexpr bool kHandRaiseEnabled = true;

    // Number of sensor rows from y=0 (top of analysis grid) used as "hand raise" region.
    static constexpr int kHandRaiseTopRows = 2;

    // How many extra in-band cells (vs baseline EMA) in the top rows counts as "raised".
    static constexpr float kHandRaiseMassExcess = 1.25f;

    // Debounce: frames of mass excess before hands "on"; frames without before "off".
    static constexpr uint8_t kHandRaiseOnFrames = 2;
    static constexpr uint8_t kHandRaiseOffFrames = 5;

    // How fast baseline learns when subject is stable (arms down, stance ok). 0..1, higher = faster adapt.
    static constexpr float kHandRaiseBaselineLerp = 0.12f;

    // Smoothed centroid EMA blend toward raw blob centroid each frame (0..1). Higher = snappier.
    static constexpr float kCentroidSmooth = 0.38f;

    // Text layout: approximate height in pixels for the bottom prompt lines in drawStatusBar().
    static constexpr uint16_t kStatusBarPixels = 14;

    // Trimmed median: fraction clipped off each tail before taking median (stance + palm ref).
    static constexpr float kTrimmedMedianFraction = 0.15f;

    // stepDistanceHint: treat as "no person" if fewer valid depths or too few cells in min..max band.
    static constexpr int kStanceMinValidCells = 8;
    static constexpr int kStanceMinInBandCells = 4;

    // Valid range for depth samples when gathering median / palm (matches VL53L8CX-ish reporting).
    static constexpr int16_t kValidDepthMinMm = 1;
    static constexpr int16_t kValidDepthMaxMm = 4000;

    // Palm: trimmed median ref must fall in this range or detection bails for that frame.
    static constexpr int16_t kPalmRefMinMm = 120;
    static constexpr int16_t kPalmRefMaxMm = 3800;

    // Per-cell in-band streak saturates at this value (prevents overflow in long holds).
    static constexpr uint8_t kInBandStreakCap = 120;

    // Per-frame analysis cache (8x8 grid row-major [y][x])
    int16_t analysis_grid[8][8];
    int16_t flicker_hist[3][8][8];
    uint8_t flicker_count;
    uint8_t flicker_idx;
    uint8_t in_band_streak[8][8];
    uint32_t last_processed_frame_id;
    bool analysis_ready;

    float smooth_centroid_cx;
    float smooth_centroid_cy;
    bool have_smoothed_centroid;

    TofDistanceHint distance_hint;
    int16_t last_stance_depth_mm;
    int16_t distance_median_mm;

    struct BlobInfo {
        float cx;
        float cy;
        int count;
        bool valid;
    } blob;

    struct PalmInfo {
        uint8_t x;
        uint8_t y;
        int16_t depth;
        int16_t delta_mm;
        int16_t ref_mm;
        bool valid;
    } palm;
    uint8_t palm_on_streak;
    uint8_t palm_off_streak;

    bool baseline_hand_raise_init;
    float baseline_torso_row_ema;
    float baseline_top_mass_ema;
    uint8_t hand_raise_on_streak;
    uint8_t hand_raise_off_streak;
    bool hands_raised_active;
    int last_hand_raise_top_mass;
    float last_hand_raise_mass_excess;

    void resetInteractionState();
    void sampleLiveDistances(int16_t grid[8][8]);
    void processPipelineForNewFrame();
    void pushFlickerAndBuildAnalysis(const int16_t raw[8][8]);
    void advanceInBandStreaks(const int16_t grid[8][8]);
    void findLargestBlobCentroid(const int16_t grid[8][8], BlobInfo* out_blob);
    void updateSmoothedCentroidFromBlob(const BlobInfo& b);
    void stepDistanceHint(const int16_t grid[8][8]);
    void detectPalmOutlier(const int16_t grid[8][8]);
    void stepHandRaise(const int16_t grid[8][8], const BlobInfo& b);
    bool interactionBandOk() const;

    uint16_t distanceToColor(int16_t distance);
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

    void drawHeatmapAndOverlays(uint16_t plotW, uint16_t plotH);
    void drawStatusBar();

public:
    TOFVisualizer(TOFSensor* tofSensor, Matrix* matrixDisplay = nullptr);

    void setMatrix(Matrix* matrixDisplay);

    void setDistanceRange(int16_t minDist, int16_t maxDist);

    void draw();

    void drawWithBlockSize(uint8_t blockSize);

    void drawWithBlockSizes(uint8_t blockSizeX, uint8_t blockSizeY);
};
