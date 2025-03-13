#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "platform.h"

#define USE_BEAMSPOTTER
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

#define USE_CLI_DEBUG_PRINT
#include "cli/cli_debug_print.h"

// Constants

#define MSP_V1_PREAMBLE1                ('$')
#define MSP_V1_PREAMBLE2                ('M')
#define MSP_V1_DIRECTION_TO_SENSOR      ('>')
#define MSP_V1_DIRECTION_FROM_SENSOR    ('<')

#define DEFAULT_CONFIG_FREQUENCY_HZ     (10)
#define DEFAULT_CONFIG_PORT_IDENTIFIER  (SERIAL_PORT_UART4)

#define MSP_MAX_PAYLOAD_SIZE            (128)

// Structures

#pragma pack(push, 1)
typedef struct mspMessageHeader_s {
    uint16_t preamble;
    uint8_t direction;
    uint8_t dataSize;
    uint8_t command;
} mspMessageHeader_t;
#pragma pack(pop)

typedef struct mspMessage_s {
    mspMessageHeader_t header;
    uint8_t payload[MSP_MAX_PAYLOAD_SIZE];
    uint8_t checksum;
} mspMessage_t;

// Global variables

beamspotter_t beamspotter;

static serialPort_t *beamspotterPort = NULL;

// Internal Functions

static bool beamspotterDetect(beamspotterDev_t * dev, uint8_t beamspotterHardwareToUse)
{
    UNUSED(dev);

    // TODO: Add detection logic here
    
    // const serialPortConfig_t *beamspotterPortConfig = findSerialPortConfig(FUNCTION_BEAMSPOTTER);
    // if (NULL == beamspotterPortConfig) {
    //     return false;
    // }

    // beamspotter.config.portIdentifier = beamspotterPortConfig->identifier;
    beamspotter.config.portIdentifier = DEFAULT_CONFIG_PORT_IDENTIFIER;

    // beamspotterPort = openSerialPort(beamspotter.config.portIdentifier, FUNCTION_BEAMSPOTTER, NULL, NULL, 115200, MODE_RXTX, SERIAL_NOT_INVERTED);
    beamspotterPort = openSerialPort(beamspotter.config.portIdentifier, FUNCTION_NONE, NULL, NULL, 115200, MODE_RXTX, SERIAL_NOT_INVERTED);
    if (NULL == beamspotterPort) {
        return false;
    }

    sensorsSet(SENSOR_BEAMSPOTTER);
    detectedSensors[SENSOR_INDEX_BEAMSPOTTER] = beamspotterHardwareToUse;

    return true;
}

// static void beamspotterSendMspCommand(uint8_t command, uint8_t *data, uint8_t dataSize) {
//     // Send an MSP command to the beamspotter sensor
//     mspSerialPush(beamspotter.config.portIdentifier, command, data, dataSize, MSP_DIRECTION_REQUEST, MSP_V1);
// }

static uint8_t beamspotterCalculateChecksum(uint8_t dataSize, uint8_t command, uint8_t *data) {
    uint8_t checksum = 0;

    checksum = dataSize ^ command;
    for (uint8_t i = 0; i < dataSize; i++) {
        checksum ^= data[i];
    }

    return checksum;
}

static void beamspotterSendMSPMessage(uint8_t command, uint8_t *data, uint8_t dataSize) {
    // Send the MSP message header
    mspMessageHeader_t header = {
        .preamble = MSP_V1_PREAMBLE1 | (MSP_V1_PREAMBLE2 << 8),
        .direction = MSP_V1_DIRECTION_TO_SENSOR,
        .dataSize = dataSize,
        .command = command
    };
    serialWriteBuf(beamspotterPort, (uint8_t *)&header, sizeof(header));

    // Send the MSP message payload
    if ((dataSize > 0) && (data != NULL)) {
        serialWriteBuf(beamspotterPort, data, dataSize);
    }

    // Send the MSP message checksum
    uint8_t checksum = beamspotterCalculateChecksum(dataSize, command, data);
    serialWrite(beamspotterPort, checksum);
}

static void beamspotterSerialReadBuffer(serialPort_t *serialPort, uint8_t *buffer, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        buffer[i] = serialRead(serialPort);
    }
}

static bool beamspotterReadPreamble(serialPort_t *serialPort, uint16_t *preamble) {
    uint8_t preamble1 = serialRead(serialPort);
    while (preamble1 != MSP_V1_PREAMBLE1) {
        if (!serialRxBytesWaiting(serialPort)) {
            return false;
        }

        preamble1 = serialRead(serialPort);
    }

    uint8_t preamble2 = serialRead(serialPort);
    if (preamble2 != MSP_V1_PREAMBLE2) {
        if (!serialRxBytesWaiting(serialPort)) {
            return false;
        }

        preamble2 = serialRead(serialPort);
    }

    *preamble = (preamble1 << 8) | preamble2;

    return true;
}

