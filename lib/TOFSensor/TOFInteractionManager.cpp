#include "TOFInteractionManager.h"
#include <math.h>
#include <string.h>

namespace {

void insertionSortInt16(int16_t* a, int n) {
    for (int i = 1; i < n; i++) {
        int16_t key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

int16_t medianSorted(const int16_t* s, int len) {
    return s[(len - 1) / 2];
}

bool trimmedMedianDepthMm(int16_t* work, int n, float trimFraction, int16_t* out) {
    if (n < 6) return false;
    insertionSortInt16(work, n);
    int cut = 1;
    int fracCut = (int)(n * trimFraction);
    if (fracCut > cut) cut = fracCut;
    int sliceLen = n - 2 * cut;
    if (sliceLen < 3) return false;
    int mid = cut + (sliceLen - 1) / 2;
    *out = work[mid];
    return true;
}

int countInBandInTopRows(const int16_t grid[8][8], int16_t minD, int16_t maxD, int numRows) {
    if (numRows <= 0) return 0;
    int n = 0;
    int maxY = numRows < 8 ? numRows : 8;
    for (int y = 0; y < maxY; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > minD && d < maxD) n++;
        }
    }
    return n;
}

}  // namespace

TOFInteractionManager::TOFInteractionManager(TOFSensor* tofSensor)
    : sensor(tofSensor),
      minDistance(TOF_MIN_DETECTION_DIST),
      maxDistance(TOF_MAX_DETECTION_DIST),
      flicker_count(0),
      flicker_idx(0),
      last_processed_frame_id(0),
      analysis_ready(false),
      blob{},
      palm{},
      smooth_centroid_cx(0),
      smooth_centroid_cy(0),
      have_smoothed_centroid(false),
      smoothedVelocityX(0),
      smoothedVelocityY(0),
      lastUpdateTime(millis()),
      presenceStartTime(0),
      presenceActive(false),
      baseline_hand_raise_init(false),
      baseline_torso_row_ema(0),
      baseline_top_mass_ema(0),
      hand_raise_on_streak(0),
      hand_raise_off_streak(0),
      last_hand_raise_top_mass(0),
      last_hand_raise_mass_excess(0),
      palm_on_streak(0),
      palm_off_streak(0) {
    memset(analysis_grid, 0, sizeof(analysis_grid));
    memset(raw_grid, 0, sizeof(raw_grid));
    memset(flicker_hist, 0, sizeof(flicker_hist));
    memset(in_band_streak, 0, sizeof(in_band_streak));
}

void TOFInteractionManager::setDistanceRange(int16_t minDist, int16_t maxDist) {
    minDistance = minDist;
    maxDistance = maxDist;
}

void TOFInteractionManager::resetState() {
    memset(analysis_grid, 0, sizeof(analysis_grid));
    memset(raw_grid, 0, sizeof(raw_grid));
    memset(flicker_hist, 0, sizeof(flicker_hist));
    memset(in_band_streak, 0, sizeof(in_band_streak));
    flicker_count = 0;
    flicker_idx = 0;
    analysis_ready = false;
    last_processed_frame_id = 0;

    blob = {};
    palm = {};
    palm_on_streak = 0;
    palm_off_streak = 0;

    have_smoothed_centroid = false;
    smooth_centroid_cx = 0;
    smooth_centroid_cy = 0;
    smoothedVelocityX = 0;
    smoothedVelocityY = 0;
    presenceStartTime = 0;
    presenceActive = false;
    lastUpdateTime = millis();

    baseline_hand_raise_init = false;
    baseline_torso_row_ema = 0;
    baseline_top_mass_ema = 0;
    hand_raise_on_streak = 0;
    hand_raise_off_streak = 0;
    last_hand_raise_top_mass = 0;
    last_hand_raise_mass_excess = 0;

    data = {};
}

void TOFInteractionManager::calibrateBaseline() {
    // Keep API compatibility with Aquarium; this now resets analysis state.
    resetState();
}

void TOFInteractionManager::sampleLiveDistances(int16_t grid[8][8]) {
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            int16_t d = sensor->getDistance(x, y);
            grid[y][x] = (d < 0) ? 0 : d;
        }
    }
}

