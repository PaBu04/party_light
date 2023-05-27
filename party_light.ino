//-----Tasks-----
TaskHandle_t Task1;
TaskHandle_t Task2;

//-----Including Files-----
#include "secrets.h"
#include "otaUpdater.h"
#include "motorControll.h"
#include "lightControll.h"

void setup() {
  setupLight();
  setupMotors();
  setupOtaUpdater();
  //create a task that will be executed in the Leds() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Leds,   /* Task function. */
                    "Leds",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //create a task that will be executed in the Motors() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Motors,   /* Task function. */
                    "Motors",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

void loop() {
  server.handleClient();
  delay(1);
}