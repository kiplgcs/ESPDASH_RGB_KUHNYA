#pragma once

#include <Arduino.h>
#include <cstring>
#include "LED_WS2815.h"

// -------------------- Подключение LD2420 --------------------
inline int LD2420_RX_PIN = 16;
inline int LD2420_TX_PIN = 17;
inline uint32_t LD2420_BAUD = 115200UL;
inline uint8_t LD2420_UART_PORT = 2;

// -------------------- Настройки автоматики кухни --------------------
inline float KITCHEN_DISTANCE_NEAR_ENTER_M = 3.0f; // 0..nearEnter = ближняя зона
inline float KITCHEN_NEAR_HYSTERESIS_M = 0.4f;     // Выход из ближней: nearEnter + hysteresis
inline float KITCHEN_DISTANCE_FAR_ENTER_M = 5.0f;  // Верхняя граница дальней зоны
inline float KITCHEN_FAR_HYSTERESIS_M = 0.2f;      // Дополнительный зазор дальней зоны
inline uint32_t KITCHEN_TRANSITION_WAIT_MS = 700UL;
inline uint32_t KITCHEN_LIGHTS_OFF_DELAY_MS = 20000UL;
inline int KITCHEN_LED_COUNT = 180;
inline String KITCHEN_NEAR_ENTRY_EFFECT = "edge_white";
inline uint32_t KITCHEN_NEAR_ENTRY_EFFECT_MS = 700UL;
inline uint8_t KITCHEN_WORK_WHITE_BRIGHTNESS = 220;
inline uint16_t KITCHEN_SENSOR_CONFIRM_MS = 350UL;
inline uint32_t KITCHEN_SIGNAL_HOLD_MS = 1200UL;
inline uint32_t LD2420_ALT_BAUD = 256000UL; // Для старых прошивок LD2420 (<1.5.3)
inline uint32_t LD2420_BAUD_RETRY_MS = 2500UL;
inline bool LD2420_FORCE_SIMPLE_MODE = false; // По умолчанию не переводим датчик в другой режим без явного запроса.
inline bool LD2420_DEBUG_RAW_UART = false;    // Отладка: зеркалировать сырые байты LD2420 в Serial.
inline String LD2420_PARSE_MODE = "auto";     // "binary" | "text" | "auto"
inline float LD2420_DISTANCE_GRAPH_M = 0.0f;  // Источник для UI-графика расстояния.

enum class KitchenZone : uint8_t { NONE = 0, FAR, NEAR };
enum class KitchenLightingState : uint8_t { OFF = 0, NEAR_EFFECT, NEAR_WHITE, FAR_RGB };

struct LD2420Reading {
  bool frameValid = false;
  bool hasTarget = false;
  float distanceM = 99.0f;
};

static HardwareSerial *gLd2420Serial = nullptr;
static KitchenZone gStableZone = KitchenZone::NONE;
static KitchenZone gCandidateZone = KitchenZone::NONE;
static uint32_t gZoneCandidateSince = 0;
static uint32_t gLastSeenAt = 0;
static KitchenLightingState gLightingState = KitchenLightingState::OFF;
static uint32_t gStateStartedAt = 0;
static bool gNearCycleActive = false;
static uint32_t gOffDelayStartedAt = 0;
static String gLineBuffer;
static uint8_t gRxBuffer[192] = {0};
static size_t gRxLen = 0;
// Совместимость со старыми сборками/кэшем IntelliSense, где мог остаться старый символ.
static uint32_t gLastRxByteAt = 0;
static uint32_t gLd2420LastValidFrameAt = 0;
static uint32_t gCurrentBaud = LD2420_BAUD;
static bool gTriedAltBaud = false;
static bool gBaudLocked = false;
static float gLastDistanceM = 99.0f;
static int gAppliedRxPin = -1;
static int gAppliedTxPin = -1;
static uint32_t gAppliedBaud = 0;
static uint8_t gAppliedUartPort = 255;