void TOFInteractionManager::pushFlickerAndBuildAnalysis(const int16_t raw[8][8]) {
    memcpy(flicker_hist[flicker_idx], raw, sizeof(flicker_hist[0]));
    flicker_idx = (flicker_idx + 1) % kFlickerHistoryLen;
    if (flicker_count < kFlickerHistoryLen) flicker_count++;

    if (!kUseFlickerFilter) {
        memcpy(analysis_grid, raw, sizeof(analysis_grid));
        analysis_ready = true;
        return;
    }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t samples[3];
            int ns = 0;
            for (uint8_t h = 0; h < flicker_count; h++) {
                uint8_t idx = (uint8_t)((flicker_idx + kFlickerHistoryLen - 1 - h) % kFlickerHistoryLen);
                samples[ns++] = flicker_hist[idx][y][x];
            }
            insertionSortInt16(samples, ns);
            analysis_grid[y][x] = samples[ns / 2];
        }
    }
    analysis_ready = true;
}

void TOFInteractionManager::advanceInBandStreaks(const int16_t grid[8][8]) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > minDistance && d < maxDistance) {
                uint8_t s = in_band_streak[y][x] + 1;
                in_band_streak[y][x] = s > kInBandStreakCap ? kInBandStreakCap : s;
            } else {
                in_band_streak[y][x] = 0;
            }
        }
    }
}

void TOFInteractionManager::findLargestBlobCentroid(const int16_t grid[8][8], BlobInfo* out) {
    out->valid = false;
    out->count = 0;
    bool visited[8][8];
    memset(visited, 0, sizeof(visited));

    const bool needStreak = kUseStableCentroidGate && kStableFramesRequired > 0;
    const uint8_t streakMin = needStreak ? kStableFramesRequired : 0;

    auto active = [&](int x, int y) -> bool {
        int16_t d = grid[y][x];
        if (!(d > minDistance && d < maxDistance)) return false;
        if (needStreak && in_band_streak[y][x] < streakMin) return false;
        return true;
    };

    int bestCount = 0;
    float bestCx = 0, bestCy = 0;

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (visited[y][x] || !active(x, y)) continue;

            int count = 0;
            double sumX = 0, sumY = 0, sumW = 0;
            uint8_t qx[64], qy[64];
            int qt = 0;
            qx[qt] = (uint8_t)x;
            qy[qt] = (uint8_t)y;
            qt++;
            visited[y][x] = true;

            for (int qh = 0; qh < qt; qh++) {
                int cx = qx[qh];
                int cy = qy[qh];
                count++;
                int16_t d = grid[cy][cx];
                float w = (d > 0) ? (1.0f / (float)(d > 1 ? d : 1)) : 1.0f;
                sumX += (cx + 0.5f) * w;
                sumY += (cy + 0.5f) * w;
                sumW += w;

                const int dxs[4] = {1, -1, 0, 0};
                const int dys[4] = {0, 0, 1, -1};
                for (int k = 0; k < 4; k++) {
                    int nx = cx + dxs[k];
                    int ny = cy + dys[k];
                    if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) continue;
                    if (visited[ny][nx] || !active(nx, ny)) continue;
                    visited[ny][nx] = true;
                    qx[qt] = (uint8_t)nx;
                    qy[qt] = (uint8_t)ny;
                    qt++;
                }
            }

            if (count >= kBlobMinCells && count > bestCount) {
                bestCount = count;
                bestCx = (float)(sumX / sumW);
                bestCy = (float)(sumY / sumW);
            }
        }
    }

    if (bestCount >= kBlobMinCells) {
        out->valid = true;
        out->cx = bestCx;
        out->cy = bestCy;
        out->count = bestCount;
    }
}

void TOFInteractionManager::updateSmoothedCentroidFromBlob(const BlobInfo& b) {
    if (b.valid) {
        if (!have_smoothed_centroid) {
            smooth_centroid_cx = b.cx;
            smooth_centroid_cy = b.cy;
            have_smoothed_centroid = true;
        } else {
            smooth_centroid_cx += (b.cx - smooth_centroid_cx) * kCentroidSmooth;
            smooth_centroid_cy += (b.cy - smooth_centroid_cy) * kCentroidSmooth;
        }
    } else {
        have_smoothed_centroid = false;
    }
}

