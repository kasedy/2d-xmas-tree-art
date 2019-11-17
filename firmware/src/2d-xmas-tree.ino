/* 
  2D XMAS Tree Code
  charlieplexing christmas decoration
  5 Pins with 20 LED´s

  Designed by:

    https://www.designer2k2.at

  Enhanced by:

    https://github.com/kasedy

      1. Converted to PlatformIo project for ease run animation compressing script and setting fusses
      2. More animations
      3. Significantly reduced memory footprint
      4. Improved power consumption optimizations

  MIT License

  Copyright (c) [2018] [Stephan Martin]

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "helpers.h"

#include <avr/pgmspace.h>
#include <util/delay.h>

#define NUM_LEDS 20

#if !defined(COMPILED_ANIMATION) && !defined(ANIMATION_END_FRAMES)
  #define COMPILED_ANIMATION {\
    0x03, 0x00, 0xC0, 0x03, 0x00, 0x40, 0x0D, 0x00, \
    0x28, 0x11, 0x00, 0xE0, 0x0E, 0x00, 0x40, 0x04, \
    0x01, 0x93, 0x80, 0x00, 0xE0, 0x20, 0x00, 0x00, \
    0x0C, 0x12, 0x14, 0x40, 0x10, 0x30, 0x00, 0x02, \
    0x08, 0x00, 0x40 \
  }
  #define ANIMATION_END_FRAMES {14}
#endif

static const uint8_t animation[] PROGMEM = COMPILED_ANIMATION;
static const uint16_t animationEndFrames[] = ANIMATION_END_FRAMES;
static uint8_t currentAnimation = 0;

// The delay in ms between the Frames when displaying preprogramed animation.
#define PATTERN_DELAY 500

// Intro time (how long the random change happens) in ms:
#define STARTS_ANIMATION_TIME_MS 10000

// This holds the pin configuration to make the 20 led charlieplexing.
struct LedControlPin {
  uint8_t pvcc : 4;
  uint8_t pgnd : 4;
};

const LedControlPin ledControlPins[] = {
  {0, 1},
  {1, 0},
  {1, 2},
  {2, 1},
  {2, 3},
  {3, 2},
  {3, 4},
  {4, 3},
  {0, 2},
  {2, 0},
  {1, 3},
  {3, 1},
  {2, 4},
  {4, 2},
  {0, 3},
  {3, 0},
  {1, 4},
  {4, 1},
  {0, 4},
  {4, 0},
};

// Placeholder for all current leds.
uint8_t ledstate[NUM_LEDS] = {};

void setup() {
  // Disable ADC (power saving):
  ADCSRA = 0;

  // Shut down Timer/Counter1, Timer/Counter0, ISI, ADC
  PRR = 1;
}

void loop() {
  showStarsAnimation();
  showNextPreprogrammedAnimation();
}

uint16_t fastRandom() {
  static uint16_t seed = 0xBEEF;
  seed = (seed * 2053 + 13849) % 65536;
  return seed;
}

void showStarsAnimation() {
  // Every 25ms change a random led to a random state.
  // Extingush command makes LEDs flikering.
  unsigned long instroStartTime = millis();
  while (millis() - instroStartTime < STARTS_ANIMATION_TIME_MS) {
    uint16_t randomValue = fastRandom();
    int ledtoset = randomValue % NUM_LEDS;
    bool ledbool = randomValue & 0b10000000;
    ledstate[ledtoset] = ledbool;
    showleds(25);
    extinguish();
    #if F_CPU >= 1000000L
      _delay_ms(2);
    #endif
  }
}

void showNextPreprogrammedAnimation() {
  if (ARRAY_LEN(animationEndFrames) == 0) {
    return;
  }

  uint16_t startFrame = 0;
  if (currentAnimation != 0) {
    startFrame = animationEndFrames[currentAnimation - 1];
  }
  uint16_t endFrame = animationEndFrames[currentAnimation];

  // Show preprogrammed animation.
  for (uint16_t iFrame = startFrame; iFrame < endFrame; ++iFrame) {
    uint16_t startBitPosition = iFrame * NUM_LEDS;
    uint8_t byteIndex = startBitPosition / 8;
    uint8_t bitIndex = startBitPosition % 8;
    uint8_t state = pgm_read_byte(animation + byteIndex);
    for (int i = 0; i < NUM_LEDS; ++i) {
      if (bitIndex == 8) {
        bitIndex = 0;
        byteIndex += 1;
        state = pgm_read_byte(animation + byteIndex);
      }
      ledstate[i] = (state & (1 << bitIndex));
      bitIndex += 1;
    }
    showleds(PATTERN_DELAY); 
  }

  if (++currentAnimation >= ARRAY_LEN(animationEndFrames)) {
    currentAnimation = 0;
  }
}

//-------------------------------------------------------------------------------------

void showleds(unsigned int showTimeMs) {
  // This cycles trough all the LEDs in the array and show them one-by-one. That 
  // happens so quickly that human eye sees like LEDs shine all together.
  unsigned long startTime = millis();
  while (true) {
    for (int i = 0; i < NUM_LEDS; i++) {
      bool ledOn = ledstate[i];
      if (ledOn) {
        showled(i);
      }
      if (millis() - startTime > showTimeMs) {
        return;
      }
    }
  }
}

void showled(uint8_t led) {
  // Lookup the pin for high and low:
  const LedControlPin &pinouts = ledControlPins[led];

  // Set pins that contold LED to output. The other pins will be set to input.
  DDRB = (1 << pinouts.pvcc) | (1 << pinouts.pgnd);

  // Write the status.
  PORTB = (1 << pinouts.pvcc); // set HIGH

  // Dont extinguish. Let LED shine while microcontroller decides which LED to 
  // light next.
}

void extinguish() {
  // This tristates all pins
  DDRB &= ~(0b00011111); // Input state for pins 0-4 pins
  PORTB &= ~(0b00011111); //ensure pullups are off.
}