// Forward declarations для IntelliSense/компиляторов с более строгим одно-проходным разбором заголовка.
static inline KitchenZone ld2420MeasureZone(bool hasTarget, float dM);

static inline void ld2420SetState(KitchenLightingState next, uint32_t now) {
  gLightingState = next;
  gStateStartedAt = now;
}

static inline void ld2420BeginSerial(uint32_t baud) {
  gCurrentBaud = baud;
  if (!gLd2420Serial) { return; }
  gLd2420Serial->end();
  gLd2420Serial->begin(gCurrentBaud, SERIAL_8N1, LD2420_RX_PIN, LD2420_TX_PIN);
  gAppliedRxPin = LD2420_RX_PIN;
  gAppliedTxPin = LD2420_TX_PIN;
  gAppliedBaud = gCurrentBaud;
  gAppliedUartPort = LD2420_UART_PORT;
}

static inline void ld2420WriteFrame(const uint8_t *data, size_t len) {
  if (!gLd2420Serial || !data || len == 0) { return; }
  gLd2420Serial->write(data, len);
  gLd2420Serial->flush();
}

static inline void ld2420ConfigureSimpleMode() {
  // Источник: ESPHome LD2420 protocol docs (enable config -> set mode 0x0064 -> disable config).
  static const uint8_t kEnableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x02, 0x00, 0x04, 0x03, 0x02, 0x01};
  static const uint8_t kSetSimpleMode[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01};
  static const uint8_t kDisableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
  ld2420WriteFrame(kEnableCfg, sizeof(kEnableCfg));
  delay(30);
  ld2420WriteFrame(kSetSimpleMode, sizeof(kSetSimpleMode));
  delay(30);
  ld2420WriteFrame(kDisableCfg, sizeof(kDisableCfg));
}

static inline bool ld2420ExtractDistance(const String &line, float &distanceM) {
  if (!line.length()) { return false; }

  int start = -1;
  int end = -1;
  for (int i = 0; i < line.length(); ++i) {
    const char c = line.charAt(i);
    if ((c >= '0' && c <= '9') || c == '.' || c == ',') {
      start = i;
      break;
    }
  }
  if (start < 0) { return false; }

  for (int i = start; i < line.length(); ++i) {
    const char c = line.charAt(i);
    if (!((c >= '0' && c <= '9') || c == '.' || c == ',')) {
      end = i;
      break;
    }
  }
  if (end < 0) { end = line.length(); }

  String numeric = line.substring(start, end);
  numeric.replace(',', '.');
  const float raw = numeric.toFloat();
  if (raw <= 0.0f) { return false; }

  String lower = line;
  lower.toLowerCase();

  if (lower.indexOf("mm") >= 0) {
    distanceM = raw / 1000.0f;
  } else if (lower.indexOf("cm") >= 0) {
    distanceM = raw / 100.0f;
  } else if (lower.indexOf("m") >= 0) {
    distanceM = raw;
  } else {
    distanceM = (raw > 12.0f) ? (raw / 100.0f) : raw;
  }

  return distanceM > 0.01f && distanceM < 15.0f;
}

