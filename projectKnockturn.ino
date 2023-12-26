#include <FastLED.h>
#include <SoftwareSerial.h>
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"


// MP3 SETUP
SoftwareSerial mp3Serial(8/* Rx Arduino <- Tx DFPlayer */,7 /* Tx Arduino (Voltage Transformer) -> Rx DFPlayer */);
#define MAIN_AUDIO_PIN 11 /* pin for audio on/off */
bool isAudioEnabled = false;
DFRobotDFPlayerMini mp3player;

// LED Setup
#define NUM_LEDS 20             // how many LEDs we gots. (This needs to be updated to 20)
#define LED_PIN 4               // pin that LEDs are connected to.
#define LED_STRIP_TYPE WS2812B  // dependent on LED strip
#define LED_COLOR_MODE GRB      // dependent on LED strip
#define STATIC_COLOR CHSV(30, 208, 127)       // warm yellowish color for lanterns
#define COLOR_GRATE_PURPLE CHSV(205, 70, 86)  // purple color for grate
#define MAX_BRIGHTNESS 100
#define MAIN_POWER_BUTTON_PIN 10 /* pin for main power */
#define SW_RESET_PIN 8
// house colors
#define HOUSE_RAVENCLAW CHSV(160, 161, 52)
#define HOUSE_SLYTHERIN CHSV(99, 180, 40)
#define HOUSE_HUFFLEPUFF CHSV(54, 120, 40)
#define HOUSE_GRYFFINDOR CHSV(0, 205, 45)


// LED configuration
static const uint8_t lanterns[] = { 1, 3, 5, 8, 10, 12, 14, 16, 18 };        // which LEDs in the strip are for lanterns
static const uint8_t effects[] = { 0, 2, 4, 7, 9, 11, 13, 15, 17, 19 };  // which LEDs in the strip are for effects
static const uint8_t grates[] = { 6 };                                       // which LEDs in the strip are for secondary color grates (maybe expand this to define the color along with the led)
const static uint8_t totalEffects = sizeof(effects) / sizeof(effects[0]); // state tracking

// Configurables
static const unsigned long maxTicksUntilSpellEffects = 60000;  // at most we want to wait 1 minute for spell effects
static const unsigned long minTicksUntilSpellEffects = 30000;  // at a minimum, we want spell effects every 30 seconds
static const bool useSpellEffects = true;
// wand battle configuration variables
const static uint8_t forceOfGood = 0; // our led position of the "good" wand
const static uint8_t forceOfEvil = totalEffects - 1; // our led position of the "evil" wand
const static uint8_t meetingPoint = totalEffects / 2;
const static CRGB goodColor = CRGB::DarkRed;
const static CRGB badColor = CRGB::DarkGreen;
const static CRGB goodSpellColor = CRGB::Red;
const static CRGB badSpellColor = CRGB::Green;
const static CRGB meetingSpellColor = CRGB::Purple;
// house color effect configuration variables
const static unsigned long fadeInOutDuration = 1000;
const static unsigned long pauseDuration = 2000; // Milliseconds
// lightning effect variables
const static uint8_t minimumNumberOfLightningStrikes = 5;
const static uint8_t maximumNumberOfLightningStrikes = 10;
// house color chase variables
const static CRGB houseChaseColors[] = { CRGB::DarkGreen, CRGB::DarkBlue, CRGB::DarkRed, CRGB::Gold };
const static unsigned int houseColorChaseWaveDelay = 60;
const static int maxChaseCycles = 3;


// do not touch below here
bool isPowered = true;
CRGB leds[NUM_LEDS];
bool shouldClearEffects = false;
unsigned long msSinceLastSpellEffect = 0; // state tracking
unsigned long lastLanternReset = 0; // state tracking
unsigned long lastGrateReset = 0; // state tracking
/* BEGIN
 * The following are state tracking variables that shouldnt be touched. */
// explosion effect state tracking variables
uint32_t explosionStep = 0; 
bool explosionEffectInProgress = false;
unsigned long explosionLastStepTime = 0;
unsigned long delayBetweenSteps = 50;  // Initial delay between steps (will be randomized for future iterations)

