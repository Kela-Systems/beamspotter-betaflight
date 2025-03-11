#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "platform.h"

#ifdef USE_BEAMSPOTTER

#include "build/debug.h"
#include "common/maths.h"
#include "drivers/time.h"
#include "io/serial.h"
#include "fc/runtime_config.h"

#include "msp/msp.h"
#include "msp/msp_serial.h"
#include "msp/msp_protocol_beamspotter.h"

#include "sensors/sensors.h"
#include "sensors/beamspotter.h"

// Constants

#define MSP_V1_PREAMBLE1                ('$')
#define MSP_V1_PREAMBLE2                ('M')
#define MSP_V1_PREAMBLE3                ('<')
#define MSP_V1_DIRECTION_TO_SENSOR      (0x00)
#define MSP_V1_DIRECTION_FROM_SENSOR    (0x01)

#define DEFAULT_CONFIG_FREQUENCY_HZ     (10)
#define DEFAULT_CONFIG_PORT_IDENTIFIER  (SERIAL_PORT_USART6)

// Global variables

beamspotter_t beamspotter;

static serialPort_t *beamspotterPort = NULL;

// Internal Functions

static bool beamspotterDetect(beamspotterDev_t * dev, uint8_t beamspotterHardwareToUse)
{
    UNUSED(dev);

    // TODO: Add detection logic here
    
    const serialPortConfig_t *beamspotterPortConfig = findSerialPortConfig(FUNCTION_BEAMSPOTTER);
    if (NULL == beamspotterPortConfig) {
        return false;
    }

    beamspotter.config.portIdentifier = beamspotterPortConfig->identifier;

    beamspotterPort = openSerialPort(beamspotterPortConfig->identifier, FUNCTION_BEAMSPOTTER, NULL, NULL, 115200, MODE_RXTX, SERIAL_NOT_INVERTED);
    if (NULL == beamspotterPort) {
        return false;
    }

    detectedSensors[SENSOR_INDEX_BEAMSPOTTER] = beamspotterHardwareToUse;
    sensorsSet(SENSOR_BEAMSPOTTER);

    return true;
}

// static void beamspotterSendMspCommand(uint8_t command, uint8_t *data, uint8_t dataSize) {
//     // Send an MSP command to the beamspotter sensor
//     mspSerialPush(beamspotter.config.portIdentifier, command, data, dataSize, MSP_DIRECTION_REQUEST, MSP_V1);
// }

static void beamspotterSendMSPCommand(uint8_t command, uint8_t *data, uint8_t dataSize) {
    // Send the MSP message header
    serialWrite(beamspotterPort, MSP_V1_PREAMBLE1);
    serialWrite(beamspotterPort, MSP_V1_PREAMBLE2);
    serialWrite(beamspotterPort, MSP_V1_DIRECTION_TO_SENSOR);
    serialWrite(beamspotterPort, dataSize);
    serialWrite(beamspotterPort, command);

    // Send the MSP message payload
    if (dataSize > 0) {
        serialWriteBuf(beamspotterPort, data, dataSize);
    }

    // Calculate the checksum
    uint8_t checksum = dataSize ^ command;
    for (uint8_t i = 0; i < dataSize; i++) {
        checksum += data[i];
    }

    // Send the MSP message checksum
    serialWrite(beamspotterPort, checksum);
}

// static void beamspotterGetConfig(void) {
//     // Request configuration from the sensor (no payload needed)
//     beamspotterSendMSPCommand(MSP_BEAMSPOTTER_GET_CONFIG, NULL, 0);
// }

static void beamspotterSetConfig(uint8_t frequency) {
    // Set the configuration on the sensor
    beamspotterSendMSPCommand(MSP_BEAMSPOTTER_SET_CONFIG, &frequency, sizeof(frequency));
}

static void beamspotterGetFix(void) {
    // Request a fix from the sensor (no payload needed)
    beamspotterSendMSPCommand(MSP_BEAMSPOTTER_GET_FIX, NULL, 0);
}

// static void beamspotterEchoTest(void) {
//     // Request an echo test from the sensor (no payload needed)
//     beamspotterSendMSPCommand(MSP_BEAMSPOTTER_ECHO_TEST, NULL, 0);
// }

// External Functions

bool beamspotterInit(void) {
    if (!beamspotterDetect(&beamspotter.dev, BEAMSPOTTER_TRIEYE)) {
        return false;
    }

    beamspotter.dev.init(&beamspotter.dev);
    
    // Set default configuration
    beamspotter.config.frequency = DEFAULT_CONFIG_FREQUENCY_HZ;
    beamspotter.config.portIdentifier = DEFAULT_CONFIG_PORT_IDENTIFIER;

    // Configure the sensor with the default configuration
    beamspotterSetConfig(beamspotter.config.frequency);
    
    // Mark as initialized
    beamspotter.isInitialized = true;
    
    return true;
}

void beamspotterUpdate(void) {
    // This function is called by the beamspotter task
    // Request a fix from the sensor
    beamspotterGetFix();
}

void beamspotterProcess(void) {
    // This function is called by the beamspotter task
    // Check if it's time to update based on the configured frequency
    if (beamspotter.isInitialized) {
        beamspotterUpdate();
    }
}

bool beamspotterIsHealthy(void) {
    // Check if the sensor is responding and healthy
    const timeMs_t currentTimeMs = millis();
    const bool timeout = (currentTimeMs - beamspotter.lastUpdateMs) > BEAMSPOTTER_REQUEST_TIMEOUT_MS;
    
    return beamspotter.isInitialized && !timeout && (beamspotter.lastStatus == BEAMSPOTTER_STATUS_SUCCESS);
}

bool beamspotterHasFix(void) {
    return beamspotter.hasFix;
}

bool beamspotterGetCoordinates(beamspotterCoordinates_t *coordinates) {
    // Validate parameters
    if (coordinates == NULL) {
        return false;
    }

    if (!beamspotter.hasFix) {
        return false;
    }

    *coordinates = beamspotter.coordinates;

    return true;
}

#endif /* USE_BEAMSPOTTER */