static inline LD2420Reading readLD2420() {
  LD2420Reading reading;
  if (!gLd2420Serial) { return reading; }
  String mode = LD2420_PARSE_MODE;
  mode.trim();
  mode.toLowerCase();
  const bool textMode = (mode == "text" || mode == "auto");
  const bool binaryMode = (mode == "binary" || mode == "auto");

  while (gLd2420Serial->available() > 0) {
    const int raw = gLd2420Serial->read();
    if (raw < 0) { break; }
    const uint8_t b = static_cast<uint8_t>(raw);
    if (LD2420_DEBUG_RAW_UART) { Serial.write(b); }
    gLastRxByteAt = millis();

    if (gRxLen >= sizeof(gRxBuffer)) {
      memmove(gRxBuffer, gRxBuffer + 1, sizeof(gRxBuffer) - 1);
      gRxLen = sizeof(gRxBuffer) - 1;
    }
    gRxBuffer[gRxLen++] = b;

  if (!textMode) { continue; }
    const char c = static_cast<char>(b);
    if (c == '\r') { continue; }
    if (c == '\n') {
      float d = 0.0f;
      String line = gLineBuffer;
      String lower = line;
      lower.toLowerCase();
      const bool hasOn = lower.indexOf("on") >= 0;
      const bool hasOff = lower.indexOf("off") >= 0;
      if (hasOn || hasOff) { reading.frameValid = true; }
      if (ld2420ExtractDistance(line, d)) {
        reading.frameValid = true;
        gLastDistanceM = d;
        reading.distanceM = d;
        } else {
        reading.distanceM = gLastDistanceM;
      }
      if (hasOn) { reading.hasTarget = true; }
      if (hasOff) { reading.hasTarget = false; }
      if (!hasOn && !hasOff && reading.frameValid && reading.distanceM < 15.0f) {
        reading.hasTarget = true;
      }
      gLineBuffer = "";
    } else if (gLineBuffer.length() < 96) {
      gLineBuffer += c;
    } else {
      gLineBuffer = "";
    }
  }

    if (!binaryMode) { return reading; }

  // LD2420 energy-mode frame: F4 F3 F2 F1 LL LL PP DD DD ... F8 F7 F6 F5.
  while (gRxLen >= 10) {
    size_t start = 0;
    while (start + 3 < gRxLen) {
      if (gRxBuffer[start] == 0xF4 && gRxBuffer[start + 1] == 0xF3 &&
          gRxBuffer[start + 2] == 0xF2 && gRxBuffer[start + 3] == 0xF1) {
        break;
      }
      start++;
    }
    if (start > 0) {
      memmove(gRxBuffer, gRxBuffer + start, gRxLen - start);
      gRxLen -= start;
    }
    if (gRxLen < 6) { break; }

    const uint16_t frameDataLen = static_cast<uint16_t>(gRxBuffer[4]) |
                                  (static_cast<uint16_t>(gRxBuffer[5]) << 8);
    if (frameDataLen == 0 || frameDataLen > 128) {
      memmove(gRxBuffer, gRxBuffer + 1, gRxLen - 1);
      gRxLen -= 1;
      continue;
    }

    const size_t frameLen = static_cast<size_t>(frameDataLen) + 10;
    if (gRxLen < frameLen) { break; }

    const size_t footerPos = 6 + frameDataLen;
    const bool footerOk = gRxBuffer[footerPos] == 0xF8 && gRxBuffer[footerPos + 1] == 0xF7 &&
                          gRxBuffer[footerPos + 2] == 0xF6 && gRxBuffer[footerPos + 3] == 0xF5;
    if (!footerOk) {
      memmove(gRxBuffer, gRxBuffer + 1, gRxLen - 1);
      gRxLen -= 1;
      continue;
    }

    if (frameDataLen >= 3) {
      const uint8_t presence = gRxBuffer[6];
      const uint16_t rangeCm = static_cast<uint16_t>(gRxBuffer[7]) |
                               (static_cast<uint16_t>(gRxBuffer[8]) << 8);
                               reading.frameValid = true;
      
reading.hasTarget = (presence != 0x00);
      if (rangeCm > 0) {
        gLastDistanceM = static_cast<float>(rangeCm) / 100.0f;
        reading.distanceM = gLastDistanceM;
      } else {
        reading.distanceM = gLastDistanceM;
      }
    }

    memmove(gRxBuffer, gRxBuffer + frameLen, gRxLen - frameLen);
    gRxLen -= frameLen;
  }

  return reading;
}

