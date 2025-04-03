#pragma once

// Carbon emission intensity thresholds (gCO2eq/kWh)
// These correspond to the level of CO2 emissions from the grid
#define CARBON_LOW 100      // Very low carbon intensity (mostly renewables)
#define CARBON_MEDIUM 250   // Medium carbon intensity (mixed sources)
#define CARBON_HIGH 400     // High carbon intensity (mostly fossil fuels)

// Fetch interval for API data (in milliseconds)
#define CARBON_API_FETCH_INTERVAL 30 * 60 * 1000  // 30 minutes 