#include <Arduino.h>
#include <ArduinoOTA.h>
#define BOUNCE_LOCK_OUT // improves rev responsiveness at the risk of spurious signals from noise
#include "Bounce2.h"
#include "DShotRMT.h"
#include "ESP32Servo.h"
#include "types.h"
#include "boards_config.cpp"

#include <SimpleSerialShell.h>

#include "Pushers/solenoid.h"

// Configuration Variables

char wifiSsid[32] = "ssid";
char wifiPass[63] = "pass";
uint32_t revRPM = 50000;
uint32_t idleRPM = 1000;
uint32_t idleTime_ms = 30000; // how long to idle the flywheels for
uint32_t motorKv = 2550;
pins_t pins = pins_v0_4_noid;
// Options:
// pins_v0_4_n20
// pins_v0_4_noid
// pins_v0_3_n20
// pins_v0_3_noid
// pins_v0_2
// pins_v0_1
// _noid means use the flywheel output to drive a solenoid pusher
pusherType_t pusherType = PUSHER_SOLENOID_OPENLOOP;
// PUSHER_MOTOR_CLOSEDLOOP or PUSHER_SOLENOID_OPENLOOP
uint16_t burstLength = 3;
uint8_t bufferMode = 1;
// 0 = stop firing when trigger is released
// 1 = complete current burst when trigger is released
// 2 = fire as many bursts as trigger pulls
// for full auto, set burstLength high (50+) and bufferMode = 0
uint16_t firingDelay_ms = 200; // delay to allow flywheels to spin up before pushing dart
uint16_t solenoidExtendTime_ms = 22;
uint16_t solenoidRetractTime_ms = 78;

// Advanced Configuration Variables

uint16_t pusherStallTime_ms = 500;    // for PUSHER_MOTOR_CLOSEDLOOP, how long do you run the motor without seeing an update on the cycle control switch before you decide the motor is stalled?
uint16_t spindownSpeed = 1;           // higher number makes the flywheels spin down faster when releasing the rev trigger
bool revSwitchNormallyClosed = false; // should we invert rev signal?
bool triggerSwitchNormallyClosed = false;
bool cycleSwitchNormallyClosed = false;
uint16_t debounceTime = 25; // ms
char AP_SSID[32] = "Dettlaff";
char AP_PW[32] = "KellyIndu";
dshot_mode_t dshotMode = DSHOT300; // DSHOT_OFF to fall back to servo PWM
uint16_t targetLoopTime_us = 1000; // microseconds

// End Configuration Variables

uint32_t loopStartTimer_us = micros();
uint16_t loopTime_us = targetLoopTime_us;
uint32_t time_ms = millis();
uint32_t lastRevTime_ms = 0; // for calculating idling
uint32_t pusherTimer_ms = 0;
uint32_t targetRPM = 0;
uint32_t throttleValue = 0;    // scale is 0 - 1999
uint32_t batteryADC_mv = 1340; // voltage at the ADC, after the voltage divider
uint16_t shotsToFire = 0;
flywheelState_t flywheelState = STATE_IDLE;
bool firing = false;
bool closedLoopFlywheels = false;
uint32_t scaledMotorKv = motorKv * 11; // motor kv * battery voltage resistor divider ratio

const uint32_t maxThrottle = 1999;

Bounce2::Button revSwitch = Bounce2::Button();
Bounce2::Button triggerSwitch = Bounce2::Button();
Bounce2::Button cycleSwitch = Bounce2::Button();
Bounce2::Button button = Bounce2::Button();

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

DShotRMT dshot1(pins.esc1, RMT_CHANNEL_1);
DShotRMT dshot2(pins.esc2, RMT_CHANNEL_2);
DShotRMT dshot3(pins.esc3, RMT_CHANNEL_3);
DShotRMT dshot4(pins.esc4, RMT_CHANNEL_4);

