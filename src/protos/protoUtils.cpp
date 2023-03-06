#include "protoUtils.h"

#include "test.h"

void TestProtos(void)
{
    unsigned bufferLen = 128;
    uint8_t buffer[bufferLen];
    bool status;
    Blaster test1 = Blaster_init_zero;

    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    char name[] = "Detlaff";

    strncpy(test1.blasterName, name, 20);
    test1.has_controlconfig = true;
    test1.controlconfig.triggerSwitchPin = 15;

    status = pb_encode(&outputStream, Blaster_fields, &test1);
    assert(status);
    size_t messageLength = outputStream.bytes_written;

    shell.printf("Proto Buffer is: |0x");
    for (int ndx = 0; ndx < messageLength; ndx++) {
        shell.printf(" %02x", buffer[ndx]);
    }
    shell.printf("|\n");

    std::string bufferString = std::string((char*)buffer, messageLength);
    Blaster received = Blaster_init_zero;

    ProtoDecodeFromString(&received, bufferString);

    assert(strncmp(test1.blasterName, received.blasterName, 20) == 0);
}

void ProtoDecodeFromString(Blaster* received, const std::string& input)
{
    pb_istream_t inStream = pb_istream_from_buffer((const pb_byte_t*)input.c_str(), input.length());

    bool status = pb_decode(&inStream, Blaster_fields, received);
    assert(status);

    shell.printf("Received blaster:\n");
    PrettyPrintBlaster(shell, *received);
}

void PrettyPrintHardwareVersion(SimpleSerialShell& printer, Blaster& blaster)
{
    printer.printf("Detlaff Board Version: %s\n",
        ((blaster.hardwareVersion == HardwareVersion_VERSION_0P1)          ? "0.1"
                : (blaster.hardwareVersion == HardwareVersion_VERSION_0P2) ? "0.2"
                : (blaster.hardwareVersion == HardwareVersion_VERSION_0P3) ? "0.3"
                : (blaster.hardwareVersion == HardwareVersion_VERSION_0P4) ? "0.4"
                                                                           : "Invalid"));
}

void PrettyPrintControlConfig(SimpleSerialShell& printer, Blaster& blaster)
{
    if (blaster.has_controlconfig) {
        printer.printf("Control Config\n");
        ControlConfig controlConfig = blaster.controlconfig;
        printer.printf("\tTrigger Switch Pin: %d\n", controlConfig.triggerSwitchPin);

        printer.printf("\tTrigger Switch Orientation: ");
        switch (controlConfig.TriggerSwitchOrientation) {
        case SwitchOrientation_SWITCH_NORMALLY_OPEN:
            printer.printf("Normally Open");
            break;
        case SwitchOrientation_SWITCH_NORMALLY_CLOSED:
            printer.printf("Normally Closed");
            break;
        case SwitchOrientation_SWITCH_ORIENTATION_INVALID:
            // Fall through to default
        default:
            printer.printf("Invalid");
            break;
        }
        printer.printf("\n");

        printer.printf("\tRev Switch Pin: %d\n", controlConfig.optRevSwitchPin);

        printer.printf("\tRev Switch Orientation: ");
        switch (controlConfig.optRevSwitchOrientation) {
        case SwitchOrientation_SWITCH_NORMALLY_OPEN:
            printer.printf("Normally Open");
            break;
        case SwitchOrientation_SWITCH_NORMALLY_CLOSED:
            printer.printf("Normally Closed");
            break;
        case SwitchOrientation_SWITCH_ORIENTATION_INVALID:
            // Fall through to default
        default:
            printer.printf("Invalid");
            break;
        }
        printer.printf("\n");

        printer.printf("\tFire Mode Switch Pin: %d\n", controlConfig.fireModeCycleButtonPin);
    } else {
        printer.printf("No Control Config\n");
    }
}

void PrettyPrintControlParams(SimpleSerialShell& printer, Blaster& blaster)
{
    if (blaster.has_controlParams) {
        ControlParams controlParams = blaster.controlParams;
        printer.printf("Control Params\n");

        printer.printf("\tFiringModes:\n");
        if (controlParams.fireModesArray_count == 0) {
            printer.printf("\t\tWarning: No Firing modes");
        }
        for (int ndx = 0; ndx < controlParams.fireModesArray_count; ndx++) {
            FireModeEntry fireModeEntry = controlParams.fireModesArray[ndx];

            printer.printf("\tEntry %d", ndx);
            printer.printf("\t\tBurst Length %d, Burst Type: ", fireModeEntry.burstLength);
            switch (fireModeEntry.type) {
            case PusherBurstType_PUSHER_BURST_STOP:
                printer.printf("Stop");
                break;
            case PusherBurstType_PUSHER_BURST_COMPLETE:
                printer.printf("Complete");
                break;
            case PusherBurstType_PUSHER_BURST_COUNT:
                printer.printf("Count");
                break;
            case PusherBurstType_PUSHER_BURST_INVALID:
            // Fall through to default
            default:
                printer.printf("Invalid");
                break;
            }
            printer.printf(" RPMs: |");
            for (int rpmNdx = 0; rpmNdx < fireModeEntry.rpm_count; rpmNdx++) {
                printer.printf(" %d |", fireModeEntry.rpm[rpmNdx]);
            }
            printer.printf("\n");
        }
    } else {
        printer.printf("No Control Params\n");
    }
}

