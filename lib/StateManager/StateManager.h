#pragma once

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "ArduinoJson.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
    } image;

    // Text
    struct {
        String payload;
        TextSize size = SMALL;
    } text;

    // Settings
    struct {
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