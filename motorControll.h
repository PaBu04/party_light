#include <AccelStepper.h>
#include <ESP32Servo.h>

//-----Stepper-----
#define ENABLE_STEPPER 16
AccelStepper stepper(1, 2, 4);

//-----Servo-----
#define SERVO_DATA_PIN 21
Servo servoMotor;

void setupMotors() {

  pinMode(ENABLE_STEPPER, OUTPUT);
  digitalWrite(ENABLE_STEPPER, LOW);
  servoMotor.attach(SERVO_DATA_PIN);
}

//Controlls the motors
void Motors( void * pvParameters ) {
  int servoCurrentPosition = 0;
  int servoGoToPosition = 180;
  int stepperGoToPosition = 3000;
  bool servoUp = true;
  int lastMillis = millis();

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setCurrentPosition(0);
  stepper.moveTo(stepperGoToPosition);

  while(true) {
    if (stepper.currentPosition() == stepperGoToPosition) {
      stepperGoToPosition = rand() % 3000;
      stepper.moveTo(stepperGoToPosition);
    }
    stepper.run();

    if (millis() >= lastMillis + 20) {
      lastMillis = millis();
      if (servoCurrentPosition == servoGoToPosition) {
        servoGoToPosition = rand() % 180;
      } else if (servoGoToPosition > servoCurrentPosition) {
        servoCurrentPosition += 1;
      } else {
        servoCurrentPosition -= 1;
      }
      servoMotor.write(servoCurrentPosition);
    }

  }

}