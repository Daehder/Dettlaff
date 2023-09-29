#include "freertos/queue.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const uint32_t cFlywheelTaskNotifyTimerBitFlag = 0x01;
const uint32_t cFlywheelTaskNotifySpindownBitFlag = 0x02;
const uint32_t cFlywheelTaskNotifyIdleBitFlag = 0x04;
const uint32_t cFlywheelTaskNotifyRevBitFlag = 0x08;
const uint32_t cFlywheelTaskNotifyRpmNextBitFlag = 0x10;
const uint32_t cFlywheelTaskNotifyRpmPreviousBitFlag = 0x20;
const uint32_t cFlywheelTaskNotifyRpmSetpoint1BitFlag = 0x40;
const uint32_t cFlywheelTaskNotifyRpmSetpoint2BitFlag = 0x80;
const uint32_t cFlywheelTaskNotifyRpmSetpoint3BitFlag = 0x100;

const uint32_t cWaitForZerothNotification = 0;
const uint32_t cDontClearBits = 0;
const uint32_t cClearAllBits = UINT32_MAX;
// const uint32_t cClearAllButRevAndSetpointBits = !((cFlywheelTaskNotifyIdleBitFlag | cFlywheelTaskNotifyRevBitFlag) | (cFlywheelTaskNotifyRpmSetpoint1BitFlag | cFlywheelTaskNotifyRpmSetpoint2BitFlag | cFlywheelTaskNotifyRpmSetpoint3BitFlag));

void flywheelTask(void* arg)
{
    // Ensure that the PID timer gets kicked off on the first run through
    uint32_t flywheelTaskNotifyBits = cFlywheelTaskNotifyTimerBitFlag;

    while (true) // Loop indefinitely
    {
        xTaskNotifyWaitIndexed(cWaitForZerothNotification,
            cDontClearBits,
            cClearAllBits,
            &flywheelTaskNotifyBits,
            portMAX_DELAY);

        // TODO(Daehder) how does the pusher inform us that it's done pushing darts and to idle/spindown?

        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRevBitFlag) != 0) {
            // rev
            // Cancel spindown timer
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyIdleBitFlag) != 0) {
            // Idle
            // Kick Spindown timer
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifySpindownBitFlag) != 0) {
            // spindown
        }

        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRpmNextBitFlag) != 0) {
            // set next rpm
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRpmPreviousBitFlag) != 0) {
            // set previous
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRpmSetpoint1BitFlag) != 0) {
            // set rpm setpoint 1
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRpmSetpoint2BitFlag) != 0) {
            // set rpm setpoint 2
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRpmSetpoint3BitFlag) != 0) {
            // set rpm setpoint 2
        }

        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyTimerBitFlag) != 0) {
            // Update the PID loop and update the flywheel speed
            // Kick the timer again
        }
    }
}