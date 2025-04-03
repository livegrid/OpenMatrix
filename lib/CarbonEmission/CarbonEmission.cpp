#include "CarbonEmission.h"

// Global instance of StateManager (declared externally)
extern StateManager stateManager;

void CarbonEmission::begin() {
    // Initial fetch of data
    update();
}

void CarbonEmission::update() {
    unsigned long currentTime = millis();
    
    // Update only if fetchInterval has passed since last update
    if (currentTime - lastFetchTime >= fetchInterval || lastFetchTime == 0) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(apiUrl);
            http.addHeader("Accept", "application/json");
            
            int httpResponseCode = http.GET();
            if (httpResponseCode == 200) {
                String response = http.getString();
                if (parseResponse(response)) {
                    lastFetchTime = currentTime;
                    currentFuelMix.timestamp = currentTime;
                    currentFuelMix.dataValid = true;
                    
                    // Calculate CO2 equivalent based on the energy mix
                    currentFuelMix.calculatedCO2 = calculateCO2Equivalent();
                    
                    // Update StateManager with the calculated value
                    updateStateManager();
                    
                    log_i("Carbon data updated. CO2 equivalent: %u ppm", currentFuelMix.calculatedCO2);
                } else {
                    log_e("Failed to parse carbon emission data response");
                }
            } else {
                log_e("HTTP GET failed with response code: %d", httpResponseCode);
            }
            
            http.end();
        } else {
            log_w("WiFi not connected, can't fetch carbon emission data");
        }
    }
}

bool CarbonEmission::parseResponse(String& response) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        log_e("JSON parsing failed: %s", error.c_str());
        return false;
    }
    
    // Check if response contains expected structure
    if (!doc.containsKey("data") || doc["data"].size() == 0 || 
        !doc["data"][0].containsKey("generationmix")) {
        log_e("Unexpected JSON structure in API response");
        return false;
    }
    
    // Extract generation mix
    JsonArray mix = doc["data"][0]["generationmix"];
    
    // Reset percentages
    memset(currentFuelMix.percentages, 0, sizeof(currentFuelMix.percentages));
    
    // Map fuel types to indices
    for (JsonObject fuel : mix) {
        const char* fuelType = fuel["fuel"];
        float percentage = fuel["perc"];
        
        int index = -1;
        if (strcmp(fuelType, "gas") == 0) index = 0;
        else if (strcmp(fuelType, "coal") == 0) index = 1;
        else if (strcmp(fuelType, "biomass") == 0) index = 2;
        else if (strcmp(fuelType, "nuclear") == 0) index = 3;
        else if (strcmp(fuelType, "hydro") == 0) index = 4;
        else if (strcmp(fuelType, "imports") == 0) index = 5;
        else if (strcmp(fuelType, "other") == 0) index = 6;
        else if (strcmp(fuelType, "wind") == 0) index = 7;
        else if (strcmp(fuelType, "solar") == 0) index = 8;
        
        if (index >= 0) {
            currentFuelMix.percentages[index] = percentage;
        }
    }
    
    return true;
}

uint16_t CarbonEmission::calculateCO2Equivalent() {
    float weightedSum = 0;
    
    // Calculate weighted average of carbon intensity
    for (int i = 0; i < 9; i++) {
        weightedSum += currentFuelMix.percentages[i] * carbonIntensity[i];
    }
    
    // Log the calculated carbon intensity
    log_i("Grid carbon intensity: %.2f gCO2/kWh", weightedSum);
    
    // Scale the result to match the CO2 ppm range used in the fish health system
    // Map from typical range of carbon intensity (0-500 g/kWh) to CO2 range (400-2000 ppm)
    float co2Equivalent;
    
    if (weightedSum < CARBON_LOW) {
        // Low carbon intensity - healthy for fish (400-600 ppm)
        co2Equivalent = map(weightedSum, 0, CARBON_LOW, 400, CO2_OK);
    } else if (weightedSum < CARBON_MEDIUM) {
        // Medium carbon intensity - starts affecting fish (600-1000 ppm)
        co2Equivalent = map(weightedSum, CARBON_LOW, CARBON_MEDIUM, CO2_OK, CO2_BAD);
    } else if (weightedSum < CARBON_HIGH) {
        // High carbon intensity - bad for fish (1000-2000 ppm)
        co2Equivalent = map(weightedSum, CARBON_MEDIUM, CARBON_HIGH, CO2_BAD, CO2_REALBAD);
    } else {
        // Very high carbon intensity - really bad for fish (max 2000 ppm)
        co2Equivalent = CO2_REALBAD;
    }
    
    // Ensure the result is within reasonable bounds
    co2Equivalent = constrain(co2Equivalent, 400, 2000);
    
    return static_cast<uint16_t>(co2Equivalent);
}

uint16_t CarbonEmission::getCO2Equivalent() {
    return currentFuelMix.calculatedCO2;
}

void CarbonEmission::updateStateManager() {
    State* state = stateManager.getState();
    
    // Update the CO2 value in the environment state
    state->environment.co2.value = currentFuelMix.calculatedCO2;
    
    // You could also update other state information here if needed
    // For example, adding a new carbon emission specific state
}

float CarbonEmission::getPercentage(const char* fuelType) {
    int index = -1;
    if (strcmp(fuelType, "gas") == 0) index = 0;
    else if (strcmp(fuelType, "coal") == 0) index = 1;
    else if (strcmp(fuelType, "biomass") == 0) index = 2;
    else if (strcmp(fuelType, "nuclear") == 0) index = 3;
    else if (strcmp(fuelType, "hydro") == 0) index = 4;
    else if (strcmp(fuelType, "imports") == 0) index = 5;
    else if (strcmp(fuelType, "other") == 0) index = 6;
    else if (strcmp(fuelType, "wind") == 0) index = 7;
    else if (strcmp(fuelType, "solar") == 0) index = 8;
    
    if (index >= 0) {
        return currentFuelMix.percentages[index];
    }
    
    return 0.0f;
}

float CarbonEmission::getRenewablePercentage() {
    // Sum of hydro, wind, solar, and part of biomass
    return currentFuelMix.percentages[4] + currentFuelMix.percentages[7] + 
           currentFuelMix.percentages[8] + (currentFuelMix.percentages[2] * 0.5f);
}

float CarbonEmission::getFossilFuelPercentage() {
    // Sum of gas and coal
    return currentFuelMix.percentages[0] + currentFuelMix.percentages[1];
} 