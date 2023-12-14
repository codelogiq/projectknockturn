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
bool isPowered = true;

CRGB leds[NUM_LEDS];

bool shouldClearEffects = false;
unsigned long clearEffectTime;

bool needsChange = true;
int ticksSinceSpellEffects = 0;
unsigned long msSinceLastSpellEffect = 0;

unsigned long lastLanternReset = 0;
unsigned long lastGrateReset = 0;


// explosion effect variables
int explosionStep = 0;
bool explosionEffectInProgress = false;
unsigned long explosionLastStepTime = 0;
unsigned long delayBetweenSteps = 50;  // Initial delay between steps

// wand battle effect variables


enum WandBattleEffectStage {
  WAND_START,
  WAND_INITIAL_FLICKER,
  WAND_SPELL_CAST,
  WAND_MEETING_SPELL,
  WAND_MEETING_SPELL_FADE,
  WAND_EXPLOSION,
  WAND_CLEANUP
};

bool wandBattleInProgress = false;
int totalEffects = sizeof(effects) / sizeof(effects[0]);
int forceOfGood = 0;
int forceOfEvil = totalEffects - 1;
int travelDistance = totalEffects - 2;
int meetingPoint = totalEffects / 2;
CRGB goodColor = CRGB::DarkRed;
CRGB badColor = CRGB::DarkGreen;
CRGB goodSpellColor = CRGB::Red;
CRGB badSpellColor = CRGB::Green;
CRGB meetingSpellColor = CRGB::Purple;
WandBattleEffectStage currentStage = WAND_START;
unsigned long lastUpdate = 0;


// house color effect variables
#define HOUSE_RAVENCLAW CHSV(160, 161, 52)
#define HOUSE_SLYTHERIN CHSV(99, 180, 40)
#define HOUSE_HUFFLEPUFF CHSV(54, 120, 40)
#define HOUSE_GRYFFINDOR CHSV(0, 205, 45)

enum EffectState {
  INIT,
  FLASH_UP,
  PAUSE,
  FADE_DOWN,
  DONE
};
bool houseColorEffectInProgress = false;
EffectState state = INIT;
unsigned long effectStartTime = 0;
unsigned long transitionStartTime = 0;
unsigned long fadeInOutDuration = 0;
unsigned long pauseDuration = 2000; // Milliseconds
CHSV color;

// loop
unsigned long lastMainLoopReset = 0;


// house color chase variables

CRGB houseChaseColors[] = { CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkRed, CRGB::Gold };
bool houseColorChaseEffectRunning = false;
unsigned int houseColorChaseWaveDelay = 60;
int maxChaseCycles = 3;
int houseChaseIndex = -1;
int chaseDirection = 1;
unsigned long houseChaseLastUpdate = 0;
int cycleCount = 0;
int chaseColorIndex = 0;


// lightning variables

enum LightningEffectStage {
  LIGHTNING_START,
  LIGHTNING_STRIKE,
  LIGHTNING_STRIKE_DELAY,
  LIGHTNING_CLEANUP,
  LIGHTNING_NEXT_STRIKE
};

int minStrikes = 5;
int maxStrikes = 10;
int totalHitspots = sizeof(effects) / sizeof(effects[0]);  // total potential hitspots
int currentStrike = 0;
int sizeOfStrike = 0;
int strikeCounter = 0;
int dimmer = 1;
LightningEffectStage currentLightningStage = LIGHTNING_START;
unsigned long lastLightningUpdate = 0;
bool strikeInProgress = false;


// flash effect variables

unsigned long flashStartTime = 0;
unsigned long flashDuration = 0;
int flashStep = 0;
bool isFlashing = false;


// done with vars


