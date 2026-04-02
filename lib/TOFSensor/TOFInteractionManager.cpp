#include "TOFInteractionManager.h"
#include <math.h>

TOFInteractionManager::TOFInteractionManager(TOFSensor* tofSensor)
    : sensor(tofSensor), baselineReady(false) {
    
    // Initialize arrays
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            baseline[y][x] = 0;
            activeCells[y][x] = false;
            currentDepth[y][x] = 0;
            previousDepth[y][x] = 0;  // 0 = uninitialized for motion weighting
        }
    }
    
    lastBlobX = 0.5f;
    lastBlobY = 0.5f;
    smoothedBlobX = 0.5f;
    smoothedBlobY = 0.5f;
    smoothedVelocityX = 0;
    smoothedVelocityY = 0;
    lastUpdateTime = millis();
    baselineCalibrationTime = millis();
    presenceStartTime = 0;
    presenceActive = false;
}

void TOFInteractionManager::rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY) {
    // TOFSensor owns physical sensor orientation. Aquarium may apply an additional
    // view-space offset to keep interactions aligned with Aquarium's presentation.
    uint16_t rotation = AQUARIUM_TOF_ROTATION_OFFSET % 360;
    switch (rotation) {
        case 90:
            outX = 7 - y;
            outY = x;
            break;
        case 180:
            outX = 7 - x;
            outY = 7 - y;
            break;
        case 270:
            outX = y;
            outY = 7 - x;
            break;
        default:
            outX = x;
            outY = y;
            break;
    }
}

void TOFInteractionManager::calibrateBaseline() {
    baselineReady = false;
    baselineCalibrationTime = millis();
    
    // Clear baseline
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            baseline[y][x] = 0;
        }
    }
}

void TOFInteractionManager::updateBaseline() {
    if (!sensor || !sensor->isActive()) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // During calibration period, accumulate baseline
    if (!baselineReady) {
        if (currentTime - baselineCalibrationTime < BASELINE_CALIBRATION_DURATION) {
            // Accumulate readings
            for (uint8_t y = 0; y < 8; y++) {
                for (uint8_t x = 0; x < 8; x++) {
                    uint8_t rx, ry;
                    rotateCoordinates(x, y, rx, ry);
                    uint8_t dx, dy;
                    sensor->toDisplayAligned(rx, ry, dx, dy);
                    int16_t dist = sensor->getDistance(dx, dy);
                    if (dist > 0) {
                        baseline[y][x] = (baseline[y][x] + dist) / 2; // Running average
                    }
                }
            }
        } else {
            baselineReady = true;
        }
    } else {
        // Adaptive baseline - slowly adapt to changes
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
                uint8_t rx, ry;
                rotateCoordinates(x, y, rx, ry);
                uint8_t dx, dy;
                sensor->toDisplayAligned(rx, ry, dx, dy);
                int16_t dist = sensor->getDistance(dx, dy);
                if (dist > 0) {
                    // Only adapt if no significant change (no person present)
                    int16_t diff = abs(dist - baseline[y][x]);
                    if (diff < TOF_ACTIVE_THRESHOLD) {
                        baseline[y][x] = baseline[y][x] * (1.0f - TOF_BASELINE_ADAPT_RATE) + 
                                        dist * TOF_BASELINE_ADAPT_RATE;
                    }
                }
            }
        }
    }
}

void TOFInteractionManager::detectActiveCells() {
    if (!sensor || !sensor->isActive()) {
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 8; x++) {
                activeCells[y][x] = false;
                currentDepth[y][x] = 0;
            }
        }
        return;
    }
    
    // Save previous frame before overwriting (for motion-weighted centroid)
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            previousDepth[y][x] = currentDepth[y][x];
        }
    }
    
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            uint8_t rx, ry;
            rotateCoordinates(x, y, rx, ry);
            uint8_t dx, dy;
            sensor->toDisplayAligned(rx, ry, dx, dy);
            int16_t dist = sensor->getDistance(dx, dy);
            currentDepth[y][x] = dist;
            
            if (dist < 0 || dist < TOF_MIN_DETECTION_DIST || dist > TOF_MAX_DETECTION_DIST) {
                activeCells[y][x] = false;
                continue;
            }
            
            // Background subtraction - detect significant changes from baseline
            if (baselineReady) {
                int16_t diff = baseline[y][x] - dist;  // Positive if closer than baseline
                activeCells[y][x] = (diff > TOF_ACTIVE_THRESHOLD);
            } else {
                // During calibration, mark as active if in valid range
                activeCells[y][x] = true;
            }
        }
    }
}