static inline KitchenZone ld2420MeasureZone(bool hasTarget, float dM) {
  if (!hasTarget) { return KitchenZone::NONE; }

  const float nearEnter = constrain(KITCHEN_DISTANCE_NEAR_ENTER_M, 0.2f, 8.0f);
  const float nearExit = constrain(nearEnter + constrain(KITCHEN_NEAR_HYSTERESIS_M, 0.0f, 1.2f), nearEnter, 9.0f);
  const float farEnter = constrain(max(KITCHEN_DISTANCE_FAR_ENTER_M, nearExit + constrain(KITCHEN_FAR_HYSTERESIS_M, 0.0f, 2.0f)), nearExit, 10.0f);

  KITCHEN_DISTANCE_NEAR_ENTER_M = nearEnter;
  KITCHEN_DISTANCE_FAR_ENTER_M = farEnter;

  if (gStableZone == KitchenZone::NEAR) {
    if (dM <= nearExit) { return KitchenZone::NEAR; }
    if (dM <= farEnter) { return KitchenZone::FAR; }
    return KitchenZone::NONE;
  }

  if (dM <= nearEnter) { return KitchenZone::NEAR; }
  if (dM <= farEnter) { return KitchenZone::FAR; }
  return KitchenZone::NONE;
}

static inline String rgbControlModeNormalizedLD2420() {
  String mode = SetRGB;
  mode.trim();
  mode.toLowerCase();
  return mode;
}

static inline void ld2420RenderNearEntryEffect(uint32_t now) {
  const uint32_t elapsed = now - gStateStartedAt;
  const uint8_t frame = static_cast<uint8_t>((elapsed / 35UL) & 0xFFU);
  const uint16_t activeLeds = static_cast<uint16_t>(constrain(KITCHEN_LED_COUNT, 1, NUM_LEDS));

  clearToOrdered(RgbColor(0, 0, 0));
  const String effect = KITCHEN_NEAR_ENTRY_EFFECT;
  if (effect == "edge_white") {
    const float progress = min(1.0f, static_cast<float>(elapsed) / max<uint32_t>(1UL, KITCHEN_NEAR_ENTRY_EFFECT_MS));
    const uint16_t half = activeLeds / 2;
    const uint16_t litPerSide = static_cast<uint16_t>(half * progress);
    for (uint16_t i = 0; i < litPerSide; ++i) {
      setPixelColorOrdered(i, RgbColor(255, 255, 255));
      setPixelColorOrdered(activeLeds - 1 - i, RgbColor(255, 255, 255));
    }
  } else if (effect == "rainbow") fillRainbowFrame(frame);
  else if (effect == "pulse") fillPulseFrame(frame, RgbColor(255, 255, 255));
  else if (effect == "chase") fillChaseFrame(frame);
  else if (effect == "comet") fillCometFrame(frame);
  else if (effect == "color_wipe") fillColorWipe(frame, RgbColor(255, 255, 255));
  else if (effect == "theater_chase") fillTheaterChase(frame, RgbColor(255, 255, 255));
  else if (effect == "scanner") fillScanner(frame, RgbColor(255, 255, 255));
  else if (effect == "sparkle") fillSparkle(frame, RgbColor(255, 255, 255));
  else if (effect == "twinkle") fillTwinkle(frame, RgbColor(255, 255, 255));
  else if (effect == "confetti") fillConfetti(frame);
  else if (effect == "waves") fillWaves(frame);
  else if (effect == "breathe") fillBreathe(frame, RgbColor(255, 255, 255));
  else if (effect == "firefly") fillFirefly(frame);
  else if (effect == "ripple") fillRipple(frame, RgbColor(255, 255, 255));
  else if (effect == "dots") fillDots(frame);
  else if (effect == "gradient") fillGradient(frame, RgbColor(255, 255, 255));
  else if (effect == "meteor") fillMeteor(frame);
  else if (effect == "juggle") fillJuggle(frame);
  else if (effect == "aurora") fillAurora(frame);
  else fillRainbowFrame(frame);

  for (uint16_t i = activeLeds; i < NUM_LEDS; ++i) { setPixelColorOrdered(i, RgbColor(0, 0, 0)); }
  ledStrip.SetBrightness(KITCHEN_WORK_WHITE_BRIGHTNESS);
  ledStrip.Show();
}