void triggerSpellEffects() {
  switch (random(0, 5)) {
    case 0:
      flashEffect(); // moved to nonblocking successfully
      break;
    case 1:
      explosionEffect(); // moved to nonblocking successfully
      break;
    case 2:
      houseColorChase(); // moved to nonblocking successfully
      break;
    case 3:
      houseColorEffect(); // 
      break;
    case 4:
      lightningEffect();
      break;
    case 5: 
      wandBattleEffect(); // moved to nonblocking successfully
      break;
  }
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

void powerdownLights() {
  for (int i=0;i<NUM_LEDS;i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  Serial.println("goodbye.");
}

void explosionEffect() {
  if (!explosionEffectInProgress) {
     explosionStep = 0;
     explosionEffectInProgress = true;
  }
  unsigned long currentMillis = millis();
  if (explosionStep < 255) {
    if (currentMillis - explosionLastStepTime >= delayBetweenSteps) {
      for (int magic : effects) {
        CHSV color = CHSV(random8(), random(100, 200), explosionStep);
        leds[magic].setHSV(color.hue, color.sat, color.val);
      }
      if (random(0, 400) == 5) {
        for (int magic : effects) {
          leds[magic].setHSV(0, 0, 0);
        }
      }
      FastLED.show();
      explosionStep += 4;  // Increase the step
      explosionLastStepTime = currentMillis;
      delayBetweenSteps = random(50, 350);  // Randomize delay for next step
    }
  } else {
    for (int magic : effects) {
      leds[magic] = CRGB::Black;
    }
    FastLED.show();
    explosionStep = 0;  // Reset step counter for next run
    explosionEffectInProgress = false;
  }
}

void wandBattleEffect() {
  unsigned long currentMillis = millis();

  switch (currentStage) {
    case WAND_START:
      leds[effects[forceOfGood]] = goodColor;
      leds[effects[forceOfEvil]] = badColor;
      FastLED.show();
      lastUpdate = currentMillis;
      currentStage = WAND_INITIAL_FLICKER;
      wandBattleInProgress = true;
      break;

    case WAND_INITIAL_FLICKER:
      if (currentMillis - lastUpdate >= random(2000, 4000)) {
        for (int i = 0; i < random(8, 12); i++) {
          leds[effects[forceOfGood]] = i % 2 ? CRGB::Black : goodColor;
          leds[effects[forceOfEvil]] = i % 2 ? CRGB::Black : badColor;
          FastLED.show();
          delay(random(25, 60));
        }
        leds[effects[forceOfGood]] = goodColor;
        leds[effects[forceOfEvil]] = badColor;
        FastLED.show();
        lastUpdate = currentMillis;
        currentStage = WAND_SPELL_CAST;
      }
      break;

    case WAND_SPELL_CAST:
      for (int i = forceOfGood + 1; i <= meetingPoint; i++) {
        leds[effects[i]] = goodSpellColor;
        leds[effects[totalEffects - (i + 1)]] = badSpellColor;
        FastLED.show();
        delay(random(40, 100));
      }
      leds[effects[meetingPoint]] = meetingSpellColor;
      FastLED.show();
      lastUpdate = currentMillis;
      currentStage = WAND_MEETING_SPELL;
      break;

    case WAND_MEETING_SPELL:
      if (currentMillis - lastUpdate >= random(200, 450)) {
        for (int i = 0; i < random(10, 30); i++) {
          leds[effects[meetingPoint]] = i % 2 ? CRGB::Black : meetingSpellColor;
          FastLED.show();
          delay(random(25, 60));
        }
        for (int i = 1; i < totalEffects - 1; i++) {
          leds[effects[i]] = CRGB::Black;
        }
        leds[effects[meetingPoint]] = meetingSpellColor;
        FastLED.show();
        lastUpdate = currentMillis;
        currentStage = WAND_EXPLOSION;
      }
      break;

    case WAND_EXPLOSION:
      switch (currentMillis % 2) {
        case 0:
          for (int i = meetingPoint; i > 0; i--) {
            leds[effects[i]] = meetingSpellColor;
            FastLED.show();
            delay(random(80, 400));
          }
          break;
        case 1:
          for (int i = meetingPoint + 1; i < totalEffects - 1; i++) {
            leds[effects[i]] = meetingSpellColor;
            FastLED.show();
            delay(random(80, 400));
          }
          break;
      }
      lastUpdate = currentMillis;
      currentStage = WAND_CLEANUP;
      break;

    case WAND_CLEANUP:
      for (int magic : effects) {
        leds[magic] = CRGB::Black;
      }
      FastLED.show();
      currentStage = WAND_START;
      wandBattleInProgress = false;
      break;
  }
}

void houseColorEffect() {
  unsigned long currentMillis = millis();
  if (!houseColorEffectInProgress) { 
    state = INIT;
    houseColorEffectInProgress = true;
  }
  if (state == INIT) {
    int colorToChoose = random(0, 3);
    if (colorToChoose != 1) colorToChoose = random(0, 3); // Weight towards Ravenclaw
    switch (colorToChoose) {
      case 0:
        color = HOUSE_SLYTHERIN;
        break;
      case 1:
        color = HOUSE_RAVENCLAW;
        break;
      case 2:
        color = HOUSE_HUFFLEPUFF;
        break;
      case 3:
        color = HOUSE_GRYFFINDOR;
        break;
    }
    effectStartTime = currentMillis;
    transitionStartTime = currentMillis;
    fadeInOutDuration = 1000; // Change this value to adjust fade in/out duration
    state = FLASH_UP;
  }

  switch (state) {
    case FLASH_UP:
      if (currentMillis - transitionStartTime < fadeInOutDuration) {
        int targetValue = map(currentMillis, transitionStartTime, transitionStartTime + fadeInOutDuration, color.val, 255);
        for (int magic : effects) {
          leds[magic].setHSV(color.hue, color.sat, targetValue);
        }
        FastLED.show();
      } else {
        transitionStartTime = currentMillis;
        state = PAUSE;
      }
      break;

    case PAUSE:
      if (currentMillis - transitionStartTime >= pauseDuration) {
        transitionStartTime = currentMillis;
        state = FADE_DOWN;
      }
      break;

    case FADE_DOWN:
      if (currentMillis - transitionStartTime < fadeInOutDuration) {
        int targetValue = map(currentMillis, transitionStartTime, transitionStartTime + fadeInOutDuration, 255, color.val);
        for (int magic : effects) {
          leds[magic].setHSV(color.hue, color.sat, targetValue);
        }
        FastLED.show();
      } else {
        state = DONE;
      }
      break;

    case DONE:
      houseColorEffectInProgress = false;
      state = INIT;
      break;
  }
}

void houseColorChase() {
  unsigned long currentMillis = millis();
  if (!houseColorChaseEffectRunning) { 
    // we need to setup the color chase.
    houseColorChaseEffectRunning = true;
    houseChaseLastUpdate = currentMillis;
    houseChaseIndex = 0;
    cycleCount = 0;
    chaseDirection = 1;
    chaseColorIndex = random(0,3);
    if (chaseColorIndex != 1) chaseColorIndex = random(0,3);
  }
  int maxCount = sizeof(effects) / sizeof(effects[0])-1;

  if (currentMillis - houseChaseLastUpdate > houseColorChaseWaveDelay) { 
    if (chaseDirection == 1) { 
      // going up
      if (houseChaseIndex+1 == maxCount) { 
        chaseDirection = -1;
        houseChaseLastUpdate = currentMillis;
        return;
      }
      leds[effects[houseChaseIndex]] = houseChaseColors[chaseColorIndex];
      if (houseChaseIndex>0) {
        leds[effects[houseChaseIndex-1]] = CRGB::Black;
      }
      FastLED.show();
      houseChaseLastUpdate = currentMillis;
      houseChaseIndex++;
      return;
    } else { 
      houseChaseIndex--;
      if (houseChaseIndex == -1) {
        chaseDirection = 1;
        cycleCount++;
        if (cycleCount == maxChaseCycles) {
          houseColorChaseEffectRunning = false;
          leds[effects[0]] = CRGB::Black;
          FastLED.show();
        }
        houseChaseLastUpdate = currentMillis;
        leds[effects[0]] = houseChaseColors[chaseColorIndex];
        return;
      }
      if (houseChaseIndex <= maxCount)
        leds[effects[houseChaseIndex+1]] = CRGB::Black;
      leds[effects[houseChaseIndex]] = houseChaseColors[chaseColorIndex];
      FastLED.show();
      houseChaseLastUpdate = currentMillis;
      return;
    }
  }
}

void flashEffect() {
  unsigned long currentMillis = millis();
  if (!isFlashing) {
    flashStartTime = currentMillis;
    flashDuration = random(0, 250);
    flashStep = 0;
    isFlashing = true;
  }

  if (currentMillis - flashStartTime < flashDuration) {
    if (flashStep == 0) {
      for (int magic : effects) {
        leds[magic] = CRGB::Purple;
      }
      FastLED.show();
      flashStep = 1;
    } else if (flashStep == 1) {
      for (int magic : effects) {
        leds[magic].setHSV(0, 0, 0);
      }
      FastLED.show();
      flashStep = 2;
    }
  } else {
    // Reset LEDs to black when the flashing is done
    for (int magic : effects) {
      leds[magic] = CRGB::Black;
    }
    FastLED.show();
    isFlashing = false;
  }
}

void lightningEffect() {
  unsigned long currentMillis = millis();
  int localEffects[totalHitspots];

  switch (currentLightningStage) {
    case LIGHTNING_START:
      strikeInProgress = true;
      currentStrike = random(1, maxStrikes);
      currentLightningStage = LIGHTNING_NEXT_STRIKE;
      break;

    case LIGHTNING_NEXT_STRIKE:
      if (strikeCounter >= currentStrike) {
        currentLightningStage = LIGHTNING_CLEANUP;
        break;
      }
      sizeOfStrike = random(1, totalHitspots);
      strikeCounter++;
      dimmer = (strikeCounter == 1) ? 5 : random8(1, 3);
      for (int j = 0; j < totalHitspots - 1; j++) {
        localEffects[j] = effects[j];
      }
      for (size_t c = 0; c < totalHitspots - 1; c++) {
        size_t j = random(0, totalHitspots - c);
        int t = localEffects[c];
        localEffects[c] = localEffects[j];
        localEffects[j] = t;
      }
      currentLightningStage = LIGHTNING_STRIKE;
      lastLightningUpdate = currentMillis;
      break;

    case LIGHTNING_STRIKE:
      for (int j = 0; j < sizeOfStrike; j++) {
        leds[localEffects[j]] = CRGB::Black;
      }
      FastLED.show();
      for (int j = 0; j < sizeOfStrike; j++) {
        leds[localEffects[j]].setHSV(255, 0, 255 / dimmer);
      }
      FastLED.show();
      currentLightningStage = LIGHTNING_STRIKE_DELAY;
      break;

    case LIGHTNING_STRIKE_DELAY:
      if (currentMillis - lastLightningUpdate >= random(4, 10)) {
        for (int j = 0; j < sizeOfStrike; j++) {
          leds[localEffects[j]] = CRGB::Black;
        }
        FastLED.show();
        currentLightningStage = (strikeCounter == 1) ? LIGHTNING_CLEANUP : LIGHTNING_NEXT_STRIKE;
        lastLightningUpdate = currentMillis;
      }
      break;

    case LIGHTNING_CLEANUP:
      for (int magic : effects) {
        leds[magic] = CRGB::Black;
      }
      FastLED.show();
      strikeInProgress = false;      
      currentLightningStage = LIGHTNING_START;
      break;
  }
}

void resetEffectFlags() { 
  isFlashing = false;
  strikeInProgress = false;
  wandBattleInProgress = false;
  explosionEffectInProgress = false;
  houseColorEffectInProgress = false;
  houseColorChaseEffectRunning = false;
}

void setup() {
  unsigned long currentMillis = millis();
  Serial.begin(9600);
  randomSeed(analogRead(0));
  random8Seed(analogRead(0));
  while (!Serial);
  FastLED.addLeds<LED_STRIP_TYPE, LED_PIN, LED_COLOR_MODE>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip)
    .setDither(MAX_BRIGHTNESS < 255);

  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1050);
  FastLED.clear();  // initialize LEDs to off.
  msSinceLastSpellEffect = currentMillis;
  lastLanternReset = currentMillis;
  lastGrateReset = currentMillis;
  // power button setup
  pinMode(MAIN_POWER_BUTTON_PIN, INPUT);
  pinMode(SW_RESET_PIN, INPUT);
  digitalWrite(SW_RESET_PIN, LOW);
  resetEffectFlags();
  isPowered = digitalRead(MAIN_POWER_BUTTON_PIN) == HIGH;
  resetAllToDefault();
  lastMainLoopReset = currentMillis;
}