// void WiFiInit();

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");

  shell.attach(Serial);
  shell.addCommand(F("Solenoid"), shellCommandSolenoid);

  if (pins.flywheel)
  {
    pinMode(pins.flywheel, OUTPUT);
    digitalWrite(pins.flywheel, HIGH);
  }
  // WiFiInit();
  if (pins.revSwitch)
  {
    revSwitch.attach(pins.revSwitch, INPUT_PULLUP);
    revSwitch.interval(debounceTime);
    revSwitch.setPressedState(revSwitchNormallyClosed);
  }
  if (pins.triggerSwitch)
  {
    triggerSwitch.attach(pins.triggerSwitch, INPUT_PULLUP);
    triggerSwitch.interval(debounceTime);
    triggerSwitch.setPressedState(triggerSwitchNormallyClosed);
  }
  if (pins.cycleSwitch)
  {
    cycleSwitch.attach(pins.cycleSwitch, INPUT_PULLUP);
    cycleSwitch.interval(debounceTime);
    cycleSwitch.setPressedState(cycleSwitchNormallyClosed);
  }
  if (pins.pusher)
  {
    pinMode(pins.pusher, OUTPUT);
    digitalWrite(pins.pusher, LOW);
    pinMode(pins.pusherBrake, OUTPUT);
    digitalWrite(pins.pusherBrake, LOW);
  }
  if (dshotMode == DSHOT_OFF)
  {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    servo1.setPeriodHertz(200);
    servo2.setPeriodHertz(200);
    servo3.setPeriodHertz(200);
    servo4.setPeriodHertz(200);
    servo1.attach(pins.esc1);
    servo2.attach(pins.esc2);
    servo3.attach(pins.esc3);
    servo4.attach(pins.esc4);
  }
  else
  {
    dshot1.begin(dshotMode, false); // bitrate & bidirectional
    dshot2.begin(dshotMode, false);
    dshot3.begin(dshotMode, false);
    dshot4.begin(dshotMode, false);
  }
}

