#include "TOFVisualizer.h"
#include <Fonts/Font4x5Fixed.h>
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
    if (n < 6) {
        return false;
    }
    insertionSortInt16(work, n);
    int cut = 1;
    if ((int)(n * trimFraction) > cut) {
        cut = (int)(n * trimFraction);
    }
    int sliceLen = n - 2 * cut;
    if (sliceLen < 3) {
        return false;
    }
    int mid = cut + (sliceLen - 1) / 2;
    *out = work[mid];
    return true;
}

int countInBandInTopRows(const int16_t grid[8][8], int16_t minD, int16_t maxD, int numRows) {
    if (numRows <= 0) {
        return 0;
    }
    int n = 0;
    int maxY = numRows < 8 ? numRows : 8;
    for (int y = 0; y < maxY; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > minD && d < maxD) {
                n++;
            }
        }
    }
    return n;
}

}  // namespace

TOFVisualizer::TOFVisualizer(TOFSensor* tofSensor, Matrix* matrixDisplay)
    : sensor(tofSensor),
      matrix(matrixDisplay),
      minDistance(TOF_MIN_DETECTION_DIST),
      maxDistance(TOF_MAX_DETECTION_DIST),
      flicker_count(0),
      flicker_idx(0),
      last_processed_frame_id(0),
      analysis_ready(false),
      smooth_centroid_cx(0),
      smooth_centroid_cy(0),
      have_smoothed_centroid(false),
      distance_hint(TofDistanceHint::NoPerson),
      last_stance_depth_mm(0),
      distance_median_mm(0),
      blob{},
      palm{},
      baseline_hand_raise_init(false),
      baseline_torso_row_ema(0),
      baseline_top_mass_ema(0),
      hand_raise_on_streak(0),
      hand_raise_off_streak(0),
      hands_raised_active(false),
      last_hand_raise_top_mass(0),
      last_hand_raise_mass_excess(0),
      palm_on_streak(0),
      palm_off_streak(0) {
    memset(analysis_grid, 0, sizeof(analysis_grid));
    memset(flicker_hist, 0, sizeof(flicker_hist));
    memset(in_band_streak, 0, sizeof(in_band_streak));
}

void TOFVisualizer::setMatrix(Matrix* matrixDisplay) {
    matrix = matrixDisplay;
}

void TOFVisualizer::setDistanceRange(int16_t minDist, int16_t maxDist) {
    minDistance = minDist;
    maxDistance = maxDist;
}

void TOFVisualizer::resetInteractionState() {
    memset(in_band_streak, 0, sizeof(in_band_streak));
    have_smoothed_centroid = false;
    smooth_centroid_cx = smooth_centroid_cy = 0;
    baseline_hand_raise_init = false;
    hand_raise_on_streak = hand_raise_off_streak = 0;
    hands_raised_active = false;
    last_hand_raise_top_mass = 0;
    last_hand_raise_mass_excess = 0;
    distance_hint = TofDistanceHint::NoPerson;
    distance_median_mm = last_stance_depth_mm = 0;
    blob.valid = false;
    palm.valid = false;
    palm_on_streak = 0;
    palm_off_streak = 0;
}

void TOFVisualizer::sampleLiveDistances(int16_t grid[8][8]) {
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            int16_t d = sensor->getDistance(x, y);
            grid[y][x] = (d < 0) ? 0 : d;
        }
    }
}

