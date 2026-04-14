#pragma once

#include <Arduino.h>
#include "TOFSensor.h"

enum class TofDistanceHint : uint8_t { NoPerson, TooClose, TooFar, Ok };

struct InteractionData {
    bool hasBlob = false;
    float blobX = 0.5f;   // Normalized 0..1
    float blobY = 0.5f;   // Normalized 0..1
    uint8_t blobSize = 0;
    float velocityMag = 0;
    float velocityX = 0;
    float velocityY = 0;
    float presenceDuration = 0;
    int16_t depthMap[8][8] = {};

    bool hasPalm = false;
    uint8_t palmX = 0;
    uint8_t palmY = 0;
    int16_t palmDepth = 0;
    int16_t palmDeltaMm = 0;
    int16_t palmRefMm = 0;

    bool handsRaised = false;
    int handRaiseTopMass = 0;
    float handRaiseMassExcess = 0;

    int16_t stanceDepthMm = 0;
    int16_t distanceMedianMm = 0;
    TofDistanceHint distanceHint = TofDistanceHint::NoPerson;
};

class TOFInteractionManager {
private:
    TOFSensor* sensor;
    InteractionData data;

    int16_t minDistance;
    int16_t maxDistance;

    // Tunables (migrated from visualizer pipeline)
    static constexpr bool kUseFlickerFilter = true;
    static constexpr uint8_t kFlickerHistoryLen = 3;
    static constexpr bool kUseStableCentroidGate = true;
    static constexpr uint8_t kStableFramesRequired = 3;
    static constexpr int16_t kPalmDeltaMm = 400;
    static constexpr int16_t kPalmLocalContrastMm = 120;
    static constexpr int16_t kDistanceCenterSlopMm = 300;
    static constexpr int kPalmMinValidCells = 8;
    static constexpr int kPalmMinDepthSpreadMm = 80;
    static constexpr int kPalmMinBodyAnchorCells = 4;
    static constexpr int16_t kPalmBodyAnchorWindowMm = 220;
    static constexpr uint8_t kPalmOnFrames = 2;
    static constexpr uint8_t kPalmOffFrames = 2;
    static constexpr int kBlobMinCells = 3;
    static constexpr bool kHandRaiseEnabled = true;
    // Depth (cells) of the raised-hand band along physical up (left columns; see TOFInteractionManager.cpp).
    static constexpr int kHandRaiseTopRows = 2;
    static constexpr float kHandRaiseMassExcess = 1.25f;
    static constexpr uint8_t kHandRaiseOnFrames = 2;
    static constexpr uint8_t kHandRaiseOffFrames = 5;
    static constexpr float kHandRaiseBaselineLerp = 0.12f;
    static constexpr float kCentroidSmooth = 0.38f;
    static constexpr float kTrimmedMedianFraction = 0.15f;
    static constexpr int kStanceMinValidCells = 8;
    static constexpr int kStanceMinInBandCells = 4;
    static constexpr int16_t kValidDepthMinMm = 1;
    static constexpr int16_t kValidDepthMaxMm = 4000;
    static constexpr int16_t kPalmRefMinMm = 120;
    static constexpr int16_t kPalmRefMaxMm = 3800;
    static constexpr uint8_t kInBandStreakCap = 120;

    int16_t analysis_grid[8][8];
    int16_t raw_grid[8][8];
    int16_t flicker_hist[3][8][8];
    uint8_t flicker_count;
    uint8_t flicker_idx;
    uint8_t in_band_streak[8][8];
    uint32_t last_processed_frame_id;
    bool analysis_ready;

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

    float smooth_centroid_cx;
    float smooth_centroid_cy;
    bool have_smoothed_centroid;
    float smoothedVelocityX;
    float smoothedVelocityY;
    unsigned long lastUpdateTime;
    unsigned long presenceStartTime;
    bool presenceActive;

    bool baseline_hand_raise_init;
    float baseline_torso_row_ema;
    float baseline_top_mass_ema;
    uint8_t hand_raise_on_streak;
    uint8_t hand_raise_off_streak;
    int last_hand_raise_top_mass;
    float last_hand_raise_mass_excess;

    uint8_t palm_on_streak;
    uint8_t palm_off_streak;

    void resetState();
    void sampleLiveDistances(int16_t grid[8][8]);
    void pushFlickerAndBuildAnalysis(const int16_t raw[8][8]);
    void advanceInBandStreaks(const int16_t grid[8][8]);
    void findLargestBlobCentroid(const int16_t grid[8][8], BlobInfo* out_blob);
    void updateSmoothedCentroidFromBlob(const BlobInfo& b);
    void stepDistanceHint(const int16_t grid[8][8]);
    void detectPalmOutlier(const int16_t grid[8][8]);
    void stepHandRaise(const int16_t grid[8][8], const BlobInfo& b);
    void updateVelocityAndPresence();
    bool interactionBandOk() const;

public:
    TOFInteractionManager(TOFSensor* tofSensor);

    void update();
    InteractionData getInteractionData() const { return data; }
    void setDistanceRange(int16_t minDist, int16_t maxDist);
    void calibrateBaseline();
    bool isBaselineReady() const { return analysis_ready; }
};
