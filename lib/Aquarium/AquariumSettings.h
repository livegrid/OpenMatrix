#pragma once

#include "GeneralSettings.h"

//GENERAL SETTINGS
#if defined(PANEL_UPCYCLED)
    #define PHYSICS_SCALE 50
#else
    #define PHYSICS_SCALE 80
#endif

#define AQUARIUM_SAVE_INTERVAL 30    //in minutes
#define BORDER_BUFFER 0
#define FOOD_TOUCH_THRESHOLD 100

//AGE SETTINGS
#define FISH_LIFESPAN_DAYS 7.0f  // Average lifespan in days
#define FISH_LIFESPAN_VARIATION 1.0f  // Variation in lifespan (20% by default)

const float HEALTH_REDUCTION_RATE_BAD = 0.1f;    // 10% per second
const float HEALTH_REDUCTION_RATE_REALBAD = 0.2f; // 20% per second
const float HEALTH_INCREASE_RATE_GOOD = 0.05f;    // 5% per second

// const float HEALTH_REDUCTION_RATE_BAD = 0.2f / (3600 * 30);    // 20% per hour at 30 fps
// const float HEALTH_REDUCTION_RATE_REALBAD = 0.4f / (3600 * 30); // 40% per hour at 30 fps
// const float HEALTH_INCREASE_RATE_GOOD = 1 / (3600 * 30); // 100% per hour at 30 fps

#define NUM_FISH_START 20
#define NUM_FISH_IDEAL 20
#define NUM_PLANTS 9

//PLANT SETTINGS


//AGE THRESHOLDS
#define AGE_EGG 0.1
#define AGE_CHILD 0.3
#define AGE_TEEN 0.5
#define AGE_ADULT 0.7
#define AGE_SENIOR 0.9
#define AGE_DEAD 1.0

//GENERAL FORCE SETTINGS
#define MAX_FORCE 3
#define FOOD_FORCE MAX_FORCE * 2
#define BOUNDARY_FORCE 0.2 

//EGG SETTINGS
#define EGG_SIZE 1
#define EGG_COLOR 0, 0, 200

//FISH SETTINGS
#define FISH_NUM_SEGMENTS 4, 12
#define FISH_MIN_SEGMENT_SIZE 2.0
#define FISH_MAX_SEGMENT_SIZE 2, 6
#define FISH_GAP_BETWEEN_SEGMENTS 70, 100

#define FISH_MAX_SPEED 30
#define FISH_MIN_SPEED 10
#define FISH_MAX_FORCE 0.3
#define FISH_SIN_AMPLITUDE 2
#define FISH_SIN_FREQUENCY 0.002
#define FISH_NOISE_AMPLITUDE 6
#define FISH_NOISE_FREQUENCY 0.01

#define FISH_NEEDLE_NOSE_LENGTH_MULTIPLIER 2, 6

//SNAKE SETTINGS
#define SNAKE_NUM_SEGMENTS 8, 60

#define SNAKE_MAX_SPEED 40
#define SNAKE_MIN_SPEED 20
#define SNAKE_MAX_FORCE 0.3
#define SNAKE_SIN_AMPLITUDE 5
#define SNAKE_SIN_FREQUENCY 0.005
#define SNAKE_NOISE_AMPLITUDE 6
#define SNAKE_NOISE_FREQUENCY 0.01

//STAR SETTINGS
#define STAR_LENGTH 4, 6
#define STAR_RAD 2, 4
#define STAR_NUM_ARMS 5, 10

#define STAR_MAX_SPEED 30
#define STAR_MIN_SPEED 10
#define STAR_MAX_FORCE 0.2
#define STAR_SIN_AMPLITUDE 5
#define STAR_SIN_FREQUENCY 0.005
#define STAR_NOISE_AMPLITUDE 2
#define STAR_NOISE_FREQUENCY 0.01
#define STAR_ROTATION_SPEED 5, 10

//TURTLE SETTINGS
#define TURTLE_LENGTH 4, 10
#define TURTLE_WIDTH 2, 4

#define TURTLE_MAX_SPEED 40
#define TURTLE_MIN_SPEED 5
#define TURTLE_MAX_FORCE 0.3
#define TURTLE_SIN_AMPLITUDE 10
#define TURTLE_SIN_FREQUENCY 0.001
#define TURTLE_NOISE_AMPLITUDE 1
#define TURTLE_NOISE_FREQUENCY 0.01
//OCTOPUS SETTINGS
#define OCTOPUS_SIZE 10, 20
#define OCTOPUS_MIN_TENTACLES 6
#define OCTOPUS_MAX_TENTACLES 10
#define OCTOPUS_TENTACLE_SEGMENTS 6
#define OCTOPUS_TENTACLE_LENGTH 10, 25

#define OCTOPUS_MAX_SPEED 35
#define OCTOPUS_MIN_SPEED 5
#define OCTOPUS_MAX_FORCE 0.25
#define OCTOPUS_SIN_AMPLITUDE 8
#define OCTOPUS_SIN_FREQUENCY 0.002
#define OCTOPUS_NOISE_AMPLITUDE .1
#define OCTOPUS_NOISE_FREQUENCY 0.01