void TOFVisualizer::pushFlickerAndBuildAnalysis(const int16_t raw[8][8]) {
    memcpy(flicker_hist[flicker_idx], raw, sizeof(flicker_hist[0]));
    flicker_idx = (flicker_idx + 1) % kFlickerHistoryLen;
    if (flicker_count < kFlickerHistoryLen) {
        flicker_count++;
    }

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

void TOFVisualizer::advanceInBandStreaks(const int16_t grid[8][8]) {
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

void TOFVisualizer::findLargestBlobCentroid(const int16_t grid[8][8], BlobInfo* out) {
    out->valid = false;
    out->count = 0;
    bool visited[8][8];
    memset(visited, 0, sizeof(visited));

    const bool needStreak = kUseStableCentroidGate && kStableFramesRequired > 0;
    const uint8_t streakMin = needStreak ? kStableFramesRequired : 0;

    auto active = [&](int x, int y) -> bool {
        int16_t d = grid[y][x];
        if (!(d > minDistance && d < maxDistance)) {
            return false;
        }
        if (needStreak && in_band_streak[y][x] < streakMin) {
            return false;
        }
        return true;
    };

    int bestCount = 0;
    float bestCx = 0, bestCy = 0;

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (visited[y][x] || !active(x, y)) {
                continue;
            }

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

                const int dxs[] = {1, -1, 0, 0};
                const int dys[] = {0, 0, 1, -1};
                for (int k = 0; k < 4; k++) {
                    int nx = cx + dxs[k];
                    int ny = cy + dys[k];
                    if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) {
                        continue;
                    }
                    if (visited[ny][nx] || !active(nx, ny)) {
                        continue;
                    }
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

void TOFVisualizer::updateSmoothedCentroidFromBlob(const BlobInfo& b) {
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

void TOFVisualizer::stepDistanceHint(const int16_t grid[8][8]) {
    int16_t vals[64];
    int nv = 0;
    int inBand = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int16_t d = grid[y][x];
            if (d > kValidDepthMinMm && d < kValidDepthMaxMm) {
                vals[nv++] = d;
            }
            if (d > minDistance && d < maxDistance) {
                inBand++;
            }
        }
    }

    if (nv < kStanceMinValidCells || inBand < kStanceMinInBandCells) {
        distance_hint = TofDistanceHint::NoPerson;
        if (nv > 0) {
            insertionSortInt16(vals, nv);
            distance_median_mm = medianSorted(vals, nv);
        } else {
            distance_median_mm = 0;
        }
        last_stance_depth_mm = 0;
        return;
    }

    int16_t stance = 0;
    if (!trimmedMedianDepthMm(vals, nv, kTrimmedMedianFraction, &stance)) {
        distance_hint = TofDistanceHint::NoPerson;
        insertionSortInt16(vals, nv);
        distance_median_mm = medianSorted(vals, nv);
        last_stance_depth_mm = 0;
        return;
    }

    last_stance_depth_mm = stance;
    distance_median_mm = stance;

    int32_t optimum = ((int32_t)minDistance + (int32_t)maxDistance) / 2;
    int16_t slop = kDistanceCenterSlopMm > 50 ? kDistanceCenterSlopMm : 50;

    if (stance < minDistance) {
        distance_hint = TofDistanceHint::TooClose;
        return;
    }
    if (stance > maxDistance) {
        distance_hint = TofDistanceHint::TooFar;
        return;
    }
    if (stance < optimum - slop) {
        distance_hint = TofDistanceHint::TooClose;
        return;
    }
    if (stance > optimum + slop) {
        distance_hint = TofDistanceHint::TooFar;
        return;
    }
    distance_hint = TofDistanceHint::Ok;
}

bool TOFVisualizer::interactionBandOk() const {
    return distance_hint != TofDistanceHint::NoPerson && last_stance_depth_mm > 0 &&
           last_stance_depth_mm >= minDistance && last_stance_depth_mm <= maxDistance;
}

void TOFVisualizer::detectPalmOutlier(const int16_t grid[8][8]) {
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

    // Must see a real scene first.
    if (nv < kPalmMinValidCells || (hi - lo) < kPalmMinDepthSpreadMm) {
        clearPalmDebounced();
        return;
    }

    // Person-present gate, but NOT optimum-distance gate:
    // allow TooClose / TooFar / Ok, block only NoPerson.
    if (distance_hint == TofDistanceHint::NoPerson || last_stance_depth_mm <= 0) {
        clearPalmDebounced();
        return;
    }

    // Use stance depth as the palm reference to avoid background-skewed ref values.
    int16_t ref = last_stance_depth_mm;
    if (ref < kPalmRefMinMm || ref > kPalmRefMaxMm) {
        clearPalmDebounced();
        return;
    }

    // Need enough cells near stance depth to prove there's a body in frame.
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
            if (!d || d >= ref) {
                continue;
            }
            int delta = (int)ref - (int)d;
            if (delta < kPalmDeltaMm) {
                continue;
            }
            if (delta > bestDelta) {
                bestDelta = delta;
                bestX = x;
                bestY = y;
                bestD = d;
            }
        }
    }

    // Extra local-contrast check vs 4-neighbors to reject isolated noise spikes.
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
            if ((nAvg - bestD) < kPalmLocalContrastMm) {
                bestDelta = -1;
            }
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

void TOFVisualizer::stepHandRaise(const int16_t grid[8][8], const BlobInfo& b) {
    if (!kHandRaiseEnabled) {
        hands_raised_active = false;
        hand_raise_on_streak = 0;
        return;
    }

    if (!b.valid) {
        hand_raise_on_streak = 0;
        hand_raise_off_streak++;
        if (hand_raise_off_streak >= kHandRaiseOffFrames) {
            hands_raised_active = false;
        }
        return;
    }

    int topMass =
        countInBandInTopRows(grid, minDistance, maxDistance, kHandRaiseTopRows);
    float cy = b.cy;

    if (!baseline_hand_raise_init) {
        baseline_torso_row_ema = cy;
        baseline_top_mass_ema = (float)topMass;
        baseline_hand_raise_init = true;
    }

    float massExcess = (float)topMass - baseline_top_mass_ema;
    last_hand_raise_top_mass = topMass;
    last_hand_raise_mass_excess = massExcess;

    bool learnBaseline = (distance_hint == TofDistanceHint::Ok) && !hands_raised_active &&
                         (massExcess < kHandRaiseMassExcess * 0.4f);

    if (learnBaseline) {
        baseline_torso_row_ema += (cy - baseline_torso_row_ema) * kHandRaiseBaselineLerp;
        baseline_top_mass_ema += ((float)topMass - baseline_top_mass_ema) * kHandRaiseBaselineLerp;
    }

    bool massHitRaw = massExcess >= kHandRaiseMassExcess;
    bool massHit = massHitRaw && interactionBandOk();

    if (massHit) {
        hand_raise_on_streak++;
        hand_raise_off_streak = 0;
    } else {
        hand_raise_on_streak = 0;
        hand_raise_off_streak++;
    }

    if (!hands_raised_active && hand_raise_on_streak >= kHandRaiseOnFrames) {
        hands_raised_active = true;
    }
    if (hands_raised_active && hand_raise_off_streak >= kHandRaiseOffFrames) {
        hands_raised_active = false;
    }
}

void TOFVisualizer::processPipelineForNewFrame() {
    uint32_t fid = sensor->getRangingFrameId();
    if (fid == 0 || fid == last_processed_frame_id) {
        return;
    }
    last_processed_frame_id = fid;

    int16_t raw[8][8];
    sampleLiveDistances(raw);
    pushFlickerAndBuildAnalysis(raw);
    advanceInBandStreaks(analysis_grid);
    findLargestBlobCentroid(analysis_grid, &blob);
    updateSmoothedCentroidFromBlob(blob);
    stepDistanceHint(analysis_grid);
    detectPalmOutlier(analysis_grid);
    stepHandRaise(analysis_grid, blob);
}

float TOFVisualizer::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t TOFVisualizer::distanceToColor(int16_t distance) {
    if (distance < 1) {
        return matrix->background->color565(0, 0, 0);
    }
    int16_t d = distance;
    if (d < minDistance) {
        d = minDistance;
    }
    if (d > maxDistance) {
        d = maxDistance;
    }

    float hue = mapFloat((float)d, (float)minDistance, (float)maxDistance, 0, 240);
    float h = hue / 60.0f;
    float s = 1.0f;
    float v = 1.0f;

    int i = (int)h;
    float f = h - (float)i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    float r, g, b;
    switch (i % 6) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
        default:
            r = g = b = 0;
            break;
    }

    uint8_t r8 = (uint8_t)(r * 255);
    uint8_t g8 = (uint8_t)(g * 255);
    uint8_t b8 = (uint8_t)(b * 255);

    return matrix->background->color565(r8, g8, b8);
}

