//----Libarys-----
#include "FastLED.h"
#include <AccelStepper.h>
#include <ESP32Servo.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

//-----Own Files-----
#include "secrets.h"

//-----LEDs-----
#define DATA_PIN_OUTHER     18
#define DATA_PIN_CORE       19
#define LED_TYPE            WS2812
#define COLOR_ORDER         GRB
#define NUM_LEDS_OUTHER     27
#define NUM_LEDS_CORE       18
#define BRIGHTNESS          255

#define WHITE_LED_PIN       17

CRGB leds_outher[NUM_LEDS_OUTHER];
CRGB leds_core[NUM_LEDS_CORE];

int lastOn = millis();
int lastRand = millis();
int lastTime = 0;


//-----Tasks-----
TaskHandle_t Task1;
TaskHandle_t Task2;

//-----Stepper-----
#define ENABLE_STEPPER 16
AccelStepper stepper(1, 2, 4);

//-----Servo-----
#define SERVO_DATA_PIN 21
Servo servoMotor;

//-----OTA-Update-----
//Login credentials in the secrets.h file
const char* host = "esp32";

WebServer server(80);

void setup() {

  pinMode(ENABLE_STEPPER, OUTPUT);
  pinMode(WHITE_LED_PIN, OUTPUT);
  digitalWrite(ENABLE_STEPPER, LOW);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN_OUTHER, COLOR_ORDER>(leds_outher, NUM_LEDS_OUTHER).setCorrection(TypicalLEDStrip).setDither(BRIGHTNESS < 255);
  FastLED.addLeds<LED_TYPE, DATA_PIN_CORE, COLOR_ORDER>(leds_core, NUM_LEDS_CORE).setCorrection(TypicalLEDStrip).setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  servoMotor.attach(SERVO_DATA_PIN);

  //OTA
  Serial.begin(115200); 
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      vTaskDelete(Task1);
      vTaskDelete(Task1);
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/

      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
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

//Controlls the LEDs
void Leds( void * pvParameters ) {
  while(true) {
    controllWhiteLED();
    //pride();
    disco();
    FastLED.show();
  } 

}

//Controlls the motors
void Motors( void * pvParameters ) {
  int servoCurrentPosition = 0;
  int servoGoToPosition = 180;
  bool servoUp = true;
  int lastMillis = millis();

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  stepper.setCurrentPosition(0);

  while(true) {
    if (stepper.currentPosition() <= 0) {
      stepper.moveTo(2000);
      stepper.run();
    } else if(stepper.currentPosition() >= 2000) {
      stepper.moveTo(0);
      stepper.run();
    } else {
      stepper.run();
    }

    if (millis() >= lastMillis + 20) {
      lastMillis = millis();
      if (servoCurrentPosition == servoGoToPosition) { //UP
        servoGoToPosition = rand() % 180;
        servoCurrentPosition = servoCurrentPosition + 1;
        servoUp = true;
      } else if(servoUp && servoCurrentPosition >= servoGoToPosition) { //DOWN
        servoGoToPosition = rand() % 85;
        servoCurrentPosition = servoCurrentPosition - 1;
        servoUp = false;
      } else {
        if (servoGoToPosition >= 90) {
          servoCurrentPosition += 1;
        } else {
          servoCurrentPosition -= 1;
        }

      }
      servoMotor.write(servoCurrentPosition);
    }

  }

}


// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for(uint16_t i = 0 ; i < NUM_LEDS_OUTHER; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV(hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS_OUTHER-1) - pixelnumber;
    
    nblend(leds_outher[pixelnumber], newcolor, 64);
  }

}

void controllWhiteLED() {
  if (lastOn < millis() - lastTime) {
    digitalWrite(WHITE_LED_PIN, LOW);
  }

  if (lastRand < millis() - 150) {
    lastRand = millis();
    if (rand() % 100 < 20) {
      lastOn = millis();
      lastTime = rand() % 200;
      digitalWrite(WHITE_LED_PIN, HIGH);
    }

  }

}

void disco() {
  for (int i = 0 ; i < NUM_LEDS_CORE - 1; i+=2) {
    CRGB color;
    if (millis() % 200 < 100) {
      int colorInd = millis() / 100 % 100 * 2.5;
      color = CRGB((uint32_t)((colorInd + 85) % 255), (uint32_t)((colorInd + 170) % 255), (uint32_t)(colorInd));
    } else {
      color = CRGB(0, 0, 0);
    }
    if(millis() % 300 < 100 or (millis() % 300 > 200 and millis() % 400 < 250) or (millis() % 700 > 200 and millis() % 400 < 750)) {
      nblend(leds_core[i + 1], color, 64);
      nblend(leds_core[i], CRGB(0, 0, 0), 64);
    } else {
      nblend(leds_core[i + 1], CRGB(0, 0, 0), 64);
      nblend(leds_core[i], CRGB(255, 255, 255), 64);
    }

  }

}