bool lostPower = false;
// working
void loop() {
  unsigned long currentMillis = millis();
  if (digitalRead(MAIN_POWER_BUTTON_PIN) == LOW) {
    if (!isPowered) { 
      delay(10);
      return;
    }
    powerdownLights();
    resetEffectFlags();
    return;
  } else {
    // power button is ON
    isPowered = true;
    if (strikeInProgress) {
      lightningEffect();
      return;
    }
    if (isFlashing) {
      flashEffect();
      return;
    }
    if (wandBattleInProgress) {
      wandBattleEffect();
      return;
    }
    if (explosionEffectInProgress) {
      explosionEffect();
      return;
    }
    if (houseColorChaseEffectRunning) {
      houseColorChase();
      return;
    }      
    if (houseColorEffectInProgress) {
      houseColorEffect();
      return;
    }
    if (!strikeInProgress && !isFlashing && !wandBattleInProgress && !explosionEffectInProgress && !houseColorChaseEffectRunning && !houseColorEffectInProgress) {
      if (currentMillis - lastMainLoopReset > 200) {
        resetAllToDefault();
        lastMainLoopReset = currentMillis;
      }
    }
    if (useSpellEffects) {
      if (currentMillis - msSinceLastSpellEffect > minTicksUntilSpellEffects) {
        if ((currentMillis - msSinceLastSpellEffect) > random(minTicksUntilSpellEffects, maxTicksUntilSpellEffects)) {
          Serial.println((String)"Spell effects have been triggered. millis was: " + msSinceLastSpellEffect);
          triggerSpellEffects();
          msSinceLastSpellEffect = currentMillis;
        }
      }
    }
  }
}

