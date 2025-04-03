# Carbon Emission Integration

This module connects the Livegrid aquarium ecosystem to real-time UK power grid carbon emission data, creating a visual representation of the environmental impact of electricity generation.

## How It Works

1. The `CarbonEmission` class fetches data from the UK Carbon Intensity API (https://api.carbonintensity.org.uk/)
2. It calculates a CO2 equivalent value based on the current energy mix (coal, gas, nuclear, renewables, etc.)
3. The calculation considers the carbon intensity of each energy source:
   - Coal: 820 gCO2eq/kWh
   - Gas: 390 gCO2eq/kWh
   - Solar: 42 gCO2eq/kWh
   - Wind: 4 gCO2eq/kWh
   - etc.
4. This CO2 equivalent value is fed into the fish health system:
   - Low carbon emissions (mostly renewables): Fish are healthy and reproduce normally
   - Medium carbon emissions (mixed sources): Fish health begins to decline
   - High carbon emissions (mostly fossil fuels): Fish health declines rapidly

## Visual Indicators

- Fish behavior and health reflect the carbon intensity of the grid
- The display shows the percentage of renewable energy in the current mix
- As renewable percentage increases, the fish ecosystem thrives
- As fossil fuel usage increases, the ecosystem suffers

## Technical Implementation

- Data is fetched every 30 minutes from the API
- The calculated CO2 equivalent value integrates with the existing CO2 measurement system
- When sensor data isn't available, the system falls back to using the carbon emission data
- The relationship between carbon intensity and CO2 values follows this mapping:
  - 0-100 gCO2/kWh → 400-600 ppm CO2 (healthy)
  - 100-250 gCO2/kWh → 600-1000 ppm CO2 (moderate impact)
  - 250-400 gCO2/kWh → 1000-2000 ppm CO2 (severe impact)
  - >400 gCO2/kWh → 2000 ppm CO2 (maximum impact)

## Dependencies

- ArduinoJson
- HTTPClient
- WiFi connectivity

## Future Improvements

- Add historical carbon intensity data visualization
- Integrate with local renewable energy forecasts
- Add alerts for high carbon periods
- Implement power-saving modes during peak carbon intensity periods 