bool TOFInteractionManager::findLargestBlob(uint8_t& blobSize, float& blobX, float& blobY) {
    bool visited[8][8] = {false};
    uint8_t maxBlobSize = 0;
    float maxBlobCenterX = 0, maxBlobCenterY = 0;
    float maxBlobClosestX = 0, maxBlobClosestY = 0;
    
    // BFS flood fill to find connected components
    for (uint8_t sy = 0; sy < 8; sy++) {
        for (uint8_t sx = 0; sx < 8; sx++) {
            if (visited[sy][sx] || !activeCells[sy][sx]) continue;
            
            // Found a new blob, flood fill it
            uint8_t queue[64][2];  // x, y pairs
            uint8_t qHead = 0, qTail = 0;
            uint8_t count = 0;
            float sumX = 0, sumY = 0;
            float sumXW = 0, sumYW = 0, sumW = 0;  // Motion-weighted centroid
            int16_t minDepthInBlob = INT16_MAX;
            float closestX = sx + 0.5f;
            float closestY = sy + 0.5f;
            
            queue[qTail][0] = sx;
            queue[qTail][1] = sy;
            qTail++;
            visited[sy][sx] = true;
            
            while (qHead < qTail) {
                uint8_t cx = queue[qHead][0];
                uint8_t cy = queue[qHead][1];
                qHead++;
                
                float cellX = cx + 0.5f;
                float cellY = cy + 0.5f;
                count++;
                sumX += cellX;
                sumY += cellY;

                int16_t depth = currentDepth[cy][cx];
                if (depth > 0 && depth < minDepthInBlob) {
                    minDepthInBlob = depth;
                    closestX = cellX;
                    closestY = cellY;
                }
                
                // Motion weight: cells with significant depth change get higher weight
                // Prioritizes hand/head movement over static body
                float motionWeight = 0.0f;
                if (previousDepth[cy][cx] != 0) {
                    int16_t delta = abs(currentDepth[cy][cx] - previousDepth[cy][cx]);
                    motionWeight = (delta > TOF_MOTION_THRESHOLD_MM) 
                        ? (float)(delta - TOF_MOTION_THRESHOLD_MM) : 0.0f;
                }
                sumXW += cellX * motionWeight;
                sumYW += cellY * motionWeight;
                sumW += motionWeight;
                
                // Check 4 neighbors
                const int8_t dx[] = {1, -1, 0, 0};
                const int8_t dy[] = {0, 0, 1, -1};
                
                for (uint8_t d = 0; d < 4; d++) {
                    int8_t nx = cx + dx[d];
                    int8_t ny = cy + dy[d];
                    
                    if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) continue;
                    if (visited[ny][nx] || !activeCells[ny][nx]) continue;
                    
                    visited[ny][nx] = true;
                    if (qTail < 64) {
                        queue[qTail][0] = nx;
                        queue[qTail][1] = ny;
                        qTail++;
                    }
                }
            }
            
            // Check if this blob is the largest
            if (count >= TOF_MIN_BLOB_CELLS && count > maxBlobSize) {
                maxBlobSize = count;
                // Use motion-weighted centroid when there's moving content (hand/head)
                if (sumW > 1.0f) {
                    maxBlobCenterX = sumXW / sumW;
                    maxBlobCenterY = sumYW / sumW;
                } else {
                    maxBlobCenterX = sumX / count;
                    maxBlobCenterY = sumY / count;
                }
                maxBlobClosestX = closestX;
                maxBlobClosestY = closestY;
            }
        }
    }
    
    if (maxBlobSize >= TOF_MIN_BLOB_CELLS) {
        blobSize = maxBlobSize;
        // Bias toward closest point so hand sweeps feel more "leading edge"-driven.
        const float closestWeight = 0.7f;
        float targetX = maxBlobClosestX * closestWeight + maxBlobCenterX * (1.0f - closestWeight);
        float targetY = maxBlobClosestY * closestWeight + maxBlobCenterY * (1.0f - closestWeight);
        blobX = targetX / 8.0f;  // Normalize to 0-1
        blobY = targetY / 8.0f;
        return true;
    }
    
    return false;
}

