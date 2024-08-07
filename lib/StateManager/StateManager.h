#pragma once

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "ArduinoJson.h"

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
} OpenMatrixMode;

// Effects
typedef enum {
    NONE = 0,
    SIMPLEX_NOISE,
    CELLULAR_NOISE,
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

struct State {
    bool power = false;
    uint8_t brightness = 255;
    OpenMatrixMode mode = AQUARIUM;

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
            bool connected = false;
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
            bool connected = false;
            bool show_text = false;
        } home_assistant;
        struct {
            eDmxProtocol protocol = S_ACN;
            bool multicast = true;
            bool start_universe = true;
            uint16_t start_address = 1;
            eDmxMode mode = DMX_MODE_RGB;
            uint16_t timeout = 5000;
        } edmx;
    } settings;
};

class StateManager {
    public:
        StateManager();
        State* getState();
        void serialize(String& buffer, bool settings_only = false);
        void save();
        void restore();
        ~StateManager();
    private:
        State _state;
};
