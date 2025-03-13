#pragma once

// Headers

#include <stdint.h>

// MSP BeamSpotter Command IDs
typedef enum {
    MSP_BEAMSPOTTER_GET_CONFIG = 195,
    MSP_BEAMSPOTTER_SET_CONFIG = 196,
    MSP_BEAMSPOTTER_GET_FIX = 197,
    MSP_BEAMSPOTTER_ECHO_TEST = 198
} mspBeamspotterCommandID_t;

// Configuration limits

#define BEAMSPOTTER_FREQUENCY_MIN       (1)
#define BEAMSPOTTER_FREQUENCY_MAX       (100)

// Status codes

typedef enum {
    BEAMSPOTTER_STATUS_SUCCESS = 0,
    BEAMSPOTTER_STATUS_INVALID_FREQUENCY = 1,
    BEAMSPOTTER_STATUS_HARDWARE_ERR = 2,
    BEAMSPOTTER_STATUS_TIMEOUT = 3
} BeamspotterStatus_t;

// Message structures

#pragma pack(push, 1)

// GET_CONFIG request payload
typedef struct {
} BeamspotterGetConfigRequest_t;

// GET_CONFIG response payload
typedef struct {
    uint8_t status;    // 0: Success, >0: Error code
    uint8_t frequency; // Update frequency in Hz (1-100)
} BeamspotterConfigResponse_t;

// SET_CONFIG request payload
typedef struct {
    uint8_t frequency; // Update frequency in Hz (1-100)
} BeamspotterSetConfigRequest_t;

// SET_CONFIG response payload
typedef struct {
    uint8_t status;    // 0: Success, >0: Error code
} BeamspotterSetConfigResponse_t;

// GET_FIX response payload
typedef struct {
    uint8_t status;    // 0: Success, >0: Error code
    uint8_t hasFix;    // 0: No fix, 1: Fix acquired
    uint32_t x;           // X coordinate of the laser beam position
    uint32_t y;           // Y coordinate of the laser beam position
} BeamspotterFixResponse_t;

// ECHO_TEST request/response payload
typedef struct {
    uint8_t size;     // Size of data to echo (1-32 bytes)
    uint8_t data[32]; // Data to be echoed
} BeamspotterEchoRequest_t;

typedef struct {
    uint8_t status;   // 0: Success, >0: Error code
    uint8_t size;     // Size of echoed data (1-32 bytes)
    uint8_t data[32]; // Echoed data
} BeamspotterEchoResponse_t;

#pragma pack(pop)