void TOFInteractionManager::updateVelocity(float blobX, float blobY) {
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;  // Convert to seconds

    smoothedBlobX = smoothedBlobX * (1.0f - TOF_BLOB_POSITION_SMOOTH) + blobX * TOF_BLOB_POSITION_SMOOTH;
    smoothedBlobY = smoothedBlobY * (1.0f - TOF_BLOB_POSITION_SMOOTH) + blobY * TOF_BLOB_POSITION_SMOOTH;

    if (deltaTime > 0 && deltaTime < 1.0f) {
        float moveX = smoothedBlobX - lastBlobX;
        float moveY = smoothedBlobY - lastBlobY;
        float moveMag = sqrtf(moveX * moveX + moveY * moveY);

        if (moveMag >= TOF_VELOCITY_DEADZONE) {
            float velX = moveX / deltaTime;
            float velY = moveY / deltaTime;
            smoothedVelocityX = smoothedVelocityX * (1.0f - TOF_VELOCITY_SMOOTH) + velX * TOF_VELOCITY_SMOOTH;
            smoothedVelocityY = smoothedVelocityY * (1.0f - TOF_VELOCITY_SMOOTH) + velY * TOF_VELOCITY_SMOOTH;
        } else {
            smoothedVelocityX *= (1.0f - TOF_VELOCITY_SMOOTH);
            smoothedVelocityY *= (1.0f - TOF_VELOCITY_SMOOTH);
        }
    }

    lastBlobX = smoothedBlobX;
    lastBlobY = smoothedBlobY;
    lastUpdateTime = currentTime;
}

void TOFInteractionManager::update() {
    if (!sensor || !sensor->isActive()) {
        return;
    }
    
    updateBaseline();
    detectActiveCells();
}

InteractionData TOFInteractionManager::getInteractionData() const {
    InteractionData data;
    
    // Copy depth map
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            data.depthMap[y][x] = currentDepth[y][x];
        }
    }
    
    // Find blob
    uint8_t blobSize;
    float blobX, blobY;
    
    // Create non-const reference for findLargestBlob
    TOFInteractionManager* nonConstThis = const_cast<TOFInteractionManager*>(this);
    bool found = nonConstThis->findLargestBlob(blobSize, blobX, blobY);
    
    if (found) {
        data.hasBlob = true;
        data.blobX = blobX;
        data.blobY = blobY;
        data.blobSize = blobSize;

        if (!nonConstThis->presenceActive) {
            nonConstThis->presenceActive = true;
            nonConstThis->presenceStartTime = millis();
        }
        data.presenceDuration = (millis() - nonConstThis->presenceStartTime) / 1000.0f;

        nonConstThis->updateVelocity(blobX, blobY);

        data.velocityMag = sqrtf(smoothedVelocityX * smoothedVelocityX +
                                 smoothedVelocityY * smoothedVelocityY) * 8.0f;
        data.velocityX = smoothedVelocityX * 8.0f;
        data.velocityY = smoothedVelocityY * 8.0f;
    } else {
        data.hasBlob = false;
        data.blobX = 0.5f;
        data.blobY = 0.5f;
        data.blobSize = 0;
        data.velocityMag = 0;
        data.velocityX = 0;
        data.velocityY = 0;
        data.presenceDuration = 0;
        nonConstThis->presenceActive = false;
    }
    
    return data;
}
