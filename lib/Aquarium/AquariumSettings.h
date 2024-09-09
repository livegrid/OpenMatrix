#pragma once

//GENERAL SETTINGS
#define PHYSICS_SCALE 50
#define AQUARIUM_SAVE_INTERVAL 30    //in minutes
#define BORDER_BUFFER 0
#define FOOD_TOUCH_THRESHOLD 100

//PLANT LIFE SETTINGS
#define PLANT_LIFECYCLE_DURATION 10000 // 1 day in milliseconds
// #define PLANT_LIFECYCLE_DURATION 86400000 // 1 day in milliseconds

//AGE SETTINGS
#define FISH_LIFESPAN_DAYS 7.0f  // Average lifespan in days
#define FISH_LIFESPAN_VARIATION 0.2f  // Variation in lifespan (20% by default)

const float HEALTH_REDUCTION_RATE_BAD = 0.1f;    // 10% per second
const float HEALTH_REDUCTION_RATE_REALBAD = 0.2f; // 20% per second
const float HEALTH_INCREASE_RATE_GOOD = 0.05f;    // 5% per second

// const float HEALTH_REDUCTION_RATE_BAD = 0.2f / (3600 * 30);    // 20% per hour at 30 fps
// const float HEALTH_REDUCTION_RATE_REALBAD = 0.4f / (3600 * 30); // 40% per hour at 30 fps
// const float HEALTH_INCREASE_RATE_GOOD = 1 / (3600 * 30); // 100% per hour at 30 fps

#define NUM_FISH_START 5
#define NUM_FISH_IDEAL 30
#define NUM_PLANTS 3

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
#define BOUNDARY_FORCE 0.2  //change to 0.2

//EGG SETTINGS
#define EGG_SIZE 2
#define EGG_COLOR 0, 0, 200

//FISH SETTINGS
#define FISH_NUM_SEGMENTS 4, 12
#define FISH_MIN_SEGMENT_SIZE 2.0
#define FISH_MAX_SEGMENT_SIZE 2, 8
#define FISH_GAP_BETWEEN_SEGMENTS 70, 100

#define FISH_MAX_SPEED 30
#define FISH_MIN_SPEED 10
#define FISH_MAX_FORCE 0.3
#define FISH_SIN_AMPLITUDE 2
#define FISH_SIN_FREQUENCY 0.002
#define FISH_NOISE_AMPLITUDE 6
#define FISH_NOISE_FREQUENCY 0.01

#define FISH_NEEDLE_NOSE_LENGTH_MULTIPLIER 5, 10

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
#define STAR_ROTATION_SPEED 1, 10

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

//FLOCKING SETTINGS
#define FLOCK_MAX_FORCE 0.8
#define SEPARATION_DISTANCE 20
#define ALIGNMENT_DISTANCE 30
#define COHESION_DISTANCE 30
#define SEPARATION_WEIGHT .3
#define ALIGNMENT_WEIGHT 1.0
#define COHESION_WEIGHT .2

//ATTRACTOR SETTINGS
#define BOID_GROUPS 3
#define NUM_BOIDS 10, 20
#define BOID_MAX_SPEED 5, 10
#define BOID_MAX_FORCE 1, 2