void TOFVisualizer::drawHeatmapAndOverlays(uint16_t plotW, uint16_t plotH) {
    int16_t drawGrid[8][8];
    if (analysis_ready) {
        memcpy(drawGrid, analysis_grid, sizeof(drawGrid));
    } else {
        sampleLiveDistances(drawGrid);
    }

    // Heatmap uses only the plot region [0..plotW) × [0..plotH), leaving the bottom status strip
    // untouched so overlay text is not fighting the grid (avoids ~kStatusBarPixels of “missing” rows).
    uint16_t cDim = matrix->background->color565(6, 6, 8);
    uint16_t cTopBand = matrix->background->color565(28, 14, 0);
    uint16_t cPalmRing = matrix->background->color565(255, 40, 255);
    uint16_t cPalmCross = matrix->background->color565(255, 255, 255);
    uint16_t cCentroid = matrix->background->color565(200, 60, 255);

    for (uint8_t y = 0; y < 8; y++) {
        uint16_t y0 = (uint16_t)((unsigned)y * plotH / 8u);
        uint16_t y1 = (uint16_t)((unsigned)(y + 1u) * plotH / 8u);
        uint16_t bh = (uint16_t)(y1 - y0);
        if (bh < 1) {
            bh = 1;
        }
        for (uint8_t x = 0; x < 8; x++) {
            uint16_t x0 = (uint16_t)((unsigned)x * plotW / 8u);
            uint16_t x1 = (uint16_t)((unsigned)(x + 1u) * plotW / 8u);
            uint16_t bw = (uint16_t)(x1 - x0);
            if (bw < 1) {
                bw = 1;
            }

            int16_t d = drawGrid[y][x];
            bool masked = kHideOutsideActiveBand && !(d > minDistance && d < maxDistance);
            uint16_t color = masked ? cDim : distanceToColor(d);

            matrix->background->fillRect(x0, y0, bw, bh, color);

            if (kHandRaiseEnabled && y < (unsigned)kHandRaiseTopRows && !masked) {
                uint16_t bandH = bh > 2u ? 2u : bh;
                matrix->background->fillRect(x0, y0, bw, bandH, cTopBand);
            }
        }
    }

    if (palm.valid) {
        uint16_t x0 = (uint16_t)((unsigned)palm.x * plotW / 8u);
        uint16_t y0 = (uint16_t)((unsigned)palm.y * plotH / 8u);
        uint16_t x1 = (uint16_t)((unsigned)(palm.x + 1u) * plotW / 8u);
        uint16_t y1 = (uint16_t)((unsigned)(palm.y + 1u) * plotH / 8u);
        uint16_t bw = (uint16_t)(x1 - x0);
        uint16_t bh = (uint16_t)(y1 - y0);
        matrix->background->drawRect((int16_t)x0, (int16_t)y0, bw, bh, cPalmRing);
        if (bw > 3 && bh > 3) {
            matrix->background->drawRect((int16_t)(x0 + 1), (int16_t)(y0 + 1), (int16_t)(bw - 2), (int16_t)(bh - 2),
                                         cPalmRing);
        }
        if (bw > 2 && bh > 2) {
            int16_t ax = (int16_t)x0;
            int16_t ay = (int16_t)y0;
            int16_t bx = (int16_t)(x0 + bw - 1);
            int16_t by = (int16_t)(y0 + bh - 1);
            matrix->background->drawLine(ax, ay, bx, by, cPalmCross);
            matrix->background->drawLine(ax, by, bx, ay, cPalmCross);
        }
    }

    if (have_smoothed_centroid) {
        int16_t cmx = (int16_t)((smooth_centroid_cx * (float)plotW) / 8.0f);
        int16_t cmy = (int16_t)((smooth_centroid_cy * (float)plotH) / 8.0f);
        uint16_t minSpan = plotW < plotH ? plotW : plotH;
        int16_t r = (int16_t)(minSpan / 8u);
        if (r < 2) {
            r = 2;
        }
        if (r > 5) {
            r = 5;
        }
        matrix->background->fillCircle(cmx, cmy, r, cCentroid);
    }
}

