#ifndef XBOX_H
#define XBOX_H

#include <Bluepad32.h>

extern ControllerPtr myControllers[BP32_MAX_GAMEPADS];
extern bool weaponHalfSpeedMode;
extern bool manualMode;
extern bool weaponFullSpeedMode;
extern bool battlebot;
extern int weaponSpeed;

void onConnectedController(ControllerPtr ctl);
void onDisconnectedController(ControllerPtr ctl);
void dumpGamepad(ControllerPtr ctl);
void processControllerInput(ControllerPtr ctl);
void processConnectedControllers();
void computeDriveMotorSpeeds(int axisX, int brake, int throttle, int& leftSpeed, bool& leftForward, int& rightSpeed, bool& rightForward);
void drive(ControllerPtr ctl);
void printDriveDebug();
void stopDriveMotors();


#endif