static inline void ld2420RenderWorkWhite() {
  const uint16_t activeLeds = static_cast<uint16_t>(constrain(KITCHEN_LED_COUNT, 1, NUM_LEDS));
  clearToOrdered(RgbColor(0, 0, 0));
  for (uint16_t i = 0; i < activeLeds; ++i) {
    setPixelColorOrdered(i, RgbColor(255, 255, 255));
  }
  ledStrip.SetBrightness(KITCHEN_WORK_WHITE_BRIGHTNESS);
  ledStrip.Show();
}

static inline void ld2420RenderOff() {
  ledStrip.SetBrightness(0);
  clearToOrdered(RgbColor(0, 0, 0));
  ledStrip.Show();
}

static inline void resetLD2420Automation(uint32_t now) {
  gStableZone = KitchenZone::NONE;
  gCandidateZone = KitchenZone::NONE;
  gZoneCandidateSince = 0;
  gLastSeenAt = 0;
  gNearCycleActive = false;
  gOffDelayStartedAt = 0;
  gLineBuffer = "";
  gRxLen = 0;
  gLd2420LastValidFrameAt = 0;
  gTriedAltBaud = false;
  gBaudLocked = false;
  gLastDistanceM = 99.0f;
  ld2420SetState(KitchenLightingState::OFF, now);
}

static inline void setup_LD2420() {
  if (gLd2420Serial) {
    gLd2420Serial->end();
    delete gLd2420Serial;
  }

  gLd2420Serial = new HardwareSerial(LD2420_UART_PORT);
  ld2420BeginSerial(LD2420_BAUD);
  delay(50);
  if (LD2420_FORCE_SIMPLE_MODE) { ld2420ConfigureSimpleMode(); }
  setup_WS2815();
  resetLD2420Automation(millis());
}

static inline void ld2420ApplyRuntimeUartConfigIfChanged() {
  const bool portChanged = (gAppliedUartPort != LD2420_UART_PORT);
  const bool pinsChanged = (gAppliedRxPin != LD2420_RX_PIN) || (gAppliedTxPin != LD2420_TX_PIN);
  const bool baudChanged = (gAppliedBaud != LD2420_BAUD && !gBaudLocked);
  if (!portChanged && !pinsChanged && !baudChanged) { return; }

  if (portChanged) {
    if (gLd2420Serial) {
      gLd2420Serial->end();
      delete gLd2420Serial;
    }
    gLd2420Serial = new HardwareSerial(LD2420_UART_PORT);
  }

  ld2420BeginSerial(gBaudLocked ? gCurrentBaud : LD2420_BAUD);
  gRxLen = 0;
  gLineBuffer = "";
  gLd2420LastValidFrameAt = 0;
  gTriedAltBaud = false;
  gBaudLocked = false;
}

