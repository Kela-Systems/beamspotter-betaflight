#pragma once

// Headers

#include <stdbool.h>
#include <stdint.h>
#include "drivers/time.h"
#include "io/serial.h"
#include "msp/msp_protocol_beamspotter.h"
#include "drivers/beamspotter/beamspotter.h"

// Constants

#define BEAMSPOTTER_TASK_SCHEDULE_RATE_HZ   (10)
#define BEAMSPOTTER_REQUEST_TIMEOUT_MS      (100)

// Enums

typedef enum {
    BEAMSPOTTER_NONE = 0,
    BEAMSPOTTER_TRIEYE = 1,
} beamspotterSensor_e;

// Structures

typedef struct beamspotterConfig_s {
    uint8_t frequency;                      // Update frequency in Hz (1-100)
    serialPortIdentifier_e portIdentifier;  // Serial port to use for communication
} beamspotterConfig_t;

typedef struct beamspotterCoordinates_s {
    uint32_t x;
    uint32_t y;
} beamspotterCoordinates_t;

typedef struct beamspotter_s {
    beamspotterDev_t dev;

    bool isInitialized;
    bool hasFix;
    beamspotterCoordinates_t coordinates;    // X,Y coordinates of the laser beam position
    timeMs_t lastUpdateMs;                 // Time of last successful update
    BeamSpotterStatus_t lastStatus;        // Last status code from the sensor
    beamspotterConfig_t config;            // Configuration
} beamspotter_t;

// Global variables

extern beamspotter_t beamspotter;

// Functions

bool beamspotterInit(void);
void beamspotterUpdate(void);
void beamspotterProcess(void);
bool beamspotterIsHealthy(void);
bool beamspotterHasFix(void);
bool beamspotterGetCoordinates(beamspotterCoordinates_t *coordinates);
