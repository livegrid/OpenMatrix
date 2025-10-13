#pragma once

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "ArduinoJson.h"
#include "GeneralSettings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "StateDefaults.h"

// Diff Type
typedef enum {
    DISABLE = 0,
    UP,
    DOWN
} DiffType;

// Open Matrix Mode
typedef enum {
    AQUARIUM = 0,
    EFFECT,
    IMAGE,
    TEXT,
    DMX,
    STARTUP
} OpenMatrixMode;

typedef enum {
    CELSIUS = 0,
    FAHRENHEIT
} TemperatureUnit;

// Effects
typedef enum {
    NONE = 0,
    SIMPLEX_NOISE,
    SNAKE,
    FLOCKING,
    GAMEOFLIFE,
    LSYSTEM,
} Effects;

// Text
typedef enum {
    SMALL = 0,
    MEDIUM,
    LARGE,
} TextSize;

// eDMX 
typedef enum {
    S_ACN = 0,
    ART_NET
} eDmxProtocol;

typedef enum {
    DMX_MODE_RGB = 0,
    DMX_MODE_WHITE
} eDmxMode;

typedef enum {
    DISCONNECTED = 0,
    CONNECTED,
    RECONNECTING,
    FAILED,
} ConnectionStatus;

struct State {
    bool power;
    bool autobrightness;
    uint8_t brightness;
    OpenMatrixMode mode;
    bool firstBoot;
    TemperatureUnit temperatureUnit;

    // Environment
    struct {
        struct {
            float value;
            struct {
                DiffType type;
                String value;
                bool inverse;
            } diff;
            float history_24h[24];
        } temperature;
        struct {
            float value;
            struct {
                DiffType type;
                String value;
                bool inverse;
            } diff;
            float history_24h[24];
        } temperature_fahrenheit;
        struct {
            float value;
            struct {
                DiffType type;
                String value;
                bool inverse;
            } diff;
            float history_24h[24];
        } humidity;
        struct {
            float value;
            struct {
                DiffType type;
                String value;
                bool inverse;
            } diff;
            float history_24h[24];
        } co2;
    } environment;

    // Effects
    struct {
        Effects selected = SIMPLEX_NOISE;
    } effects;

    // Image
    struct {
        String selected = "";
        uint8_t width;
        uint8_t height;
    } image;

    // Text
    struct {
        String payload;
        TextSize size = SMALL;
    } text;

    // Settings
    struct {
        struct {
            float temperatureOffsetC = 0.0f;
            float humidityOffsetPct = 0.0f;
            int16_t co2OffsetPpm = 0;
        } calibration;
        struct {
            ConnectionStatus status = DISCONNECTED;
            String host;
            String port;
            String client_id;
            String username;
            String password;
            String co2_topic;
            String matrix_text_topic;
            bool show_text = false;
        } mqtt;
        struct {
            ConnectionStatus status = DISCONNECTED;
            bool show_text = false;
        } home_assistant;
        struct {
            eDmxProtocol protocol = eDmxProtocol::S_ACN;
            bool multicast = true;
            uint16_t start_universe = 1;
            uint16_t start_address = 1;
            eDmxMode mode = eDmxMode::DMX_MODE_RGB;
            uint16_t timeout = 5000;
        } edmx;
        struct {
            bool enableDarkAutoPower = false;
            float darkThresholdLux = 2.0f;
            float darkHysteresisLux = 0.5f;
            uint16_t darkStabilitySeconds = 15;
        } scheduler;
    } settings;
};

class StateManager {
    public:
        StateManager(unsigned long saveIntervalinMinutes = 10);
        State* getState();
        void serialize(String& buffer, bool settings_only = false);
        void save();
        bool stateChanged;
        void restore();
        void startPeriodicSave();
        ~StateManager();
    private:
        State _state;
        TaskHandle_t _saveTaskHandle;
        static void saveTask(void* parameter);
                
        void setDefaultState();
        void saveState();
        unsigned long SAVE_INTERVAL;
};