static bool beamspotterReceiveMSPMessage(mspMessage_t *message) {
    uint16_t preamble = 0;

    if (!beamspotterReadPreamble(beamspotterPort, &preamble)) {
        return false;
    }

    message->header.preamble = preamble;

    beamspotterSerialReadBuffer(beamspotterPort, (uint8_t *)&message->header.direction, sizeof(message->header.direction));
    beamspotterSerialReadBuffer(beamspotterPort, (uint8_t *)&message->header.dataSize, sizeof(message->header.dataSize));
    beamspotterSerialReadBuffer(beamspotterPort, (uint8_t *)&message->header.command, sizeof(message->header.command));

    // Receive the MSP message payload
    if (message->header.dataSize > 0) {
        beamspotterSerialReadBuffer(beamspotterPort, message->payload, message->header.dataSize);
    }

    // Receive the MSP message checksum
    message->checksum = serialRead(beamspotterPort);

    // Validate the MSP message checksum
    uint8_t calculatedChecksum = beamspotterCalculateChecksum(message->header.dataSize, message->header.command, message->payload);
    if (message->checksum != calculatedChecksum) {
        return false;
    }

    return true;
}

// static void beamspotterGetConfig(void) {
//     // Request configuration from the sensor (no payload needed)
//     beamspotterSendMSPMessage(MSP_BEAMSPOTTER_GET_CONFIG, NULL, 0);
// }

static void beamspotterSetConfig(uint8_t frequency) {
    // Set the configuration on the sensor
    uint8_t data[] = { frequency };
    beamspotterSendMSPMessage(MSP_BEAMSPOTTER_SET_CONFIG, data, sizeof(data));
}

static uint32_t ntohl(uint32_t value) {
    return __builtin_bswap32(value);
}

static void beamspotterGetFix(void) {
    // Request a fix from the sensor (no payload needed)
    beamspotterSendMSPMessage(MSP_BEAMSPOTTER_GET_FIX, NULL, 0);

    // Receive the response from the sensor
    mspMessage_t mspResponseMessage = {0};
    if (!beamspotterReceiveMSPMessage(&mspResponseMessage)) {
        return;
    }

    // Validate the response from the sensor
    if (mspResponseMessage.header.command != MSP_BEAMSPOTTER_GET_FIX) {
        return;
    }
    if (mspResponseMessage.header.dataSize != sizeof(BeamspotterFixResponse_t)) {
        return;
    }
    // Extract the response from the sensor
    BeamspotterFixResponse_t *beamspotterResponse = (BeamspotterFixResponse_t *)mspResponseMessage.payload;

    // Update the beamspotter state
    beamspotter.hasFix = beamspotterResponse->hasFix;
    beamspotter.coordinates.x = ntohl(beamspotterResponse->x);
    beamspotter.coordinates.y = ntohl(beamspotterResponse->y);
}

// static void beamspotterEchoTest(void) {
//     // Request an echo test from the sensor (no payload needed)
//     beamspotterSendMSPMessage(MSP_BEAMSPOTTER_ECHO_TEST, NULL, 0);
// }

// External Functions

bool beamspotterInit(void) {
    beamspotter.isInitialized = false;

    // cliDebugPrintf("beamspotterInit\n");

    if (!beamspotterDetect(&beamspotter.dev, BEAMSPOTTER_TRIEYE)) {
        return false;
    }

    // beamspotter.dev.init(&beamspotter.dev);
    
    // Set default configuration
    beamspotter.config.frequency = DEFAULT_CONFIG_FREQUENCY_HZ;
    beamspotter.config.portIdentifier = DEFAULT_CONFIG_PORT_IDENTIFIER;

    // Configure the sensor with the default configuration
    beamspotterSetConfig(beamspotter.config.frequency);

    // Mark as initialized
    beamspotter.isInitialized = true;

    return true;
}

// This function is called by the beamspotter task
void beamspotterUpdate(void) {
    // Request a fix from the sensor
    beamspotterGetFix();
}

// This function is called by the beamspotter task
// Check if it's time to update based on the configured frequency
void beamspotterProcess(void) {
    beamspotterUpdate();
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

    // if (!beamspotter.hasFix) {
    //     return false;
    // }

    *coordinates = beamspotter.coordinates;

    return true;
}

#endif /* USE_BEAMSPOTTER */
