#include <FastLED.h>

#define NUM_LEDS 21             // how many LEDs we gots.
#define LED_PIN 4               // pin that LEDs are connected to.
#define LED_STRIP_TYPE WS2812B  // dependent on LED strip
#define LED_COLOR_MODE GRB      // dependent on LED strip

#define STATIC_COLOR CHSV(30, 208, 127)       // warm yellowish color for lanterns
#define COLOR_GRATE_PURPLE CHSV(205, 70, 86)  // purple color for grate
#define MAX_BRIGHTNESS 85


#define MAIN_POWER_BUTTON_PIN 7
#define SW_RESET_PIN 8

// LED configuration
uint32_t lanterns[] = { 1, 3, 5, 8, 10, 12, 15, 16, 19 };        // which LEDs in the strip are for lanterns
uint32_t effects[] = { 0, 2, 4, 6, 9, 11, 13, 14, 17, 18, 20 };  // which LEDs in the strip are for effects
uint32_t grates[] = { 7 };                                       // which LEDs in the strip are for secondary color grates (maybe expand this to define the color along with the led)

// Configurables
unsigned long maxTicksUntilSpellEffects = 60000;  // at most we want to wait 1 minute for spell effects
unsigned long minTicksUntilSpellEffects = 30000;  // at a minimum, we want spell effects every 30 seconds
bool useSpellEffects = true;
bool useLanterns = true;
bool useGrates = true;

// do not touch below here

CRGB leds[NUM_LEDS];

bool shouldClearEffects = false;
unsigned long clearEffectTime;

bool needsChange = true;
int ticksSinceSpellEffects = 0;
unsigned long msSinceLastSpellEffect = 0;

unsigned long lastLanternReset = 0;
unsigned long lastGrateReset = 0;
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  while (!Serial)
    ;
  FastLED.addLeds<LED_STRIP_TYPE, LED_PIN, LED_COLOR_MODE>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip)
    .setDither(MAX_BRIGHTNESS < 255);

  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1050);
  FastLED.clear();  // initialize LEDs to off.

  resetAllToDefault();
  msSinceLastSpellEffect = millis();
  lastLanternReset = millis();
  lastGrateReset = millis();
  // power button setup
  pinMode(MAIN_POWER_BUTTON_PIN, INPUT);
  pinMode(SW_RESET_PIN, INPUT);
  digitalWrite(SW_RESET_PIN, LOW);

}


void resetEffects(bool show = true) {
  for (int led : effects) {
    leds[led] = CRGB::Black;
  }
  if (show) {
    FastLED.show();
  }
}


void resetLanterns(bool show = true) {
  for (int torch : lanterns) {
    // leds[torch] = CRGB(255,147,41);
    // fadeToBlackBy(&leds[torch], 1, 235);
    leds[torch].setHSV(STATIC_COLOR.hue, STATIC_COLOR.sat, STATIC_COLOR.val);
  }
  if (show) {
    lastLanternReset = millis();
    FastLED.show();
  }
}

void resetGrates(bool show = true) {
  for (int grate : grates) {
    leds[grate] = CRGB::Purple;
    leds[grate].maximizeBrightness(30);
  }
  if (show) {
    lastGrateReset = millis();
    FastLED.show();
  }
}


void resetAllToDefault() {
  FastLED.clear(false);
  resetGrates(false);
  resetEffects(false);
  resetLanterns(false);
  FastLED.show();
}

bool demoMode = false;