static inline void loop_LD2420() {
  const uint32_t now = millis();
  const String mode = rgbControlModeNormalizedLD2420();

  if (mode != "auto") {
    resetLD2420Automation(now);
    loop_WS2815();
    return;
  }

  ld2420ApplyRuntimeUartConfigIfChanged();

// Одноразовый fallback: стартуем на 115200 (новые прошивки), при отсутствии валидных кадров
  // пробуем 256000 (старые прошивки) и далее фиксируемся на рабочем baud.
  if (!gBaudLocked && !gTriedAltBaud && gLd2420LastValidFrameAt == 0 &&
      now >= LD2420_BAUD_RETRY_MS) {
    ld2420BeginSerial(LD2420_ALT_BAUD);
    gTriedAltBaud = true;
    gRxLen = 0;
    gLineBuffer = "";
  }

  LD2420Reading reading = readLD2420();
  if (reading.frameValid) {
    gLd2420LastValidFrameAt = now;
    gBaudLocked = true;
  }
  LD2420_DISTANCE_GRAPH_M = (reading.distanceM > 0.0f && reading.distanceM < 15.0f) ? reading.distanceM : gLastDistanceM;
  if (reading.hasTarget) {
    gLastSeenAt = now;
  } else if ((now - gLastSeenAt) <= KITCHEN_SIGNAL_HOLD_MS) {
    reading.hasTarget = true;
    if (gStableZone == KitchenZone::NEAR) {
      reading.distanceM = KITCHEN_DISTANCE_NEAR_ENTER_M * 0.9f;
    } else if (gStableZone == KitchenZone::FAR) {
      reading.distanceM = (KITCHEN_DISTANCE_NEAR_ENTER_M + KITCHEN_DISTANCE_FAR_ENTER_M) * 0.5f;
    }
  }

  KitchenZone measured = ld2420MeasureZone(reading.hasTarget, reading.distanceM);
  if (measured != gCandidateZone) {
    gCandidateZone = measured;
    gZoneCandidateSince = now;
  } else if (gStableZone != gCandidateZone && (now - gZoneCandidateSince) >= KITCHEN_SENSOR_CONFIRM_MS) {
    gStableZone = gCandidateZone;
  }

  // 1) Вход только в дальнюю зону из OFF -> ничего не делаем
  if (gStableZone == KitchenZone::FAR && gLightingState == KitchenLightingState::OFF) {
    gOffDelayStartedAt = 0;
  }

  // 2) FAR -> NEAR при выключенной ленте: эффект, затем белый рабочий свет
  if (gStableZone == KitchenZone::NEAR) {
    gOffDelayStartedAt = 0;
    if (!gNearCycleActive && gLightingState == KitchenLightingState::OFF) {
      gNearCycleActive = true;
      ld2420SetState(KitchenLightingState::NEAR_EFFECT, now);
    }

    if (gLightingState == KitchenLightingState::NEAR_EFFECT &&
        (now - gStateStartedAt) >= max<uint32_t>(1UL, KITCHEN_NEAR_ENTRY_EFFECT_MS)) {
      ld2420SetState(KitchenLightingState::NEAR_WHITE, now);
    }

    if (gLightingState == KitchenLightingState::FAR_RGB) {
      ld2420SetState(KitchenLightingState::NEAR_WHITE, now);
    }
  }

  // 3) Выход из ближней зоны в дальнюю -> RGB до тех пор, пока есть цель в дальней зоне
  if (gStableZone == KitchenZone::FAR) {
    gOffDelayStartedAt = 0;
    if (gNearCycleActive && (gLightingState == KitchenLightingState::NEAR_WHITE || gLightingState == KitchenLightingState::NEAR_EFFECT)) {
      if (KITCHEN_TRANSITION_WAIT_MS == 0 || (now - gStateStartedAt) >= KITCHEN_TRANSITION_WAIT_MS) {
        ld2420SetState(KitchenLightingState::FAR_RGB, now);
      }
    }
  }

  // 4) Полный выход из дальней зоны -> через delay полное выключение
  if (gStableZone == KitchenZone::NONE && gNearCycleActive) {
    if (gOffDelayStartedAt == 0) { gOffDelayStartedAt = now; }
    if ((now - gOffDelayStartedAt) >= KITCHEN_LIGHTS_OFF_DELAY_MS) {
      gNearCycleActive = false;
      ld2420SetState(KitchenLightingState::OFF, now);
    }
  }

  switch (gLightingState) {
    case KitchenLightingState::NEAR_EFFECT:
      Pow_WS2815 = false;
      ld2420RenderNearEntryEffect(now);
      break;
    case KitchenLightingState::NEAR_WHITE:
      Pow_WS2815 = false;
      ld2420RenderWorkWhite();
      break;
    case KitchenLightingState::FAR_RGB:
      Pow_WS2815 = true;
      loop_WS2815();
      break;
    case KitchenLightingState::OFF:
    default:
      Pow_WS2815 = false;
      ld2420RenderOff();
      break;
  }
}