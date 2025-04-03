#ifndef CARBON_EMISSION_H
#define CARBON_EMISSION_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "StateManager.h"
#include "CarbonEmissionSettings.h" 
#include "..\SCD40\SCD40Settings.h"

class CarbonEmission {
private:
    const char* apiUrl = "https://api.carbonintensity.org.uk/generation";
    unsigned long lastFetchTime = 0;
    const unsigned long fetchInterval = CARBON_API_FETCH_INTERVAL;
    
    // Carbon intensity values for different fuel types (gCO2eq/kWh)
    // Source: https://www.gov.uk/government/publications/greenhouse-gas-reporting-conversion-factors-2022
    const float carbonIntensity[9] = {
        390,    // gas
        820,    // coal
        230,    // biomass
        6,      // nuclear
        4,      // hydro
        300,    // imports (average)
        300,    // other (average)
        4,      // wind
        42      // solar
    };

    struct FuelMix {
        float percentages[9] = {0}; // Percentages for each fuel type
        uint16_t calculatedCO2 = 400; // Default CO2 value
        unsigned long timestamp = 0;
        bool dataValid = false;
    };

    FuelMix currentFuelMix;
    
    bool parseResponse(String& response);
    uint16_t calculateCO2Equivalent();
    void updateStateManager();
    
public:
    CarbonEmission() {}
    
    void begin();
    void update();
    uint16_t getCO2Equivalent();
    bool isDataValid() const { return currentFuelMix.dataValid; }
    
    // For display purposes
    float getPercentage(const char* fuelType);
    float getRenewablePercentage();
    float getFossilFuelPercentage();
};

#endif // CARBON_EMISSION_H 