void powerdownLights() {
  for (int i=0;i<NUM_LEDS;i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  Serial.println("goodbye.");
}

bool isPowered = true;

// working
void loop() {
  if (digitalRead(MAIN_POWER_BUTTON_PIN) == LOW) {
    if (!isPowered) { 
      delay(10);
      return;
    }
    Serial.println("Need to turn off things.");
    powerdownLights();
    isPowered = false;
    return;
  }
  isPowered = true;
  resetAllToDefault();
  delay(10);
  if (demoMode) {
    wandBattleEffect();
    delay(5000);
    explosionEffect2();
    resetAllToDefault();
    delay(5000);
    flashEffect();
    resetAllToDefault();
    delay(5000);
    houseColorChase();
    resetAllToDefault();
    delay(5000);
    houseColorEffect();
    resetAllToDefault();
    delay(5000);
    lightningEffect();
    resetAllToDefault();
    delay(5000);
    return;
  }
  if (useSpellEffects) {
    if (millis() - msSinceLastSpellEffect > minTicksUntilSpellEffects) {
      if ((millis() - msSinceLastSpellEffect) > random(minTicksUntilSpellEffects, maxTicksUntilSpellEffects)) {
        triggerSpellEffects();
        msSinceLastSpellEffect = millis();
        Serial.println((String)"Spell effects have been triggered. millis was: " + msSinceLastSpellEffect);
      }
    }
  }

}


// int iterationsSinceReset = 0;
// void loop() {
//   // LED Functionality
//   if (useSpellEffects) {
//     if (millis() - msSinceLastSpellEffect > minTicksUntilSpellEffects) {
//       if ((millis() - msSinceLastSpellEffect) > random(minTicksUntilSpellEffects, maxTicksUntilSpellEffects)) {
//         triggerSpellEffects();
//         msSinceLastSpellEffect = millis();
//         Serial.println((String)"Spell effects have been triggered. millis was: " + msSinceLastSpellEffect);
//       }
//     }
//   }
//   if (iterationsSinceReset>50) {
//     resetAllToDefault();
//   }
//   // END LEDS
//   delay(5);
//   resetAllToDefault();
// }

void explosionEffect() {
  for (int x = 0; x < random(1, 2); x++) {
    uint8_t hue = random(0, 255);
    CHSV color;
    for (int i = 0; i < 255; i += 6) {
      color = CHSV(hue, 200, i);
      for (int magic : effects) {
        leds[magic].setHSV(color.hue, color.sat, color.val);
      }
      if (random(0, 400) == 5) {
        for (int magic : effects) {
          leds[magic].setHSV(0, 0, 0);
        }
        FastLED.show();
        delay(random(0, 850));  // was 450
      }
      FastLED.show();
      delay(2);
    }
  }

  for (int magic : effects) {
    leds[magic] = CRGB::Black;
  }
  FastLED.show();
}


void explosionEffect2() {
  CRGB possibleColors[] = { CRGB::Purple, CRGB::Blue, CRGB::Green, CRGB::DodgerBlue, CRGB::MidnightBlue, CRGB::LightSalmon, CRGB::Pink };
  int totalPossibilities = sizeof(possibleColors) / sizeof(possibleColors[0]);
  for (int i = 0; i < 255; i += 4) {
    // color = CHSV(hue, 255, i);
    for (int magic : effects) {
      CHSV color = CHSV(random(0,255), random(100,200),i);
      // leds[magic] = possibleColors[random(0,totalPossibilities)-1];
      // fadeToBlackBy(&leds[magic], 1, 128/i);
      leds[magic].setHSV(color.hue, color.sat, color.val);
    }
    if (random(0, 400) == 5) {
      for (int magic : effects) {
        leds[magic].setHSV(0, 0, 0);
      }
      // FastLED.show();
      // delay(random(0, 850));  // was 450
    }
    FastLED.show();
    delay(random(50,350));
  }

  for (int magic : effects) {
    leds[magic] = CRGB::Black;
  }
  FastLED.show();
}

void wandBattleEffect() {
  int totalEffects = sizeof(effects)/sizeof(effects[0]);
  int forceOfGood = 0; // our lefthand wand is the first led
  int forceOfEvil = totalEffects-1; // our righthand wand is the final led in the chain.
  int travelDistance = totalEffects-2;
  int meetingPoint = totalEffects/2;
  CRGB goodColor = CRGB::DarkRed;
  CRGB badColor = CRGB::DarkGreen;
  CRGB goodSpellColor = CRGB::Red;
  CRGB badSpellColor = CRGB::Green;
  CRGB meetingSpellColor = CRGB::Purple;
  // let's start by lighting up our wands..
  leds[effects[forceOfGood]] = goodColor;
  leds[effects[forceOfEvil]] = badColor;
  FastLED.show();
  delay(random(2000,4000)); // 3ish second delay
  for (int i=0;i<random(8,12);i++) {
    leds[effects[forceOfGood]] = i%2 ? CRGB::Black : goodColor;
    leds[effects[forceOfEvil]] = i%2 ? CRGB::Black : badColor;
    FastLED.show();
    delay(random(25,60));
  }
  leds[effects[forceOfGood]] = goodColor;
  leds[effects[forceOfEvil]] = badColor;
  FastLED.show();
  for (int i=forceOfGood+1; i<=meetingPoint; i++) {
    leds[effects[i]] = goodSpellColor;
    leds[effects[totalEffects-(i+1)]] = badSpellColor;
    FastLED.show();
    delay(random(40,100));
  }
  leds[effects[meetingPoint]] = meetingSpellColor;
  FastLED.show();
  delay(random(200,450));
  for (int i=0;i<random(10,30);i++) {
    leds[effects[meetingPoint]] = i%2 ? CRGB::Black : meetingSpellColor;
    FastLED.show();
    delay(random(25,60));
  }
  //cleanup
  for (int i=1;i<totalEffects-1;i++) {
    leds[effects[i]] = CRGB::Black;
  }
  leds[effects[meetingPoint]] = meetingSpellColor;

  FastLED.show();
  // now let's pick a direction and blow it up
  switch (millis()%2) { 
    case 0:
      for (int i=meetingPoint;i>0;i--) {
        leds[effects[i]] = meetingSpellColor;
        FastLED.show();
        delay(random(80,400));
      }
      break;
    case 1:    
      for (int i=meetingPoint+1;i<totalEffects-1;i++) {
        leds[effects[i]] = meetingSpellColor;
        FastLED.show();
        delay(random(80,400));
      }
      break;
  }
  for (int magic : effects) { 
    leds[magic] = CRGB::Black;
  }
  FastLED.show();
}

#define HOUSE_RAVENCLAW CHSV(160, 161, 52)
#define HOUSE_SLYTHERIN CHSV(99, 180, 40)
#define HOUSE_HUFFLEPUFF CHSV(54, 120, 40)
#define HOUSE_GRYFFINDOR CHSV(0, 205, 45)

void houseColorEffect() {
  // this is a long running effect so we're not going to block it entirely.
  int colorToChoose = random(0, 3);
  if (colorToChoose != 1) colorToChoose = random(0, 3);  // weight it towards ravenclaw tho...
  CHSV color;
  int minValue = 0;
  int maxValue = 255;
  int fadeDelay = 15;
  int pauseDelay = 2000;
  switch (colorToChoose) {
    case 0:
      minValue = 40;
      maxValue = 180;
      color = HOUSE_SLYTHERIN;  // Slytherin
      break;
    case 1:
      minValue = 30;
      maxValue = 200;
      color = HOUSE_RAVENCLAW;  // Ravenclaw
      break;
    case 2:
      minValue = 38;
      maxValue = 42;
      color = HOUSE_HUFFLEPUFF;  // Hufflepuff
      break;
    case 3:
      minValue = 25;
      maxValue = 150;
      color = HOUSE_GRYFFINDOR;  // Gryffindor
      break;
  }

  for (int i = color.val; i < maxValue; i++) {  // flash up
    for (int magic : effects) {
      leds[magic].setHSV(color.hue, color.sat, i);
    }
    FastLED.show();
    delay(fadeDelay);
  }
  delay(pauseDelay); // may look into making this nonblocking
  for (int i = maxValue; i > minValue; i--) {
    for (int magic : effects) {
      leds[magic].setHSV(color.hue, color.sat, i);
    }
    FastLED.show();
    delay(fadeDelay);
  }
  for (int i = minValue; i <= color.val; i++) {
    for (int magic : effects) {
      leds[magic].setHSV(color.hue, color.sat, i);
    }
    FastLED.show();
    delay(fadeDelay);
  }

  // for (int magic : effects) {
  //   leds[magic] = CRGB::Black;
  // }
  // FastLED.show();
  // clearEffectTime = millis();
  // shouldClearEffects = true;
}

// should write a specific magic scene with different colors in different windows.
// we'll need to hardcode the leds for this one though since we can't just rely on 'effects'
// maybe define each wall or something as a shop or area

void houseColorChase() {
  //CHSV chosenColor;
  int waveDelay = 75;  // swap delay
  CRGB possibleColors[] = { CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkRed, CRGB::Gold };
  CRGB chosenColor = possibleColors[random(0, 3)];
  if (chosenColor != 1) chosenColor = possibleColors[random(0, 3)];
  for (int i = 0; i < 2; i++) {
    for (int magic : effects) {
      leds[magic] = chosenColor;
      FastLED.show();
      delay(waveDelay);
      leds[magic] = CRGB::Black;
      FastLED.show();
    }
    for (int c = (sizeof(effects) / sizeof(effects[0])) - 1; c >= 0; c--) {
      leds[effects[c]] = chosenColor;
      FastLED.show();
      delay(waveDelay);
      leds[effects[c]] = CRGB::Black;
      FastLED.show();
    }
  }
  // cleanup
  for (int magic : effects) {
    leds[magic] = CRGB::Black;
  }
  FastLED.show();
}

void flashEffect() {
  for (int i = 0; i < random(2, 6); i++) {
    for (int magic : effects) {
      leds[magic] = CRGB::Purple;
    }
    FastLED.show();
    delay(random(0, 250));
    for (int magic : effects) {
      leds[magic].setHSV(0, 0, 0);
    }
    FastLED.show();
    delay(random(0, 450));
  }
  for (int magic : effects) {
    leds[magic] = CRGB::Black;
  }
  FastLED.show();
}

void lightningEffect() {
  int minStrikes = 5;
  int maxStrikes = 10;
  int totalHitspots = sizeof(effects) / sizeof(effects[0]);  // total potential hitspots
  for (int i = random(1, minStrikes); i <= random(1, maxStrikes); i++) {
    Serial.println((String) "Hitting lightning strike " + i);
    int sizeOfStrike = random(1, totalHitspots);  // how many leds are going to get struck
    const size_t n = totalHitspots;
    int localEffects[totalHitspots];
    for (int j = 0; j < totalHitspots - 1; j++) {
      localEffects[j] = effects[j];
    }
    int dimmer = 1;
    if (i == 1) dimmer = 5;
    else dimmer = random8(1, 3);

    for (size_t c = 0; c < n - 1; c++) {
      size_t j = random(0, n - c);
      int t = localEffects[c];
      localEffects[j] = t;
    }
    for (int j = 0; j < sizeOfStrike; j++) {
      leds[localEffects[j]] = CRGB::Black;
    }
    FastLED.show();
    for (int j = 0; j < sizeOfStrike; j++) {
      leds[localEffects[j]].setHSV(255, 0, 255 / dimmer);
    }
    FastLED.show();
    delay(random(4, 10));
    for (int j = 0; j < sizeOfStrike; j++) {
      leds[localEffects[j]] = CRGB::Black;
    }
    FastLED.show();
    if (i == 1) delay(150);
    delay(50 + random8(100));
  }
}


void triggerSpellEffects() {
  switch (random(0, 5)) {
    case 0:
      flashEffect();
      break;
    case 1:
      explosionEffect2();
      break;
    case 2:
      houseColorChase();
      break;
    case 3:
      houseColorEffect();
      break;
    case 4:
      lightningEffect();
      break;
    case 5: 
      wandBattleEffect();
      break;
  }
}
