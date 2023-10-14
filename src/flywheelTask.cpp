/* clang-format off */
/* The FreeRTOS.h include needs to happen before the others can be included*/
#include <freertos/FreeRTOS.h>
/* clang-format off */
#include "freertos/queue.h"
#include <freertos/task.h>

/* These are the only bits needed to spin up the flywheels
 * The pusher should set the Fire request bit when it wants the flywheels up to speed, then wait for its Up to Speed bit to be set */
const uint32_t cFlywheelTaskNotifyFireRequestBitFlag = 0x01;
/* Once the pusher has successfully fired all darts, it should clear the Fire Request bit to let the flywheels idle, then spindown (or spin up again) */
const uint32_t cFlywheelTaskNotifyClearFireRequestBitFlag = 0x02;
/* This is an internal bit that will kick the PID loop; calling this from any other task will cause undefined behvior in the PID loop*/
const uint32_t cFlywheelTaskNotifyTimerBitFlag = 0x04;
/* This task will normally govern spindown and this should not need to be used
 * This bit is for error cases where something else has happened, like a pusher jam*/
const uint32_t cFlywheelTaskNotifySpindownBitFlag = 0x08;
/* This task will normally govern idle state after darts are fired without needing this bit to be set
 * This bit is for competitive modes or short idle buttons initiated from the UX*/
const uint32_t cFlywheelTaskNotifyIdleBitFlag = 0x10;
const uint32_t cFlywheelTaskNotifyClearIdleBitFlag = 0x20;
/* This task will normally govern rev states by the fire request bit without needing this bit to be set
 * This bit is for seperate rev triggers (or takeup) */
const uint32_t cFlywheelTaskNotifyRevBitFlag = 0x40;
const uint32_t cFlywheelTaskNotifyClearRevBitFlag = 0x80;
/* These are the UX bits. Setting them will do the named action once; no need to clear them, as this task will do that after every notification*/
const uint32_t cFlywheelTaskNotifyRpmNextBitFlag = 0x100;
const uint32_t cFlywheelTaskNotifyRpmPreviousBitFlag = 0x200;
const uint32_t cFlywheelTaskNotifyRpmSetpoint1BitFlag = 0x400;
const uint32_t cFlywheelTaskNotifyRpmSetpoint2BitFlag = 0x800;
const uint32_t cFlywheelTaskNotifyRpmSetpoint3BitFlag = 0x1000;

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

        /* Spindown has the highest priority in case something needs to abort flywheel motion*/
        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifySpindownBitFlag) != 0) {
            // spindown
        }
        
        // TODO(Daehder) If we get both rev and clear, should we rev? clear?
        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyClearFireRequestBitFlag) != 0) {
            // clear rev state
        }
        else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyFireRequestBitFlag) != 0) {
            // rev
            // Cancel spindown timer
        } 
        
        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyRevBitFlag) != 0) {
            // rev
            // Cancel spindown timer
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyClearRevBitFlag) != 0) {
            // clear rev state
        } 
        
        if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyIdleBitFlag) != 0) {
            // Idle
            // Kick Spindown timer
        } else if ((flywheelTaskNotifyBits & cFlywheelTaskNotifyClearIdleBitFlag) != 0) {
            // Idle
            // Kick Spindown timer
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