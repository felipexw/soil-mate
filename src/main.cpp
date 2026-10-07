// SoilMate for the Arduino Pro Mini: shows the plant's mood on an 8x8 LED matrix.
// Ported from the original SoilMate sketch (Wi-Fi and web server removed).
//
// Wiring:
//   Matrix DIN -> D4    Matrix CS -> D5    Matrix CLK -> D6
//   Sensor AOUT -> A0   VCC -> 5V, GND -> GND on both
//
// D10 to D13 are left free for the ISP programmer.

#include <Arduino.h>
#include <MD_MAX72XX.h>

const uint8_t DATA_PIN = 4;
const uint8_t CS_PIN = 5;
const uint8_t CLK_PIN = 6;
const uint8_t SENSOR_PIN = A0;

// 0 (dimmest) to 15. The power estimates in the README assume 1.
const uint8_t BRIGHTNESS = 1;

// Raw 10-bit readings (0-1023). The capacitive sensor reads higher when drier.
// These are the original sketch's 3000/1500 thresholds converted by voltage;
// calibrate them with the values printed on the serial monitor.
const int DRY_ABOVE = 495;
const int WET_BELOW = 247;

const uint8_t SAMPLE_COUNT = 10;
const unsigned long SAMPLE_INTERVAL_MS = 1000;

const unsigned long BLINK_EVERY_MS = 60000;     // happy: blink
const unsigned long TONGUE_EVERY_MS = 3600000;  // happy: tongue out
const unsigned long ANGRY_EVERY_MS = 120000;    // dry or wet: angry flash
const uint8_t HEART_STEPS = 10;                 // seconds of heart after being watered

// Software SPI, so the matrix can sit on any three digital pins.
MD_MAX72XX mx(MD_MAX72XX::FC16_HW, DATA_PIN, CLK_PIN, CS_PIN, 1);

const uint8_t SMILE[8] = {
  0b00100000,
  0b01000100,
  0b00100010,
  0b00000010,
  0b00000010,
  0b00100010,
  0b01000100,
  0b00100000
};

const uint8_t SMILE2[8] = {
  0b01000000,
  0b10000000,
  0b01001100,
  0b00001010,
  0b00001010,
  0b01001100,
  0b10000000,
  0b01000000
};

const uint8_t TONGUE[8] = {
  0b01000000,
  0b10001000,
  0b01000110,
  0b00000110,
  0b00000100,
  0b01000100,
  0b10001000,
  0b01000000
};

const uint8_t HEART[8] = {
  0b00111000,
  0b01111100,
  0b01111110,
  0b00111111,
  0b00111111,
  0b01111110,
  0b01111100,
  0b00111000
};

const uint8_t SAD_FACE[8] = {
  0b00000000,
  0b00100010,
  0b01100100,
  0b00000100,
  0b00000100,
  0b01100100,
  0b00100010,
  0b00000000
};

const uint8_t ANGRY_FACE[8] = {
  0b00000000,
  0b01000010,
  0b00100100,
  0b00000100,
  0b00000100,
  0b00100100,
  0b01000010,
  0b00000000
};

enum Mood { DRY, OPTIMAL, WET };

Mood mood = OPTIMAL;

int samples[SAMPLE_COUNT];
uint8_t sampleIndex = 0;
unsigned long lastSampleAt = 0;

unsigned long lastBlinkAt = 0;
unsigned long lastTongueAt = 0;
unsigned long lastAngryAt = 0;

bool animating = false;
unsigned long animationStartedAt = 0;
unsigned long animationDuration = 0;

uint8_t heartStepsLeft = 0;
unsigned long lastHeartStepAt = 0;

void show(const uint8_t* pattern) {
  for (uint8_t row = 0; row < 8; row++) {
    mx.setRow(0, row, pattern[row]);
  }
}

const uint8_t* restingFace() {
  return mood == OPTIMAL ? SMILE : SAD_FACE;
}

void playAnimation(const uint8_t* pattern, unsigned long duration, unsigned long now) {
  show(pattern);
  animating = true;
  animationStartedAt = now;
  animationDuration = duration;
}

int averageMoisture() {
  long sum = 0;
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    sum += samples[i];
  }
  return sum / SAMPLE_COUNT;
}

Mood moodFor(int moisture) {
  if (moisture > DRY_ABOVE) return DRY;
  if (moisture < WET_BELOW) return WET;
  return OPTIMAL;
}

const char* moodName(Mood m) {
  return m == DRY ? "DRY" : (m == WET ? "WET" : "OPTIMAL");
}

void setup() {
  Serial.begin(115200);

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, BRIGHTNESS);
  mx.clear();

  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    samples[i] = analogRead(SENSOR_PIN);
    delay(100);
  }

  show(SMILE);
}

void loop() {
  unsigned long now = millis();
  
  // Heart celebration: alternates heart and smile, pausing everything else.
  if (heartStepsLeft > 0) {
    if (now - lastHeartStepAt >= 1000) {
      lastHeartStepAt = now;
      heartStepsLeft--;
      if (heartStepsLeft == 0) {
        show(restingFace());
      } else {
        show(heartStepsLeft % 2 ? SMILE : HEART);
      }
    }
    return;
  }

  if (animating && now - animationStartedAt >= animationDuration) {
    animating = false;
    show(restingFace());
  }

  if (now - lastSampleAt >= SAMPLE_INTERVAL_MS) {
    lastSampleAt = now;

    samples[sampleIndex] = analogRead(SENSOR_PIN);
    sampleIndex = (sampleIndex + 1) % SAMPLE_COUNT;

    int moisture = averageMoisture();
    Serial.print(F("Moisture value (average): "));
    Serial.println(moisture);

    Mood newMood = moodFor(moisture);
    if (newMood != mood) {
      bool wasDry = mood == DRY;
      mood = newMood;
      Serial.print(F("State changed to: "));
      Serial.println(moodName(mood));

      animating = false;
      lastBlinkAt = now;
      lastTongueAt = now;
      lastAngryAt = now;

      if (wasDry) {
        heartStepsLeft = HEART_STEPS;
        lastHeartStepAt = now;
        show(HEART);
        return;
      }
      show(restingFace());
    }
  }

  if (animating) return;

  if (mood == OPTIMAL) {
    if (now - lastBlinkAt > BLINK_EVERY_MS) {
      lastBlinkAt = now;
      playAnimation(SMILE2, 5000, now);
    } else if (now - lastTongueAt > TONGUE_EVERY_MS) {
      lastTongueAt = now;
      playAnimation(TONGUE, 4000, now);
    }
  } else if (now - lastAngryAt > ANGRY_EVERY_MS) {
    lastAngryAt = now;
    playAnimation(ANGRY_FACE, 5000, now);
  }
}
