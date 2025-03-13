#pragma once

// Global Variables

struct beamspotterDev_s;

// Callback Definitions

typedef void (*beamspotterOpInitFuncPtr)(struct beamspotterDev_s * dev);
typedef void (*beamspotterOpStartFuncPtr)(struct beamspotterDev_s * dev);
typedef void (*beamspotterOpReadFuncPtr)(struct beamspotterDev_s * dev);

// Structure

typedef struct beamspotterDev_s {
    beamspotterOpInitFuncPtr init;
    beamspotterOpStartFuncPtr update;
    beamspotterOpReadFuncPtr read;
} beamspotterDev_t;
