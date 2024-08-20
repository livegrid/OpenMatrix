#pragma once 

#include <Arduino.h>

//NEW PCB
// #define RL1 18
// #define GL1 17
// #define BL1 16
// #define RL2 15
// #define GL2 7
// #define BL2 6
// #define CH_A 4
// #define CH_B 10
// #define CH_C 14
// #define CH_D 21
// // #define CH_E -1 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
// #define CH_E 5 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
// #define CLK 47
// #define LAT 48
// #define OE 38

//OLD PCB
#define RL1 18
#define GL1 17
#define BL1 16
#define RL2 7
#define GL2 15
#define BL2 14
#define CH_A 47
#define CH_B 21
#define CH_C 37
#define CH_D 38
#define CH_E 6 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
#define CLK 35
#define LAT 36
#define OE 48

#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 64     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1      // Total number of panels chained one to another

// #define DOUBLE_BUFFER 1
