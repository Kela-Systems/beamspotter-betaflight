#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "drivers/beamspotter/beamspotter_trieye.h"
#include "sensors/beamspotter.h"

// Internal Functions

static void beamspotterTrieyeInit(beamspotterDev_t *dev) {
    UNUSED(dev);
}

static void beamspotterTrieyeUpdate(beamspotterDev_t *dev) {
    UNUSED(dev);
}

static void beamspotterTrieyeRead(beamspotterDev_t *dev) {
    UNUSED(dev);
}

// External Functions

bool beamspotterTrieyeDetect(beamspotterDev_t *dev) {
    dev->init = &beamspotterTrieyeInit;
    dev->update = &beamspotterTrieyeUpdate;
    dev->read = &beamspotterTrieyeRead;

    return true;
}