// wand battle effect state tracking variables
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
WandBattleEffectStage currentStage = WAND_START;
unsigned long lastUpdate = 0;

// house color effect state tracking variables
enum EffectState {
  EF_INIT,
  EF_FLASH_UP,
  EF_PAUSE,
  EF_FADE_DOWN,
  EF_DONE
};
bool houseColorEffectInProgress = false;
EffectState state = EF_INIT;
unsigned long effectStartTime = 0;
unsigned long transitionStartTime = 0;
CHSV color;

// loop tracking for resetting LEDs
unsigned long lastMainLoopReset = 0;

// house color state tracking variables
bool houseColorChaseEffectRunning = false;
int houseChaseIndex = -1;
int chaseDirection = 1;
unsigned long houseChaseLastUpdate = 0;
int cycleCount = 0;
int chaseColorIndex = 0;

// flash effect state tracking variables
unsigned long flashStartTime = 0;
unsigned long flashDuration = 0;
int flashStep = 0;
bool isFlashing = false;

/* END State tracking variables */

void triggerSpellEffects() {
  int chosenEffect = random8(0,4);
  Serial.println((String)"Starting effect " + chosenEffect);
  switch (chosenEffect) {
    case 0:
      explosionEffect();
      break;
    case 1:
      houseColorChase();
      break;
    case 2:
      wandBattleEffect();
      break;
    case 3:
      lightningEffect();
      break;
    case 4: 
      houseColorEffect();
      break;
  }
}

void resetEffects(bool show = true) {
  for (uint8_t led : effects) {
    leds[led] = CRGB::Black;
  }
  if (show) {
    FastLED.show();
  }
}

void resetLanterns(bool show = true) {
  for (uint8_t torch : lanterns) {
    leds[torch].setHSV(STATIC_COLOR.hue, STATIC_COLOR.sat, STATIC_COLOR.val);
  }
  if (show) {
    lastLanternReset = millis();
    FastLED.show();
  }
}