void TOFVisualizer::drawStatusBar() {
    uint16_t h = matrix->getYResolution();
    // Two lines: distance prompt + hands state (Font4x5Fixed ~5px + gap)
    const uint16_t barH = kStatusBarPixels;
    if (h <= barH) {
        return;
    }
    uint16_t y0 = (uint16_t)(h - barH);

    const char* prompt = "";
    uint16_t promptColor = matrix->background->color565(180, 180, 190);

    switch (distance_hint) {
        case TofDistanceHint::NoPerson:
            prompt = "No person";
            promptColor = matrix->background->color565(90, 90, 100);
            break;
        case TofDistanceHint::TooClose:
            if (last_stance_depth_mm > 0 && last_stance_depth_mm < minDistance) {
                prompt = "Too close";
                promptColor = matrix->background->color565(255, 80, 40);
            } else {
                prompt = "Step back slightly";
                promptColor = matrix->background->color565(255, 140, 40);
            }
            break;
        case TofDistanceHint::TooFar:
            if (last_stance_depth_mm > maxDistance) {
                prompt = "Too far";
                promptColor = matrix->background->color565(80, 140, 255);
            } else {
                prompt = "Step forward slightly";
                promptColor = matrix->background->color565(100, 180, 255);
            }
            break;
        case TofDistanceHint::Ok:
            prompt = "In range";
            promptColor = matrix->background->color565(60, 220, 90);
            break;
    }

    const char* handsLine;
    uint16_t handsColor;
    if (!kHandRaiseEnabled) {
        handsLine = "Hands: (off)";
        handsColor = matrix->background->color565(80, 80, 90);
    } else if (hands_raised_active && interactionBandOk()) {
        handsLine = "HANDS UP";
        handsColor = matrix->background->color565(255, 220, 40);
    } else if (hands_raised_active) {
        handsLine = "Hands up (settle)";
        handsColor = matrix->background->color565(200, 160, 40);
    } else {
        handsLine = "Hands: down";
        handsColor = matrix->background->color565(130, 130, 145);
    }

    matrix->background->setFont(&Font4x5Fixed);
    matrix->background->setTextSize(1);
    matrix->background->setTextColor(promptColor);
    matrix->background->setCursor(2, (int16_t)(y0 + 1));
    matrix->background->print(prompt);

    matrix->background->setTextColor(handsColor);
    matrix->background->setCursor(2, (int16_t)(y0 + 7));
    matrix->background->print(handsLine);
}

void TOFVisualizer::draw() {
    drawWithBlockSizes(0, 0);
}

void TOFVisualizer::drawWithBlockSize(uint8_t blockSize) {
    drawWithBlockSizes(blockSize, blockSize);
}

void TOFVisualizer::drawWithBlockSizes(uint8_t blockSizeX, uint8_t blockSizeY) {
    (void)blockSizeX;
    (void)blockSizeY;

    if (!matrix) {
        log_e("TOF Visualizer: Matrix pointer is null!");
        return;
    }

    if (!sensor->isActive()) {
        log_v("TOF Visualizer: Sensor not active");
        matrix->clearScreen();
        resetInteractionState();
        flicker_count = 0;
        flicker_idx = 0;
        analysis_ready = false;
        last_processed_frame_id = 0;
        return;
    }

    processPipelineForNewFrame();

    uint16_t w = matrix->getXResolution();
    uint16_t h = matrix->getYResolution();

    matrix->background->fillScreen(matrix->background->color565(0, 0, 0));
    drawHeatmapAndOverlays(w, h);
    drawStatusBar();
}

