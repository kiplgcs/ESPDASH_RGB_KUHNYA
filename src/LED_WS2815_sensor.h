#pragma once

#include <Arduino.h>

#include "web.h"
#include "LED_WS2815.h"

inline float KITCHEN_DISTANCE_NEAR_ENTER_M = 1.20f; // Порог входа в ближнюю зону (м).
inline float KITCHEN_DISTANCE_FAR_ENTER_M = 3.00f; // Порог входа в дальнюю зону (м).
inline uint32_t KITCHEN_TRANSITION_WAIT_MS = 1200; // Задержка перехода Ближняя -> Дальняя (мс).
inline uint32_t KITCHEN_LIGHTS_OFF_DELAY_MS = 7000; // Задержка полного выключения ленты после выхода из дальней зоны (мс).
inline int KITCHEN_LED_COUNT = NUM_LEDS; // Число светодиодов ленты для UI (справочный параметр).
inline String KITCHEN_NEAR_ENTRY_EFFECT = "edge_white"; // Эффект на переходе Дальняя -> Ближняя.
inline int KITCHEN_WORK_WHITE_BRIGHTNESS = 220; // Яркость рабочего белого света.

namespace KitchenWs2815Sensor {

enum Zone : uint8_t {
  ZoneNone = 0,
  ZoneFar,
  ZoneNear
};

static Zone gZone = ZoneNone;
static bool gNearEntryEffectRunning = false;
static uint32_t gNearEntryEffectStartedAt = 0;
static uint32_t gNearToFarStartedAt = 0;
static uint32_t gFarExitStartedAt = 0;
static bool gNearToFarPending = false;
static bool gFarExitPending = false;

static inline uint16_t activeLedCount() {
  return static_cast<uint16_t>(constrain(KITCHEN_LED_COUNT, 1, NUM_LEDS));
}

static inline int findPatternIndexByName(const String &name) {
  initPatterns();
  for (size_t i = 0; i < LED_PATTERN_COUNT; ++i) {
    if (name.equalsIgnoreCase(LED_PATTERNS[i].name)) {
      return static_cast<int>(i);
    }
  }
  return 0;
}

static inline void fillWhiteWorkLight() {
  new_bright = constrain(KITCHEN_WORK_WHITE_BRIGHTNESS, 1, 255);
  ColorRGB = true;
  LEDColor = "#FFFFFF";
  clearToOrdered(RgbColor(255, 255, 255));
  ledStrip.SetBrightness(new_bright);
  ledStrip.Show();
}

static inline void applyFarAmbientLight() {
  ColorRGB = LedColorMode.equalsIgnoreCase("manual");
  new_bright = constrain(LedBrightness, 10, 255);
}

static inline bool isRadarDistanceValid(float distanceM) {
  const float minRangeM = 0.5f;
  const float maxRangeM = 15.0f;
  return RadarAverageValidSensors > 0 && distanceM >= minRangeM && distanceM <= maxRangeM;
}

static inline Zone resolveZone(float distanceM) {
  if (!isRadarDistanceValid(distanceM)) {
    return ZoneNone;
  }

  const float nearEnter = max(0.5f, KITCHEN_DISTANCE_NEAR_ENTER_M);
  const float farEnter = max(nearEnter, KITCHEN_DISTANCE_FAR_ENTER_M);
  
  if (distanceM <= nearEnter) return ZoneNear;
  if (distanceM <= farEnter) return ZoneFar;
  return ZoneNone;
}

static inline void powerOffStripNow() {
  Pow_WS2815 = false;
  ColorRGB = false;
  gNearEntryEffectRunning = false;
}

static inline void startNearEntryEffect() {
  gNearEntryEffectRunning = true;
  gNearEntryEffectStartedAt = millis();
  Pow_WS2815 = true;
  ColorRGB = false;

  if (KITCHEN_NEAR_ENTRY_EFFECT.equalsIgnoreCase("edge_white")) {
    clearToOrdered(RgbColor(0, 0, 0));
    ledStrip.SetBrightness(constrain(LedBrightness, 10, 255));
    ledStrip.Show();
    return;
  }

  const int patternIndex = findPatternIndexByName(KITCHEN_NEAR_ENTRY_EFFECT);
  currentPatternIndex = patternIndex;
  LedPattern = LED_PATTERNS[patternIndex].name;
}

static inline bool runNearEntryEffect() {
  if (!gNearEntryEffectRunning) {
    return false;
  }

  const uint32_t elapsed = millis() - gNearEntryEffectStartedAt;
  const uint16_t leds = activeLedCount();

  if (KITCHEN_NEAR_ENTRY_EFFECT.equalsIgnoreCase("edge_white")) {
    const uint16_t half = leds / 2;
    uint16_t steps = half > 0 ? half : 1;
    uint16_t lit = map(min<uint32_t>(elapsed, 1000), 0, 1000, 0, steps);
    clearToOrdered(RgbColor(0, 0, 0));
    for (uint16_t i = 0; i < lit; ++i) {
      setPixelColorOrdered(i, RgbColor(255, 255, 255));
      setPixelColorOrdered((leds - 1) - i, RgbColor(255, 255, 255));
    }
    ledStrip.SetBrightness(constrain(LedBrightness, 10, 255));
    ledStrip.Show();
    if (elapsed >= 1000) {
      gNearEntryEffectRunning = false;
      return false;
    }
    return true;
  }

  if (elapsed < 1200) {
    ledStrip.SetBrightness(constrain(LedBrightness, 10, 255));
    renderPattern(static_cast<uint8_t>(elapsed / 30));
    ledStrip.Show();
    return true;
  }

  gNearEntryEffectRunning = false;
  return false;
}

static inline bool sensorAutoModeEnabled() {
  return SetRGB.equalsIgnoreCase("auto") || Pow_WS2815_autosvet;
}

inline void setup() {
  gZone = ZoneNone;
  gNearEntryEffectRunning = false;
  gNearToFarPending = false;
  gFarExitPending = false;
}

inline void loop() {
  if (!sensorAutoModeEnabled()) {
    gZone = ZoneNone;
    gNearEntryEffectRunning = false;
    gNearToFarPending = false;
    gFarExitPending = false;
    return;
  }

  const uint32_t now = millis();
  const Zone nextZone = resolveZone(RadarAverageDistanceM);

  if (gNearEntryEffectRunning) {
    if (!runNearEntryEffect()) {
      fillWhiteWorkLight();
    }
  }

  if (nextZone == ZoneNear) {
    gFarExitPending = false;
    gNearToFarPending = false;

    if (gZone == ZoneFar || gZone == ZoneNone) {
      gNearEntryEffectRunning = false;
      Pow_WS2815 = true;
      fillWhiteWorkLight();
    }
    gZone = ZoneNear;
    return;
  }

  if (nextZone == ZoneFar) {
    gFarExitPending = false;

    if (gZone == ZoneNone) {
      Pow_WS2815 = true;
      applyFarAmbientLight();
      gZone = ZoneFar;
      return;
    }

    if (gZone == ZoneNear) {
      if (!gNearToFarPending) {
        gNearToFarPending = true;
        gNearToFarStartedAt = now;
        return;
      }
      if (now - gNearToFarStartedAt < KITCHEN_TRANSITION_WAIT_MS) {
        return;
      }
      gNearToFarPending = false;
      Pow_WS2815 = true;
      applyFarAmbientLight();
      gZone = ZoneFar;
      return;
    }

    gZone = ZoneFar;
    return;
  }

  gNearToFarPending = false;

  if (gZone == ZoneFar || gZone == ZoneNear) {
    if (!gFarExitPending) {
      gFarExitPending = true;
      gFarExitStartedAt = now;
      return;
    }

    if (now - gFarExitStartedAt >= KITCHEN_LIGHTS_OFF_DELAY_MS) {
      powerOffStripNow();
      gFarExitPending = false;
      gZone = ZoneNone;
      return;
    }
  }

  gZone = ZoneNone;
}

} // namespace KitchenWs2815Sensor

inline void setup_LED_WS2815_sensor() {
  KitchenWs2815Sensor::setup();
}

inline void loop_LED_WS2815_sensor() {
  KitchenWs2815Sensor::loop();
}