void TOFInteractionManager::stepDistanceHint(const int16_t grid[8][8]) {
    int16_t vals[64];
    int nv = 0;
    int inBand = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > kValidDepthMinMm && d < kValidDepthMaxMm) vals[nv++] = d;
            if (d > minDistance && d < maxDistance) inBand++;
        }
    }

    if (nv < kStanceMinValidCells || inBand < kStanceMinInBandCells) {
        data.distanceHint = TofDistanceHint::NoPerson;
        if (nv > 0) {
            insertionSortInt16(vals, nv);
            data.distanceMedianMm = medianSorted(vals, nv);
        } else {
            data.distanceMedianMm = 0;
        }
        data.stanceDepthMm = 0;
        return;
    }

    int16_t stance = 0;
    if (!trimmedMedianDepthMm(vals, nv, kTrimmedMedianFraction, &stance)) {
        data.distanceHint = TofDistanceHint::NoPerson;
        insertionSortInt16(vals, nv);
        data.distanceMedianMm = medianSorted(vals, nv);
        data.stanceDepthMm = 0;
        return;
    }

    data.stanceDepthMm = stance;
    data.distanceMedianMm = stance;
    int32_t optimum = ((int32_t)minDistance + (int32_t)maxDistance) / 2;
    int16_t slop = kDistanceCenterSlopMm > 50 ? kDistanceCenterSlopMm : 50;

    if (stance < minDistance) {
        data.distanceHint = TofDistanceHint::TooClose;
        return;
    }
    if (stance > maxDistance) {
        data.distanceHint = TofDistanceHint::TooFar;
        return;
    }
    if (stance < optimum - slop) {
        data.distanceHint = TofDistanceHint::TooClose;
        return;
    }
    if (stance > optimum + slop) {
        data.distanceHint = TofDistanceHint::TooFar;
        return;
    }
    data.distanceHint = TofDistanceHint::Ok;
}

bool TOFInteractionManager::interactionBandOk() const {
    return data.distanceHint != TofDistanceHint::NoPerson && data.stanceDepthMm > 0 &&
           data.stanceDepthMm >= minDistance && data.stanceDepthMm <= maxDistance;
}

void TOFInteractionManager::detectPalmOutlier(const int16_t grid[8][8]) {
    PalmInfo candidate{};
    candidate.valid = false;
    int16_t valid[64];
    int nv = 0;
    int16_t lo = kValidDepthMaxMm;
    int16_t hi = 0;

    auto clearPalmDebounced = [&]() {
        palm_on_streak = 0;
        palm_off_streak++;
        if (palm_off_streak >= kPalmOffFrames) {
            palm.valid = false;
        }
    };

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > kValidDepthMinMm && d < kValidDepthMaxMm) {
                valid[nv++] = d;
                if (d < lo) lo = d;
                if (d > hi) hi = d;
            }
        }
    }

    if (nv < kPalmMinValidCells || (hi - lo) < kPalmMinDepthSpreadMm) {
        clearPalmDebounced();
        return;
    }

    if (data.distanceHint == TofDistanceHint::NoPerson || data.stanceDepthMm <= 0) {
        clearPalmDebounced();
        return;
    }

    int16_t ref = data.stanceDepthMm;
    if (ref < kPalmRefMinMm || ref > kPalmRefMaxMm) {
        clearPalmDebounced();
        return;
    }

    int anchors = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > kValidDepthMinMm && d < kValidDepthMaxMm && abs(d - ref) <= kPalmBodyAnchorWindowMm) {
                anchors++;
            }
        }
    }
    if (anchors < kPalmMinBodyAnchorCells) {
        clearPalmDebounced();
        return;
    }

    int bestDelta = -1;
    int bestX = 0, bestY = 0;
    int16_t bestD = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (!d || d >= ref) continue;
            int delta = (int)ref - (int)d;
            if (delta < kPalmDeltaMm) continue;
            if (delta > bestDelta) {
                bestDelta = delta;
                bestX = x;
                bestY = y;
                bestD = d;
            }
        }
    }

    if (bestDelta >= kPalmDeltaMm) {
        int nCount = 0;
        int nSum = 0;
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4; i++) {
            int nx = bestX + dx[i];
            int ny = bestY + dy[i];
            if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) continue;
            int16_t nd = grid[ny][nx];
            if (nd > kValidDepthMinMm && nd < kValidDepthMaxMm) {
                nSum += nd;
                nCount++;
            }
        }
        if (nCount > 0) {
            int nAvg = nSum / nCount;
            if ((nAvg - bestD) < kPalmLocalContrastMm) bestDelta = -1;
        }
    }

    if (bestDelta >= kPalmDeltaMm) {
        candidate.valid = true;
        candidate.x = (uint8_t)bestX;
        candidate.y = (uint8_t)bestY;
        candidate.depth = bestD;
        candidate.delta_mm = (int16_t)bestDelta;
        candidate.ref_mm = ref;
    }

    if (candidate.valid) {
        palm_on_streak++;
        palm_off_streak = 0;
        if (palm_on_streak >= kPalmOnFrames) {
            palm = candidate;
            palm.valid = true;
        }
    } else {
        clearPalmDebounced();
    }
}