void loop()
{
  loopStartTimer_us = micros();
  time_ms = millis();
  if (pins.revSwitch)
  {
    revSwitch.update();
  }
  if (pins.triggerSwitch)
  {
    triggerSwitch.update();
  }

  if (triggerSwitch.pressed())
  { // pressed and released are transitions, isPressed is for state
    if (bufferMode == 0)
    {
      shotsToFire = burstLength;
    }
    else if (bufferMode == 1)
    {
      if (shotsToFire < burstLength)
      {
        shotsToFire += burstLength;
      }
    }
    else if (bufferMode == 2)
    {
      shotsToFire += burstLength;
    }
  }
  else if (triggerSwitch.released())
  {
    if (bufferMode == 0)
    {
      shotsToFire = 0;
    }
  }

  switch (flywheelState)
  {

  case STATE_IDLE:
    if (triggerSwitch.isPressed() || revSwitch.isPressed())
    {
      targetRPM = revRPM;
      lastRevTime_ms = time_ms;
      flywheelState = STATE_ACCELERATING;
    }
    // idle flywheels
    else if (time_ms < lastRevTime_ms + idleTime_ms && lastRevTime_ms > 0)
    {
      targetRPM = idleRPM;
    }
    // stop flywheels
    else
    {
      targetRPM = 0;
    }
    break;

  case STATE_ACCELERATING:
    if ((closedLoopFlywheels) || (!closedLoopFlywheels && time_ms > lastRevTime_ms + firingDelay_ms))
    {
      flywheelState = STATE_FULLSPEED;
    }
    break;

  case STATE_FULLSPEED:
    if (!revSwitch.isPressed() && shotsToFire == 0 && !firing)
    {
      flywheelState = STATE_IDLE;
    }
    else if (shotsToFire > 0 || firing)
    {
      switch (pusherType)
      {

      case PUSHER_MOTOR_CLOSEDLOOP:
        cycleSwitch.update();
        // start pusher stroke
        if (shotsToFire > 0 && !firing)
        {
          digitalWrite(pins.pusher, HIGH);
          digitalWrite(pins.pusherBrake, LOW);
          firing = true;
          pusherTimer_ms = time_ms;
        }
        // brake pusher
        else if (firing && shotsToFire == 0 && cycleSwitch.pressed())
        {
          digitalWrite(pins.pusher, HIGH);
          digitalWrite(pins.pusherBrake, HIGH);
          firing = false;
        }
        else if (firing && shotsToFire > 0 && cycleSwitch.released())
        {
          shotsToFire = shotsToFire - 1;
          pusherTimer_ms = time_ms;
        }
        // stall protection
        else if (firing && time_ms > pusherTimer_ms + pusherStallTime_ms)
        {
          digitalWrite(pins.pusher, LOW); // let pusher coast
          digitalWrite(pins.pusherBrake, LOW);
          shotsToFire = 0;
          firing = false;
          Serial.println("Pusher motor stalled!");
        }
        break;

      case PUSHER_SOLENOID_OPENLOOP:
        // extend solenoid
        if (shotsToFire > 0 && !firing && time_ms > pusherTimer_ms + solenoidRetractTime_ms)
        {
          digitalWrite(pins.pusher, HIGH);
          firing = true;
          shotsToFire -= 1;
          pusherTimer_ms = time_ms;
          Serial.println("solenoid extending");
        }
        // retract solenoid
        else if (firing && time_ms > pusherTimer_ms + solenoidRetractTime_ms)
        {
          digitalWrite(pins.pusher, LOW);
          firing = false;
          pusherTimer_ms = time_ms;
          Serial.println("solenoid retracting");
        }
        break;
      }
    }
    break;
  }

  if (closedLoopFlywheels)
  {
    // ray control code goes here
  }
  else
  {
    if (throttleValue == 0)
    {
      throttleValue = min(maxThrottle, maxThrottle * targetRPM / batteryADC_mv * 1000 / scaledMotorKv);
    }
    else
    {
      throttleValue = max(min(maxThrottle, maxThrottle * targetRPM / batteryADC_mv * 1000 / scaledMotorKv),
                          throttleValue - spindownSpeed);
    }
  }

  // send signal to ESCs
  if (dshotMode == DSHOT_OFF)
  {
    servo1.writeMicroseconds(throttleValue / 2 + 1000);
    servo2.writeMicroseconds(throttleValue / 2 + 1000);
    servo3.writeMicroseconds(throttleValue / 2 + 1000);
    servo4.writeMicroseconds(throttleValue / 2 + 1000);
  }
  else
  {
    dshot1.send_dshot_value(throttleValue + 48, NO_TELEMETRIC);
    dshot2.send_dshot_value(throttleValue + 48, NO_TELEMETRIC);
    dshot3.send_dshot_value(throttleValue + 48, NO_TELEMETRIC);
    dshot4.send_dshot_value(throttleValue + 48, NO_TELEMETRIC);
  }
  ArduinoOTA.handle();
  loopTime_us = micros() - loopStartTimer_us;
  if (loopTime_us > targetLoopTime_us)
  {
    Serial.print("loop over time, ");
    Serial.println(loopTime_us);
  }
  else
  {
    delayMicroseconds(max((long)(0), (long)(targetLoopTime_us - loopTime_us)));
  }

  shell.executeIfInput();
}

// void WiFiInit()
// {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(wifiSsid, wifiPass);
//   if (WiFi.waitForConnectResult() != WL_CONNECTED)
//   {
//     Serial.println("WiFi Connection Failed!");
//     WiFi.mode(WIFI_AP);
//     WiFi.softAP(AP_SSID, AP_PW);
//   }
//   else
//   {
//     Serial.print("WiFi Connected ");
//     Serial.println(wifiSsid);
//     ArduinoOTA.setHostname("Dettlaff");
//     /*
//         if(!MDNS.begin("dettlaff")) {
//           Serial.println("Error starting mDNS");
//           return;
//         }
//     */
//     Serial.println(WiFi.localIP());

//     // No authentication by default
//     // ArduinoOTA.setPassword("admin");
//   }

//   ArduinoOTA
//       .onStart([]()
//                {
//       String type;
//       if (ArduinoOTA.getCommand() == U_FLASH) {
//         type = "sketch";
//       } else { // U_SPIFFS
//         type = "filesystem";
//       }
//       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
//       Serial.println("Start updating " + type); })
//       .onEnd([]()
//              { Serial.println("\nEnd"); })
//       .onProgress([](unsigned int progress, unsigned int total)
//                   { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
//       .onError([](ota_error_t error)
//                {
//       Serial.printf("Error[%u]: ", error);
//       if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//       else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//       else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//       else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//       else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

//   ArduinoOTA.begin();

//   Serial.println("Ready");
//   Serial.print("IP address: ");
//   Serial.println(WiFi.localIP());
// }