void resetGrates(bool show = true) {
  for (uint8_t grate : grates) {
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
  for (uint8_t i=0; i<NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void explosionEffect() {
  if (!explosionEffectInProgress) {
     explosionStep = 0;
     explosionEffectInProgress = true;
     delayBetweenSteps = 50;
  }
  unsigned long currentMillis = millis();
  if (explosionStep < 255) {
    if (currentMillis-explosionLastStepTime >= delayBetweenSteps) {
      for (uint8_t magic : effects) {
        CHSV color = CHSV(random8(0, 255), random16(100, 200), explosionStep);
        leds[magic].setHSV(color.hue, color.sat, color.val);
      }
      if (random16(0, 400) == 5) {
        for (uint8_t magic : effects) {
          leds[magic].setHSV(0, 0, 0);
        }
      }
      FastLED.show();
      explosionStep += 4;  // Increase the step
      explosionLastStepTime = currentMillis;
      delayBetweenSteps = random16(50, 350);  // Randomize delay for next step
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

// this stil has delay() but is about as close as i can get to the animation i want in a non blocking capacity.
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
      if (currentMillis-lastUpdate >= random16(2000, 4000)) {
        for (uint8_t i=0; i<random8(8, 12); i++) {
          leds[effects[forceOfGood]] = i%2 ? CRGB::Black : goodColor;
          leds[effects[forceOfEvil]] = i%2 ? CRGB::Black : badColor;
          FastLED.show();
          delay(random8(25, 60));
        }
        leds[effects[forceOfGood]] = goodColor;
        leds[effects[forceOfEvil]] = badColor;
        FastLED.show();
        lastUpdate = currentMillis;
        currentStage = WAND_SPELL_CAST;
      }
      break;
    case WAND_SPELL_CAST:
      for (uint8_t i=forceOfGood+1; i<=meetingPoint; i++) {
        leds[effects[i]] = goodSpellColor;
        leds[effects[totalEffects-(i+1)]] = badSpellColor;
        FastLED.show();
        delay(random16(40, 100));
      }
      leds[effects[meetingPoint]] = meetingSpellColor;
      FastLED.show();
      lastUpdate = currentMillis;
      currentStage = WAND_MEETING_SPELL;
      break;

    case WAND_MEETING_SPELL:
      if (currentMillis-lastUpdate >= random16(200, 450)) {
        for (uint8_t i=0; i<random8(10, 30); i++) {
          leds[effects[meetingPoint]] = i % 2 ? CRGB::Black : meetingSpellColor;
          FastLED.show();
          delay(random8(25, 60));
        }
        for (uint8_t i=1; i<totalEffects-1; i++) {
          leds[effects[i]] = CRGB::Black;
        }
        leds[effects[meetingPoint]] = meetingSpellColor;
        FastLED.show();
        lastUpdate = currentMillis;
        currentStage = WAND_EXPLOSION;
      }
      break;

    case WAND_EXPLOSION:
      if (currentMillis%2) { 
          for (uint8_t i=meetingPoint; i>0; i--) {
            leds[effects[i]] = meetingSpellColor;
            FastLED.show();
            delay(random16(80, 400));
          }
      } else {
          for (uint8_t i=meetingPoint+1; i<totalEffects-1; i++) {
            leds[effects[i]] = meetingSpellColor;
            FastLED.show();
            delay(random16(80, 400));
          }
      }
      lastUpdate=currentMillis;
      currentStage=WAND_CLEANUP;
      break;

    case WAND_CLEANUP:
      for (uint8_t magic : effects) {
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
    state = EF_INIT;
    houseColorEffectInProgress = true;
  }
  if (state == EF_INIT) {
    uint8_t colorToChoose = random8(0, 3);
    if (colorToChoose != 1) colorToChoose = random8(0, 3); // Weight towards Ravenclaw

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
    state = EF_FLASH_UP;
  }

  switch (state) {
    case EF_FLASH_UP:
      if (currentMillis-transitionStartTime < fadeInOutDuration) {
        uint32_t targetValue = map(currentMillis, transitionStartTime, transitionStartTime + fadeInOutDuration, color.val, 255);
        for (uint8_t magic : effects) {
          leds[magic].setHSV(color.hue, color.sat, targetValue);
        }
        FastLED.show();
      } else {
        transitionStartTime = currentMillis;
        state = EF_PAUSE;
      }
      break;
    case EF_PAUSE:
      if (currentMillis-transitionStartTime >= pauseDuration) {
        transitionStartTime = currentMillis;
        state = EF_FADE_DOWN;
      }
      break;
    case EF_FADE_DOWN:
      if (currentMillis-transitionStartTime < fadeInOutDuration) {
        uint32_t targetValue = map(currentMillis, transitionStartTime, transitionStartTime + fadeInOutDuration, 255, color.val);
        for (uint8_t magic : effects) {
          leds[magic].setHSV(color.hue, color.sat, targetValue);
        }
        FastLED.show();
        delay(10); // needed to stop bad behavior of LED color shifts.
      } else {
        state = EF_DONE;
      }
      break;
    case EF_DONE:
      houseColorEffectInProgress = false;
      state = EF_INIT;
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
    chaseColorIndex = random8(0,3);
    if (chaseColorIndex != 1) chaseColorIndex = random8(0,3);
  }
  uint8_t maxCount = totalEffects-1;// sizeof(effects) / sizeof(effects[0])-1;

  if (currentMillis - houseChaseLastUpdate > houseColorChaseWaveDelay) { 
    if (chaseDirection == 1) { 
      // going up
      if (houseChaseIndex == maxCount) { 
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

// not a big fan of this effect..maybe we'll leave this out.
void flashEffect() {
  unsigned long currentMillis = millis();
  if (!isFlashing) {
    flashStartTime = currentMillis;
    flashDuration = random16(1200, 2500);
    flashStep = 0;
    isFlashing = true;
  }

  if (currentMillis-flashStartTime < flashDuration) {
    if (flashStep == 0) {
      for (uint8_t magic : effects) {
        leds[magic].setHSV(random8(0,255), random8(10,200), random8(30,200));//CRGB::Purple;
      }
      FastLED.show();
      flashStep = 1;
    } else if (flashStep == 1) {
      for (uint8_t magic : effects) {
        leds[magic].setHSV(0, 0, 0);
      }
      FastLED.show();
      flashStep = 2;
    }
  } else {
    // Reset LEDs to black when the flashing is done
    for (uint8_t magic : effects) {
      leds[magic] = CRGB::Black;
    }
    FastLED.show();
    isFlashing = false;
  }
}

void lightningEffect() {
  // uint8_t totalHitspots = sizeof(effects) / sizeof(effects[0]);  // total potential hitspots
  for (uint8_t i = minimumNumberOfLightningStrikes; i<=random8(1, maximumNumberOfLightningStrikes); i++) {
    Serial.println((String) "Hitting lightning strike " + i);
    uint8_t sizeOfStrike = random(1, totalEffects);  // how many leds are going to get struck
    uint8_t localEffects[totalEffects];
    for (int j = 0; j<totalEffects-1; j++) {
      localEffects[j] = effects[j];
    }
    int dimmer = 1;
    if (i == 1) dimmer = 5;
    else dimmer = random8(1, 3);

    for (uint8_t c = 0; c<totalEffects-1; c++) {
      uint8_t j = random(0, totalEffects - c);
      uint8_t t = localEffects[c];
      localEffects[j] = t;
    }
    for (uint8_t j = 0; j<sizeOfStrike; j++) {
      leds[localEffects[j]] = CRGB::Black;
    }
    FastLED.show();
    for (uint8_t j=0; j<sizeOfStrike; j++) {
      leds[localEffects[j]].setHSV(255, 0, 255 / dimmer);
    }
    FastLED.show();
    delay(random8(4, 10));
    for (uint8_t j=0; j<sizeOfStrike; j++) {
      leds[localEffects[j]] = CRGB::Black;
    }
    FastLED.show();
    if (i == 1) delay(150);
    delay(50 + random8(100));
  }
}

void resetEffectFlags() { 
  isFlashing = false;
  wandBattleInProgress = false;
  explosionEffectInProgress = false;
  houseColorEffectInProgress = false;
  houseColorChaseEffectRunning = false;
}

bool startupAudioPlayed = false;
// // music track definitions
#define MUSIC_HEDWIGS_THEME 2
#define MUSIC_KNOCKTURN_ALLEY 1
#define MUSIC_VOLDEMORT 3
// unsigned long startupAudioTime = 0;
// enum AudioSequence {
//   AS_INIT,
//   AS_STEP1,
//   AS_STEP2,
//   AS_STEP3,
//   AS_DONE
// };
#define HEDWIG_STARTUP_SCENE 2
#define KNOCKTURN_STARTUP_SCENE 3

// AudioSequence startupAudioSequence = AS_INIT;
// int startupSceneChosen = -1;
// unsigned long audioVolumeStepDown = 0;
// int currentVolume = 30;
// bool hedwigAudioLoopBegun = false;
// unsigned long audioStepdownTime = 0;
// unsigned long lastAudioStepdownTime = 0;
// CRGB hedwigColors[] = { CRGB::Purple, CRGB::Blue, CRGB::Green, CRGB::Yellow };
// int hedwigTheme[] = {624, 416, 1249, 416, 624, 416, 6, 6, 1249, 416, 624, 416, 1249, 416, 624, 416, 624, 416, 624, 416, 1249, 416, 624, 416, 416, 624, 416, 624, 416, 624, 416, 1249, 416, 624, 416, 416, 416, 624, 416, 624, 416, 624, 416, 624, 416, 1249, 416, 624, 416 };
// void hedwigStartupScene() {
//   Serial.println((String)"Current audio sequence is: " + startupAudioSequence);
//   Serial.println("Hedwig Startup Scene has been chosen.");
//   unsigned long currentMillis = millis();
//   if (startupAudioSequence == AS_INIT) {
//     Serial.println("AS_INIT was current stage.");
//     startupAudioTime = currentMillis;
//     startupAudioSequence = AS_STEP1;
//     Serial.println("Initializing Hedwig Startup Scene.");
//     return;
//   }
//   if (startupAudioSequence == AS_STEP1) {
//     Serial.println("Step 1");
//     if (!hedwigAudioLoopBegun == false) { 
//       audioStepdownTime = currentMillis;
//       mp3player.volume(audioVolumeStepDown);
//       mp3player.play(2);
//       hedwigAudioLoopBegun = true;
//       audioVolumeStepDown = currentVolume;
//     }
//     // we only want to play for 15 seconds..
//     if (currentMillis - startupAudioTime > 15000) { 
//       // let's move to step3 
//       startupAudioSequence = AS_STEP2;
//     }
//     // for (int note : hedwigTheme) {
//     //   int color = random(1,4);
//     //   Serial.println((String)color + " note");
//     //   delay(note);
    
//     // int tempo = 144;
//     // int wholeNote = (60000 * 4) / tempo;
//     // int divider = 0;
//     // int noteDuration = 0;
//     // int melody[] = { 2, 4, -4, 8, 4, 2, 4, -2, -2, -4, 8, 4, 2, 4, -1, 4, -4, 8, 4, 2, 4, 2, 4, 2, 4, -4, 8, 4, 2, 4, -1, 4, 2, 4, 2, 4, 2, 4, -4, 8, 4, 2, 4, -1, 4, 4, 2, 4, 2, 4, 2, 4, 2, 4, -4, 8, 4, 2, 4, -1 }; 
//     // for (int note : melody) {
//     //   divider = melody[note];
//     //   if (divider > 0) {
//     //     noteDuration = (wholeNote) / divider;
//     //   } else if (divider < 0) { 
//     //     noteDuration = (wholeNote) / abs(divider);
//     //     noteDuration *= 1.5;
//     //   }
//     //   Serial.println((String)"note duration is: " + noteDuration);
//     //   delay(noteDuration);
//   }
// }

// void knockturnStartupScene() {
//   Serial.println("Knockturn Startup Scene has been chosen.");
// }

// void startupScene() { 
//   switch (startupSceneChosen) {
//     case -1: 
//       startupSceneChosen = 1;
//       break;
//     case 1:
//       hedwigStartupScene();
//       break;
//     case 2: 
//       Serial.println("should not hit here.");
//       break;
//     default:
//       break;
//   }
//   Serial.println("Leaving switch");
// }

// void startupScene() {
//   Serial.println("In StartupScene");
//   // we'll never get here unless startupAudioPlayed is false..
//   switch (startupSceneChosen) { 
//     case -1: 
//       int c = random(1,100);
//       if (c%2 == 0) {
//         Serial.println((String) c + " was our number.  which %2 = " + (c%2) + " Starting up with hedwig scene");
//         startupSceneChosen = 1;
//         break;
//        } else {
//         Serial.println((String) c + " was our number.  Starting up with knockturn scene");
//         startupSceneChosen = 2;
//       }
//       break;
//     case 1:
//       Serial.println("Calling hedwig thing.");
// //      hedwigStartupScene();
//       break;
//     case 2: 
//       Serial.println("Calling knockturn thing.");
//    //   knockturnStartupScene();
//       break;
//     default:
//       Serial.println((String)"hit default.  value was " + startupSceneChosen);
//       break;
//   }
//   Serial.println("Leaving StartupScenee...");
// }

void setup() {
  unsigned long currentMillis = millis();
  Serial.begin(9600);
  randomSeed(analogRead(0));
  while (!Serial);
  mp3Serial.begin(9600);
  isAudioEnabled = true;// digitalRead(MAIN_AUDIO_PIN) == HIGH;
  delay(1500);
  Serial.println("Welcome to knockturn!");
  while (!mp3Serial);
  if (!mp3player.begin(mp3Serial, /* isACK = */ true, /* doReset = */ false)) {
    Serial.println("Unable to start mp3 player module.");
    while(true);
  }
  Serial.println("mp3 player module initialized.");
  // mp3player.stop();
  mp3player.EQ(DFPLAYER_EQ_CLASSIC);  
  FastLED.addLeds<LED_STRIP_TYPE, LED_PIN, LED_COLOR_MODE>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip)
    .setDither(MAX_BRIGHTNESS < 255);

  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2050);
  FastLED.clear();  // initialize LEDs to off.
  msSinceLastSpellEffect = currentMillis;
  lastLanternReset = currentMillis;
  lastGrateReset = currentMillis;
  // power button setup
  pinMode(MAIN_POWER_BUTTON_PIN, INPUT);
  //pinMode(MAIN_AUDIO_PIN, INPUT);

  // don't believe that we need this.
  // pinMode(SW_RESET_PIN, INPUT);
  // digitalWrite(SW_RESET_PIN, LOW);
  resetEffectFlags();
  isPowered = true;
  // Serial.println((String)"Value of main power button: " + digitalRead(MAIN_POWER_BUTTON_PIN));
  // Serial.println("10 seconds to change the value.");
  // delay(10000);
  // Serial.println((String)"Value of main power button: " + digitalRead(MAIN_POWER_BUTTON_PIN));

  //isAudioEnabled = digitalRead(MAIN_AUDIO_PIN) == HIGH;
  resetAllToDefault();
  startupAudioPlayed = false;
  // startupAudioSequence = -1;
  lastMainLoopReset = currentMillis;
  Serial.println("Startup complete.");
}


// void setup() {
//   unsigned long currentMillis = millis();
//   Serial.begin(9600);
//   randomSeed(analogRead(0));
//   while (!Serial);
//   // startupAudioSequence = AS_INIT;
//   lastMainLoopReset = currentMillis;
//   Serial.println("Startup complete!");
// }

void startupScene() { 
  if ((random(0,100)%2) == 0) { 
    Serial.println("Playing hedwig.");
    mp3player.volume(30);
    Serial.println("Whee");
    mp3player.play(MUSIC_HEDWIGS_THEME);
    Serial.println("Whee");
    startupAudioPlayed = true;
    Serial.println("Returning");
  } else { 
    Serial.println("Playing knockturn");
    mp3player.volume(30);
    Serial.println("whee");
    mp3player.play(MUSIC_KNOCKTURN_ALLEY);
    Serial.println("Wheeeee");
    startupAudioPlayed = true;
    Serial.println("Returning");
  }
}
unsigned long audioRuntime = 60000;
unsigned long audioStartTime = 0;
bool audioRunning = false;

bool powered = true;
void loop() {
  Serial.println("Checking power...");
  unsigned long currentMillis = millis();
  if (false == true) { // change back
    if (!isPowered) { 
      Serial.println("Power off still");
      delay(10);
      return;
    }
    Serial.println("power off");
    // we may want to consider doing a soft reset here when we turn back on.
    startupAudioPlayed = false;
    powerdownLights();
    mp3player.stop();
    resetEffectFlags();
    return;
  } else {
    Serial.println("Here!");
    // power button is ON
    isPowered = true;
    if (!startupAudioPlayed) {
      Serial.println("Playing startup audio.");
      startupScene();
      audioRunning = true;
      audioStartTime = currentMillis;
      return;
    }
    if (audioRunning && startupAudioPlayed && (currentMillis - audioStartTime) > audioRuntime) {
      mp3player.stop();
      audioRunning = false;
      Serial.println("Stopped audio.");
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
    // Serial.println("Checking for light effects.");
    if (!isFlashing && !wandBattleInProgress && !explosionEffectInProgress && !houseColorChaseEffectRunning && !houseColorEffectInProgress) {
      if (currentMillis - lastMainLoopReset > 200) {
        resetAllToDefault();
        lastMainLoopReset = currentMillis;
      }
    }
    if (useSpellEffects) {
      if (currentMillis-msSinceLastSpellEffect > minTicksUntilSpellEffects) {
        if ((currentMillis-msSinceLastSpellEffect) > random(minTicksUntilSpellEffects, maxTicksUntilSpellEffects)) {
          Serial.println((String)"Spell effects have been triggered. millis was: " + msSinceLastSpellEffect);
          triggerSpellEffects();
          msSinceLastSpellEffect = currentMillis;
        }
      }
    }
  }
}