void PrettyPrintFlywheelConfig(SimpleSerialShell& printer, Blaster& blaster)
{
    if (blaster.has_flywheelConfig) {
        printer.printf("Flywheel Config\n");
        FlywheelConfig flywheelConfig = blaster.flywheelConfig;

        printer.printf("Number of motors: %d\n", flywheelConfig.numMotors);

        printer.printf("\tMotor Control Method: %s\n",
            ((flywheelConfig.controlMethod == MotorControlMethod_ESC_DSHOT)            ? "DShot"
                    : (flywheelConfig.controlMethod == MotorControlMethod_ESC_PWM)     ? "PWM"
                    : (flywheelConfig.controlMethod == MotorControlMethod_BRUSHED_PWM) ? "Brushed PWM"
                    : (flywheelConfig.controlMethod == MotorControlMethod_BRUSHED_PWM) ? "Brushed Binary"
                                                                                       : "Invalid"));

        printer.printf("\tDShot %s\n",
            ((flywheelConfig.dshotMode == DshotModes_DSHOT_150)           ? "150"
                    : (flywheelConfig.dshotMode == DshotModes_DSHOT_300)  ? "300"
                    : (flywheelConfig.dshotMode == DshotModes_DSHOT_600)  ? "600"
                    : (flywheelConfig.dshotMode == DshotModes_DSHOT_1200) ? "1200"
                                                                          : "Off"));

        printer.printf("\tTelemetry Method %s\n",
            ((flywheelConfig.telemetryMethod == TelemMethod_BIDIRECTIONAL_DSHOT) ? "DSHOT"
                    : (flywheelConfig.telemetryMethod == TelemMethod_SERIAL)     ? "Serial"
                    : (flywheelConfig.telemetryMethod == TelemMethod_TACHOMETER) ? "Techometer"
                                                                                 : "No Telemetry"));

        printer.printf("\tMotor Configs:\n");
        if (flywheelConfig.motorConfig_count == 0) {
            printer.printf("\t\tWarning: No Motors Configured");
        }
        for (int ndx = 0; ndx < flywheelConfig.motorConfig_count; ndx++) {
            MotorConfig motorConfig = flywheelConfig.motorConfig[ndx];

            printer.printf("Motor %d: KV: %d, Idle RPM %d, Firing Threshold %d%\n",
                ndx, motorConfig.motorKv, motorConfig.idleRPM, motorConfig.firingThresholdPercentage);
        }
    } else {
        printer.printf("No Flywheel Config\n");
    }
}

void PrettyPrintFlywheelParams(SimpleSerialShell& printer, Blaster& blaster)
{
    printer.printf("Flywheel Params of type: ");
    if (blaster.which_flywheelParams == 0) {
        printer.printf("Closed Loop\n");

        ClosedLoopFlywheelParams closedLoopFlywheelParams = blaster.flywheelParams.closedLoopFlywheelParams;

        printer.printf("\tIdle RPM: %d, Idle Time: %d ms, Spindown Speed: %d\n",
            closedLoopFlywheelParams.idleRpm,
            closedLoopFlywheelParams.idleTime_ms,
            closedLoopFlywheelParams.spindownSpeed);
    } else {
        printer.printf("Open Loop\n");

        OpenLoopFlywheelParams openLoopFlywheelParams = blaster.flywheelParams.openLoopFlywheelParams;

        printer.printf("\tIdle Duty Cycle: %d, Idle Time: %d ms, Minimum Rev Time: %d ms, Minimum Idle Rev Time: %d ms, Spindown Speed: %d\n",
            openLoopFlywheelParams.idleDutyCycle,
            openLoopFlywheelParams.idleTime_ms,
            openLoopFlywheelParams.minimumRevTime_ms,
            openLoopFlywheelParams.minimumIdleRevTime_ms,
            openLoopFlywheelParams.spindownSpeed);
    }
}

void PrettyPrintBlaster(SimpleSerialShell& printer, Blaster& blaster)
{
    printer.printf("Name: %s\n", blaster.blasterName);
    PrettyPrintHardwareVersion(printer, blaster);

    PrettyPrintControlConfig(printer, blaster);
    PrettyPrintControlParams(printer, blaster);
    PrettyPrintFlywheelConfig(printer, blaster);
    PrettyPrintFlywheelConfig(printer, blaster);
}

// Do I need to past this by reference?
void ProtoEncodeToString(Blaster& received, std::string* output)
{

    // Do we need this intermediary buffer? I don't think we can directly interact with the cstring from ble
    // We also kinda need to interface with the BLE library in case we need to send a message longer than 512
    // But blaster shouldn't be longer than 512; the longer stuff like OTA, esc flashing, etc will probably have its own characteristics
    // TL;DR: This may not work well as a proto-only helper function
    unsigned bufferLen = 512;
}