//FLOCKING SETTINGS
#define FLOCK_MAX_FORCE 0.8
#define SEPARATION_DISTANCE 20
#define ALIGNMENT_DISTANCE 30
#define COHESION_DISTANCE 30
#define SEPARATION_WEIGHT .3
#define ALIGNMENT_WEIGHT 1.0
#define COHESION_WEIGHT .2

//ATTRACTOR SETTINGS
#define BOID_GROUPS 2
#define NUM_BOIDS 10, 20
#if defined(PANEL_UPCYCLED)
    #define BOID_MAX_SPEED 5, 10
#else
    #define BOID_MAX_SPEED 4, 8
#endif
#define BOID_MAX_FORCE 1, 2

//TOF Interaction Settings
#define TOF_MIN_DETECTION_DIST 1000    // mm - closer than this is ignored (too close)
#define TOF_MAX_DETECTION_DIST 3000   // mm - further than this is ignored
#define TOF_BASELINE_ADAPT_RATE 0.01f  // How fast baseline adapts (0-1)
#define TOF_ACTIVE_THRESHOLD 200      // mm difference from baseline to be "active"
#define TOF_MIN_BLOB_CELLS 2          // Minimum cells to count as valid blob
#define TOF_VELOCITY_SMOOTH 0.3f       // Velocity smoothing factor (0-1)
#define TOF_BLOB_POSITION_SMOOTH 0.3f   // Blob position smoothing (0-1, lower = smoother)
#define TOF_VELOCITY_DEADZONE 0.0625f  // Ignore velocity if blob move < 0.5 grid cells (1/16)
#define TOF_MOTION_THRESHOLD_MM 40      // mm depth change between frames to count as "moving" (prioritizes hand over static body)

// Fish Interaction State Machine
#define INTERACTION_RADIUS_FRACTION 1.0f   // Fraction of min(screenW,screenH) that counts as interaction range
#define INTERACTION_DISTANCE_EPSILON 0.1f  // Min distance to avoid div-by-zero

// ALERT state
#define STATE_ALERT_DURATION_MS 500        // How long fish freezes when blob first appears
#define STATE_ALERT_DAMPING 0.85f          // vel *= this each frame while alert (quick decel)

// SCARED state
#define STATE_SCARED_REPEL_FORCE 1.5f      // Force magnitude when darting away
#define STATE_SCARED_SPEED_BOOST 1.5f      // maxSpeed multiplier while scared
#define STATE_SCARED_CALM_VELOCITY 0.3f    // Blob velocity below this = "calm" (allows transition to CURIOUS)
#define STATE_SCARED_CALM_DURATION_S 3.0f  // Seconds of calm presence before transitioning to CURIOUS

// CURIOUS state
#define STATE_CURIOUS_FOLLOW_FORCE_FRAC 0.3f  // Fraction of FOOD_FORCE used to follow blob
#define STATE_CURIOUS_SIN_SCALE 0.3f       // Reduce sin amplitude to this fraction
#define STATE_CURIOUS_NOISE_SCALE 0.2f     // Reduce noise amplitude to this fraction
#define STATE_CURIOUS_SCARE_VELOCITY 1.0f  // Blob velocity above this snaps back to SCARED (lowered for hand motion sensitivity)
#define CURIOUS_ATTRACTION_STOP_DISTANCE 60  // Physics units - stop attracting when fish this close (avoids clustering)
#define STATE_CURIOUS_PAUSE_INTERVAL_MS 500 // How often to check for a pause while curious
#define STATE_CURIOUS_PAUSE_CHANCE 50      // 1/50 = 2% chance per check
#define STATE_CURIOUS_PAUSE_MIN_MS 400     // Min curious pause
#define STATE_CURIOUS_PAUSE_MAX_MS 800     // Max curious pause
#define STATE_CURIOUS_PAUSE_DAMPING 0.90f  // Damping during curious pause

// IDLE recovery
#define STATE_IDLE_RECOVERY_MS 2000        // After blob disappears, return to IDLE after this delay

// Plankton Visual Feedback
#define PLANKTON_MAX_COUNT 2000
#define PLANKTON_TOF_GRID_SIZE 8
#define PLANKTON_HUE_BASE 96            // Greenish (FastLED hue)
#define PLANKTON_HUE_RANGE 40
#define PLANKTON_SAT_BASE 100
#define PLANKTON_SAT_RANGE 60
#define PLANKTON_BRIGHTNESS_RISE_SHIFT 2   // diff >> 2 = fast rise ~25%/frame
#define PLANKTON_BRIGHTNESS_FADE_SHIFT 4   // diff >> 4 = slow fade ~6%/frame

// Food
#define FOOD_FALL_SPEED 0.3f

// TOF Baseline
#define TOF_BASELINE_CALIBRATION_DURATION_MS 2000

// Silhouette Effect Settings (drawn on foreground)
#define SILHOUETTE_OPACITY 0.8f        // How visible the silhouette is (0-1)
#define SILHOUETTE_DEPTH_THRESHOLD 0.7f // Max normalized depth to show (0-1, higher = show further objects)
#define SILHOUETTE_COLOR_R 30          // Dark blue-ish color for underwater shadow
#define SILHOUETTE_COLOR_G 50
#define SILHOUETTE_COLOR_B 80