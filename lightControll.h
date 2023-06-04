#include "FastLED.h"

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
bool whiteOn = false;

void setupLight() {
  pinMode(WHITE_LED_PIN, OUTPUT);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN_OUTHER, COLOR_ORDER>(leds_outher, NUM_LEDS_OUTHER).setCorrection(TypicalLEDStrip).setDither(BRIGHTNESS < 255);
  FastLED.addLeds<LED_TYPE, DATA_PIN_CORE, COLOR_ORDER>(leds_core, NUM_LEDS_CORE).setCorrection(TypicalLEDStrip).setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
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
    
    CRGB newcolor = CHSV(hue8, sat8, 50);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS_OUTHER-1) - pixelnumber;
    
    if (!whiteOn) {
      nblend(leds_outher[pixelnumber], newcolor, 64);
    } else {
      nblend(leds_outher[pixelnumber], CRGB(255, 255, 255), 64);
    }

  }

}

void controllWhiteLED() {
  if (lastOn < millis() - lastTime) {
    whiteOn = false;

  }

  if (lastRand < millis() - 150) {
    lastRand = millis();
    if (rand() % 100 < 20) {
      lastOn = millis();
      lastTime = rand() % 200;
      whiteOn = true;
    }

  }

}

void disco() {
  for (int i = 0 ; i < NUM_LEDS_CORE; i++) {
    nblend(leds_core[i], CRGB(0, 0, 0), 64);
    CRGB color;
    if (millis() % 200 < 100) {
      int colorInd = millis() / 100 % 100 * 2.5;
      color = CRGB((uint32_t)((colorInd * i) % 255), (uint32_t)((colorInd * i) % 255), (uint32_t)(colorInd));
      if (i % 2 == 0) {
        nblend(leds_core[i], color, 64);
      }

    } else {
      int colorInd = millis() / 100 + 100 % 100 * 2.5;
      color = CRGB((uint32_t)((colorInd * i) % 255), (uint32_t)((colorInd * i) % 255), (uint32_t)(colorInd));
      if (i % 2 != 0) {
        nblend(leds_core[i], color, 64);
      }
    }

    if(whiteOn) {
      nblend(leds_core[i], CRGB(255, 255, 255), 64);
    }

  }

}

//Controlls the LEDs
void Leds(void * pvParameters) {
  while(true) {
    controllWhiteLED();
    digitalWrite(WHITE_LED_PIN, whiteOn);
    pride();
    disco();
    FastLED.show();
  } 

}