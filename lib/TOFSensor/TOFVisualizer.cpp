#include "TOFVisualizer.h"
#include <Fonts/Font4x5Fixed.h>

TOFVisualizer::TOFVisualizer(TOFSensor* tofSensor, Matrix* matrixDisplay)
    : sensor(tofSensor),
      matrix(matrixDisplay),
      interactionManager(tofSensor),
      minDistance(TOF_MIN_DETECTION_DIST),
      maxDistance(TOF_MAX_DETECTION_DIST) {}

void TOFVisualizer::setMatrix(Matrix* matrixDisplay) {
    matrix = matrixDisplay;
}

void TOFVisualizer::setDistanceRange(int16_t minDist, int16_t maxDist) {
    minDistance = minDist;
    maxDistance = maxDist;
    interactionManager.setDistanceRange(minDist, maxDist);
}

float TOFVisualizer::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t TOFVisualizer::distanceToColor(int16_t distance) {
    if (distance < 1) return matrix->background->color565(0, 0, 0);
    int16_t d = distance;
    if (d < minDistance) d = minDistance;
    if (d > maxDistance) d = maxDistance;

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
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0; break;
    }

    return matrix->background->color565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

void TOFVisualizer::drawHeatmapAndOverlays(const InteractionData& interaction, uint16_t plotW, uint16_t plotH) {
    uint16_t cDim = matrix->background->color565(6, 6, 8);
    uint16_t cTopBand = matrix->background->color565(28, 14, 0);
    uint16_t cPalmRing = matrix->background->color565(255, 40, 255);
    uint16_t cPalmCross = matrix->background->color565(255, 255, 255);
    uint16_t cCentroid = matrix->background->color565(200, 60, 255);

    for (uint8_t y = 0; y < 8; y++) {
        uint16_t y0 = (uint16_t)((unsigned)y * plotH / 8u);
        uint16_t y1 = (uint16_t)((unsigned)(y + 1u) * plotH / 8u);
        uint16_t bh = (uint16_t)(y1 - y0);
        if (bh < 1) bh = 1;

        for (uint8_t x = 0; x < 8; x++) {
            uint16_t x0 = (uint16_t)((unsigned)x * plotW / 8u);
            uint16_t x1 = (uint16_t)((unsigned)(x + 1u) * plotW / 8u);
            uint16_t bw = (uint16_t)(x1 - x0);
            if (bw < 1) bw = 1;

            int16_t d = interaction.depthMap[y][x];
            bool masked = kHideOutsideActiveBand && !(d > minDistance && d < maxDistance);
            uint16_t color = masked ? cDim : distanceToColor(d);
            matrix->background->fillRect(x0, y0, bw, bh, color);

            if (y < 2 && !masked) {
                uint16_t bandH = bh > 2 ? 2 : bh;
                matrix->background->fillRect(x0, y0, bw, bandH, cTopBand);
            }
        }
    }

    if (interaction.hasPalm) {
        uint16_t x0 = (uint16_t)((unsigned)interaction.palmX * plotW / 8u);
        uint16_t y0 = (uint16_t)((unsigned)interaction.palmY * plotH / 8u);
        uint16_t x1 = (uint16_t)((unsigned)(interaction.palmX + 1u) * plotW / 8u);
        uint16_t y1 = (uint16_t)((unsigned)(interaction.palmY + 1u) * plotH / 8u);
        uint16_t bw = (uint16_t)(x1 - x0);
        uint16_t bh = (uint16_t)(y1 - y0);
        matrix->background->drawRect((int16_t)x0, (int16_t)y0, bw, bh, cPalmRing);
        if (bw > 3 && bh > 3) {
            matrix->background->drawRect((int16_t)(x0 + 1), (int16_t)(y0 + 1), (int16_t)(bw - 2), (int16_t)(bh - 2),
                                         cPalmRing);
            int16_t ax = (int16_t)x0;
            int16_t ay = (int16_t)y0;
            int16_t bx = (int16_t)(x0 + bw - 1);
            int16_t by = (int16_t)(y0 + bh - 1);
            matrix->background->drawLine(ax, ay, bx, by, cPalmCross);
            matrix->background->drawLine(ax, by, bx, ay, cPalmCross);
        }
    }

    if (interaction.hasBlob) {
        int16_t cx = (int16_t)(interaction.blobX * (float)plotW);
        int16_t cy = (int16_t)(interaction.blobY * (float)plotH);
        uint16_t minSpan = plotW < plotH ? plotW : plotH;
        int16_t r = (int16_t)(minSpan / 8u);
        if (r < 2) r = 2;
        if (r > 5) r = 5;
        matrix->background->fillCircle(cx, cy, r, cCentroid);
    }
}

void TOFVisualizer::drawStatusBar(const InteractionData& interaction) {
    uint16_t h = matrix->getYResolution();
    const uint16_t barH = kStatusBarPixels;
    if (h <= barH) return;
    uint16_t y0 = (uint16_t)(h - barH);

    const char* prompt = "";
    uint16_t promptColor = matrix->background->color565(180, 180, 190);
    switch (interaction.distanceHint) {
        case TofDistanceHint::NoPerson:
            prompt = "No person";
            promptColor = matrix->background->color565(90, 90, 100);
            break;
        case TofDistanceHint::TooClose:
            if (interaction.stanceDepthMm > 0 && interaction.stanceDepthMm < minDistance) {
                prompt = "Too close";
                promptColor = matrix->background->color565(255, 80, 40);
            } else {
                prompt = "Step back slightly";
                promptColor = matrix->background->color565(255, 140, 40);
            }
            break;
        case TofDistanceHint::TooFar:
            if (interaction.stanceDepthMm > maxDistance) {
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

    const char* handsLine = interaction.handsRaised ? "HANDS UP" : "Hands: down";
    uint16_t handsColor =
        interaction.handsRaised ? matrix->background->color565(255, 220, 40) : matrix->background->color565(130, 130, 145);

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
    if (!sensor || !sensor->isActive()) {
        matrix->clearScreen();
        interactionManager.calibrateBaseline();
        return;
    }

    interactionManager.update();
    InteractionData interaction = interactionManager.getInteractionData();

    uint16_t w = matrix->getXResolution();
    uint16_t h = matrix->getYResolution();
    matrix->background->fillScreen(matrix->background->color565(0, 0, 0));
    drawHeatmapAndOverlays(interaction, w, h);
    drawStatusBar(interaction);
}