void TOFInteractionManager::stepHandRaise(const int16_t grid[8][8], const BlobInfo& b) {
    if (!kHandRaiseEnabled) {
        data.handsRaised = false;
        hand_raise_on_streak = 0;
        return;
    }

    if (!grid || !b.valid) {
        hand_raise_on_streak = 0;
        hand_raise_off_streak++;
        if (hand_raise_off_streak >= kHandRaiseOffFrames) data.handsRaised = false;
        return;
    }

    int topMass = countInBandInTopRows(grid, minDistance, maxDistance, kHandRaiseTopRows);
    float cy = b.cy;

    if (!baseline_hand_raise_init) {
        baseline_torso_row_ema = cy;
        baseline_top_mass_ema = (float)topMass;
        baseline_hand_raise_init = true;
    }

    float massExcess = (float)topMass - baseline_top_mass_ema;
    last_hand_raise_top_mass = topMass;
    last_hand_raise_mass_excess = massExcess;

    bool learnBaseline =
        data.distanceHint == TofDistanceHint::Ok && !data.handsRaised && massExcess < kHandRaiseMassExcess * 0.4f;
    if (learnBaseline) {
        baseline_torso_row_ema += (cy - baseline_torso_row_ema) * kHandRaiseBaselineLerp;
        baseline_top_mass_ema += ((float)topMass - baseline_top_mass_ema) * kHandRaiseBaselineLerp;
    }

    bool massHit = (massExcess >= kHandRaiseMassExcess) && interactionBandOk();
    if (massHit) {
        hand_raise_on_streak++;
        hand_raise_off_streak = 0;
    } else {
        hand_raise_on_streak = 0;
        hand_raise_off_streak++;
    }

    if (!data.handsRaised && hand_raise_on_streak >= kHandRaiseOnFrames) data.handsRaised = true;
    if (data.handsRaised && hand_raise_off_streak >= kHandRaiseOffFrames) data.handsRaised = false;
}

void TOFInteractionManager::updateVelocityAndPresence() {
    unsigned long now = millis();
    float dt = (now - lastUpdateTime) / 1000.0f;
    if (dt <= 0.0f || dt > 1.0f) dt = 0.033f;
    lastUpdateTime = now;

    if (have_smoothed_centroid) {
        float bx = smooth_centroid_cx / 8.0f;
        float by = smooth_centroid_cy / 8.0f;
        bx = constrain(bx, 0.0f, 1.0f);
        by = constrain(by, 0.0f, 1.0f);

        float dx = bx - data.blobX;
        float dy = by - data.blobY;
        float instVx = dx / dt;
        float instVy = dy / dt;
        constexpr float vSmooth = 0.30f;
        smoothedVelocityX += (instVx - smoothedVelocityX) * vSmooth;
        smoothedVelocityY += (instVy - smoothedVelocityY) * vSmooth;

        data.hasBlob = true;
        data.blobX = bx;
        data.blobY = by;
        data.blobSize = blob.valid ? (uint8_t)blob.count : 0;

        if (!presenceActive) {
            presenceActive = true;
            presenceStartTime = now;
        }
        data.presenceDuration = (now - presenceStartTime) / 1000.0f;
        data.velocityX = smoothedVelocityX;
        data.velocityY = smoothedVelocityY;
        data.velocityMag = sqrtf(smoothedVelocityX * smoothedVelocityX + smoothedVelocityY * smoothedVelocityY);
    } else {
        data.hasBlob = false;
        data.blobSize = 0;
        data.velocityX = 0;
        data.velocityY = 0;
        data.velocityMag = 0;
        data.presenceDuration = 0;
        presenceActive = false;
    }
}

void TOFInteractionManager::update() {
    if (!sensor || !sensor->isActive()) {
        resetState();
        return;
    }

    uint32_t fid = sensor->getRangingFrameId();
    if (fid == 0 || fid == last_processed_frame_id) return;
    last_processed_frame_id = fid;

    sampleLiveDistances(raw_grid);
    pushFlickerAndBuildAnalysis(raw_grid);
    advanceInBandStreaks(analysis_grid);
    findLargestBlobCentroid(analysis_grid, &blob);
    updateSmoothedCentroidFromBlob(blob);
    stepDistanceHint(analysis_grid);
    detectPalmOutlier(analysis_grid);
    stepHandRaise(analysis_grid, blob);
    updateVelocityAndPresence();

    // publish maps/derived fields
    memcpy(data.depthMap, analysis_grid, sizeof(data.depthMap));
    data.hasPalm = palm.valid;
    data.palmX = palm.x;
    data.palmY = palm.y;
    data.palmDepth = palm.depth;
    data.palmDeltaMm = palm.delta_mm;
    data.palmRefMm = palm.ref_mm;
    data.handRaiseTopMass = last_hand_raise_top_mass;
    data.handRaiseMassExcess = last_hand_raise_mass_excess;
}
