#pragma once

#include <Arduino.h>
#include <cstring>

// ------------------------------ НАСТРОЙКИ ПОДКЛЮЧЕНИЯ HLK-LD2410C ------------------------------
inline int HLK_LD2410C_RX_PIN = 18; // Отдельный RX-пин ESP32-S для датчика HLK-LD2410C.
inline int HLK_LD2410C_TX_PIN = 15; // Отдельный TX-пин ESP32-S для датчика HLK-LD2410C.
inline uint8_t HLK_LD2410C_UART_PORT = 2; // Отдельный UART-порт ESP32-S (0..2), чтобы датчик не конфликтовал с LD2420.
inline uint32_t HLK_LD2410C_BAUD = 256000UL; // Штатная скорость HLK-LD2410C.

// ------------------------------ НАСТРОЙКИ ОБРАБОТКИ ДАННЫХ ------------------------------
inline String HLK_LD2410C_PARSE_MODE = "auto"; // auto|binary|text
inline bool HLK_LD2410C_DEBUG_RAW_UART = false; // Выводить сырые байты в Serial.
inline float HLK_LD2410C_FILTER_ALPHA = 0.35f; // EMA-фильтр расстояния.
inline float HLK_LD2410C_DISTANCE_JUMP_REJECT_M = 0.90f; // Ограничение скачков.

// ------------------------------ ПЕРЕМЕННЫЕ ДЛЯ WEB/UI ------------------------------
inline float HLK_LD2410C_DISTANCE_GRAPH_M = 0.0f;
inline bool HLK_LD2410C_HAS_TARGET = false;
inline float HLK_LD2410C_DISTANCE_M = 0.0f;
inline float HLK_LD2410C_DISTANCE_CM = 0.0f;
inline int HLK_LD2410C_MOVING_DISTANCE_CM = 0;
inline int HLK_LD2410C_STILL_DISTANCE_CM = 0;
inline int HLK_LD2410C_DETECT_DISTANCE_CM = 0;
inline int HLK_LD2410C_TARGET_STATE = 0;
inline int HLK_LD2410C_VALID_FRAMES = 0;
inline int HLK_LD2410C_INVALID_FRAMES = 0;
inline int HLK_LD2410C_LAST_FRAME_AT_MS = 0;
inline int HLK_LD2410C_LAST_DISTANCE_AGE_MS = 0;
inline String HLK_LD2410C_LAST_LINE = "";
inline String HLK_LD2410C_LINK_STATUS = "Нет данных";

// ------------------------------ УПРАВЛЕНИЕ ДАТЧИКОМ (КОМАНДЫ ИЗ WEB) ------------------------------
inline bool HLK_LD2410C_CMD_READ_VERSION = false;
inline bool HLK_LD2410C_CMD_REBOOT = false;
inline bool HLK_LD2410C_CMD_FACTORY_RESET = false;
inline bool HLK_LD2410C_CMD_ENABLE_CONFIG = false;
inline bool HLK_LD2410C_CMD_DISABLE_CONFIG = false;
inline bool HLK_LD2410C_CMD_REQUEST_PARAMS = false;

struct HlkLd2410cReading {
  bool frameValid = false;
  bool hasTarget = false;
  float distanceM = 0.0f;
  uint8_t targetState = 0;
  uint16_t movingCm = 0;
  uint16_t stillCm = 0;
  uint16_t detectCm = 0;
};

static HardwareSerial *gHlkLd2410cSerial = nullptr;
static uint8_t gHlkLd2410cRxBuffer[256] = {0};
static size_t gHlkLd2410cRxLen = 0;
static String gHlkLd2410cTextLineBuffer;
static float gHlkLd2410cLastDistanceM = 0.0f;
static uint32_t gHlkLd2410cLastRxByteAt = 0;
static int gHlkLd2410cAppliedRxPin = -1;
static int gHlkLd2410cAppliedTxPin = -1;
static uint8_t gHlkLd2410cAppliedUartPort = 255;
static uint32_t gHlkLd2410cAppliedBaud = 0;
inline int HLK_LD2410C_MAX_FRAMES_PER_LOOP = 6; // Сколько валидных кадров максимум обрабатываем за один проход loop для ускорения опроса.

static inline void hlkLd2410cWriteFrame(const uint8_t *data, size_t len) {
  if (!gHlkLd2410cSerial || !data || len == 0) { return; }
  gHlkLd2410cSerial->write(data, len);
  gHlkLd2410cSerial->flush();
}

static inline void hlkLd2410cSendEnableConfig() {
  static const uint8_t kEnableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x02, 0x00, 0x04, 0x03, 0x02, 0x01};
  hlkLd2410cWriteFrame(kEnableCfg, sizeof(kEnableCfg));
}

static inline void hlkLd2410cSendDisableConfig() {
  static const uint8_t kDisableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01};
  hlkLd2410cWriteFrame(kDisableCfg, sizeof(kDisableCfg));
}

static inline void hlkLd2410cSendReadVersion() {
  static const uint8_t kReadVersion[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA0, 0x00, 0x04, 0x03, 0x02, 0x01};
  hlkLd2410cSendEnableConfig();
  delay(30);
  hlkLd2410cWriteFrame(kReadVersion, sizeof(kReadVersion));
  delay(30);
  hlkLd2410cSendDisableConfig();
}

static inline void hlkLd2410cSendReboot() {
  static const uint8_t kReboot[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA3, 0x00, 0x04, 0x03, 0x02, 0x01};
  hlkLd2410cSendEnableConfig();
  delay(30);
  hlkLd2410cWriteFrame(kReboot, sizeof(kReboot));
}

static inline void hlkLd2410cSendFactoryReset() {
  static const uint8_t kFactoryReset[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA2, 0x00, 0x04, 0x03, 0x02, 0x01};
  hlkLd2410cSendEnableConfig();
  delay(30);
  hlkLd2410cWriteFrame(kFactoryReset, sizeof(kFactoryReset));
}

static inline void hlkLd2410cSendRequestParams() {
  static const uint8_t kReadParams[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x61, 0x00, 0x04, 0x03, 0x02, 0x01};
  hlkLd2410cSendEnableConfig();
  delay(30);
  hlkLd2410cWriteFrame(kReadParams, sizeof(kReadParams));
  delay(30);
  hlkLd2410cSendDisableConfig();
}

static inline void hlkLd2410cBeginSerial() {
  if (!gHlkLd2410cSerial) { return; }
  gHlkLd2410cSerial->end();
  gHlkLd2410cSerial->begin(HLK_LD2410C_BAUD, SERIAL_8N1, HLK_LD2410C_RX_PIN, HLK_LD2410C_TX_PIN);
  gHlkLd2410cAppliedRxPin = HLK_LD2410C_RX_PIN;
  gHlkLd2410cAppliedTxPin = HLK_LD2410C_TX_PIN;
  gHlkLd2410cAppliedUartPort = HLK_LD2410C_UART_PORT;
  gHlkLd2410cAppliedBaud = HLK_LD2410C_BAUD;
}

static inline float hlkLd2410cFilterDistance(float rawDistanceM) {
  if (rawDistanceM <= 0.01f || rawDistanceM >= 12.0f) { return gHlkLd2410cLastDistanceM; }
  const float jumpLimit = constrain(HLK_LD2410C_DISTANCE_JUMP_REJECT_M, 0.10f, 3.00f);
  float candidate = rawDistanceM;
  if (gHlkLd2410cLastDistanceM > 0.01f && fabsf(candidate - gHlkLd2410cLastDistanceM) > jumpLimit) {
    candidate = gHlkLd2410cLastDistanceM + (candidate > gHlkLd2410cLastDistanceM ? jumpLimit : -jumpLimit);
  }
  const float alpha = constrain(HLK_LD2410C_FILTER_ALPHA, 0.05f, 0.95f);
  if (gHlkLd2410cLastDistanceM <= 0.01f) { return candidate; }
  return gHlkLd2410cLastDistanceM + (candidate - gHlkLd2410cLastDistanceM) * alpha;
}

static inline bool hlkLd2410cExtractDistance(const String &line, float &distanceM) {
  if (!line.length()) { return false; }
  int start = -1;
  int end = -1;
  for (int i = 0; i < line.length(); ++i) {
    const char c = line.charAt(i);
    if ((c >= '0' && c <= '9') || c == '.' || c == ',') { start = i; break; }
  }
  if (start < 0) { return false; }
  for (int i = start; i < line.length(); ++i) {
    const char c = line.charAt(i);
    if (!((c >= '0' && c <= '9') || c == '.' || c == ',')) { end = i; break; }
  }
  if (end < 0) { end = line.length(); }

  String numeric = line.substring(start, end);
  numeric.replace(',', '.');
  const float raw = numeric.toFloat();
  if (raw <= 0.0f) { return false; }

  String lower = line;
  lower.toLowerCase();
  if (lower.indexOf("mm") >= 0) { distanceM = raw / 1000.0f; }
  else if (lower.indexOf("cm") >= 0) { distanceM = raw / 100.0f; }
  else if (lower.indexOf("m") >= 0) { distanceM = raw; }
  else { distanceM = (raw > 12.0f) ? (raw / 100.0f) : raw; }
  return distanceM > 0.01f && distanceM < 12.0f;
}

static inline HlkLd2410cReading hlkLd2410cReadFrame() {
  HlkLd2410cReading reading;
  if (!gHlkLd2410cSerial) { return reading; }

  String mode = HLK_LD2410C_PARSE_MODE;
  mode.trim();
  mode.toLowerCase();
  const bool textMode = (mode == "text" || mode == "auto");
  const bool binaryMode = (mode == "binary" || mode == "auto");

  while (gHlkLd2410cSerial->available() > 0) {
    const int raw = gHlkLd2410cSerial->read();
    if (raw < 0) { break; }
    const uint8_t b = static_cast<uint8_t>(raw);
    gHlkLd2410cLastRxByteAt = millis();
    if (HLK_LD2410C_DEBUG_RAW_UART) { Serial.write(b); }

    if (gHlkLd2410cRxLen >= sizeof(gHlkLd2410cRxBuffer)) {
      memmove(gHlkLd2410cRxBuffer, gHlkLd2410cRxBuffer + 1, sizeof(gHlkLd2410cRxBuffer) - 1);
      gHlkLd2410cRxLen = sizeof(gHlkLd2410cRxBuffer) - 1;
    }
    gHlkLd2410cRxBuffer[gHlkLd2410cRxLen++] = b;

    if (!textMode) { continue; }
    const char c = static_cast<char>(b);
    if (c == '\r') { continue; }
    if (c == '\n') {
      String line = gHlkLd2410cTextLineBuffer;
      gHlkLd2410cTextLineBuffer = "";
      line.trim();
      if (!line.isEmpty()) { HLK_LD2410C_LAST_LINE = line; }

      float d = 0.0f;
      if (hlkLd2410cExtractDistance(line, d)) {
        reading.frameValid = true;
        reading.distanceM = d;
        reading.hasTarget = true;
      }

      String lower = line;
      lower.toLowerCase();
      if (lower.indexOf("off") >= 0) { reading.frameValid = true; reading.hasTarget = false; }
      if (lower.indexOf("on") >= 0) { reading.frameValid = true; reading.hasTarget = true; }
      continue;
    }
    if (gHlkLd2410cTextLineBuffer.length() < 120) { gHlkLd2410cTextLineBuffer += c; }
    else { gHlkLd2410cTextLineBuffer = ""; }
  }

  if (!binaryMode) { return reading; }

  while (gHlkLd2410cRxLen >= 10) {
    size_t start = 0;
    while (start + 3 < gHlkLd2410cRxLen) {
      if (gHlkLd2410cRxBuffer[start] == 0xF4 && gHlkLd2410cRxBuffer[start + 1] == 0xF3 &&
          gHlkLd2410cRxBuffer[start + 2] == 0xF2 && gHlkLd2410cRxBuffer[start + 3] == 0xF1) { break; }
      start++;
    }
    if (start > 0) {
      memmove(gHlkLd2410cRxBuffer, gHlkLd2410cRxBuffer + start, gHlkLd2410cRxLen - start);
      gHlkLd2410cRxLen -= start;
    }
    if (gHlkLd2410cRxLen < 6) { break; }

    const uint16_t frameDataLen = static_cast<uint16_t>(gHlkLd2410cRxBuffer[4]) |
                                  (static_cast<uint16_t>(gHlkLd2410cRxBuffer[5]) << 8);
    if (frameDataLen == 0 || frameDataLen > 180) {
      memmove(gHlkLd2410cRxBuffer, gHlkLd2410cRxBuffer + 1, gHlkLd2410cRxLen - 1);
      gHlkLd2410cRxLen -= 1;
      HLK_LD2410C_INVALID_FRAMES++;
      continue;
    }

    const size_t frameLen = static_cast<size_t>(frameDataLen) + 10;
    if (gHlkLd2410cRxLen < frameLen) { break; }

    const size_t footerPos = 6 + frameDataLen;
    const bool footerOk = gHlkLd2410cRxBuffer[footerPos] == 0xF8 && gHlkLd2410cRxBuffer[footerPos + 1] == 0xF7 &&
                          gHlkLd2410cRxBuffer[footerPos + 2] == 0xF6 && gHlkLd2410cRxBuffer[footerPos + 3] == 0xF5;
    if (!footerOk) {
      memmove(gHlkLd2410cRxBuffer, gHlkLd2410cRxBuffer + 1, gHlkLd2410cRxLen - 1);
      gHlkLd2410cRxLen -= 1;
      HLK_LD2410C_INVALID_FRAMES++;
      continue;
    }

    const uint8_t *frameData = gHlkLd2410cRxBuffer + 6;
    if (frameDataLen >= 13 && frameData[0] == 0x02 && frameData[1] == 0xAA &&
        frameData[frameDataLen - 2] == 0x55 && frameData[frameDataLen - 1] == 0x00) {
      reading.frameValid = true;
      reading.targetState = frameData[2];
      reading.movingCm = static_cast<uint16_t>(frameData[3]) | (static_cast<uint16_t>(frameData[4]) << 8);
      reading.stillCm = static_cast<uint16_t>(frameData[6]) | (static_cast<uint16_t>(frameData[7]) << 8);
      reading.detectCm = static_cast<uint16_t>(frameData[9]) | (static_cast<uint16_t>(frameData[10]) << 8);

      uint16_t chosenCm = 0;
      if (reading.movingCm > 0 && reading.stillCm > 0) { chosenCm = min(reading.movingCm, reading.stillCm); }
      else if (reading.movingCm > 0) { chosenCm = reading.movingCm; }
      else if (reading.stillCm > 0) { chosenCm = reading.stillCm; }
      else { chosenCm = reading.detectCm; }

      reading.hasTarget = (reading.targetState != 0x00) && (chosenCm > 0);
      if (chosenCm > 0) { reading.distanceM = static_cast<float>(chosenCm) / 100.0f; }
    }

    memmove(gHlkLd2410cRxBuffer, gHlkLd2410cRxBuffer + frameLen, gHlkLd2410cRxLen - frameLen);
    gHlkLd2410cRxLen -= frameLen;
    if (reading.frameValid) { break; }
  }

  return reading;
}

static inline void hlkLd2410cApplyRuntimeUartConfigIfChanged() {
  const bool portChanged = (gHlkLd2410cAppliedUartPort != HLK_LD2410C_UART_PORT);
  const bool pinsChanged = (gHlkLd2410cAppliedRxPin != HLK_LD2410C_RX_PIN) || (gHlkLd2410cAppliedTxPin != HLK_LD2410C_TX_PIN);
  const bool baudChanged = (gHlkLd2410cAppliedBaud != HLK_LD2410C_BAUD);
  if (!portChanged && !pinsChanged && !baudChanged) { return; }
  if (portChanged) {
    if (gHlkLd2410cSerial) { gHlkLd2410cSerial->end(); delete gHlkLd2410cSerial; }
    gHlkLd2410cSerial = new HardwareSerial(HLK_LD2410C_UART_PORT);
  }
  hlkLd2410cBeginSerial();
  gHlkLd2410cRxLen = 0;
  gHlkLd2410cTextLineBuffer = "";
}

static inline void hlkLd2410cHandleUserCommands() {
  if (HLK_LD2410C_CMD_ENABLE_CONFIG) { hlkLd2410cSendEnableConfig(); HLK_LD2410C_CMD_ENABLE_CONFIG = false; }
  if (HLK_LD2410C_CMD_DISABLE_CONFIG) { hlkLd2410cSendDisableConfig(); HLK_LD2410C_CMD_DISABLE_CONFIG = false; }
  if (HLK_LD2410C_CMD_REQUEST_PARAMS) { hlkLd2410cSendRequestParams(); HLK_LD2410C_CMD_REQUEST_PARAMS = false; }
  if (HLK_LD2410C_CMD_READ_VERSION) { hlkLd2410cSendReadVersion(); HLK_LD2410C_CMD_READ_VERSION = false; }
  if (HLK_LD2410C_CMD_REBOOT) { hlkLd2410cSendReboot(); HLK_LD2410C_CMD_REBOOT = false; HLK_LD2410C_LAST_FRAME_AT_MS = 0; }
  if (HLK_LD2410C_CMD_FACTORY_RESET) { hlkLd2410cSendFactoryReset(); HLK_LD2410C_CMD_FACTORY_RESET = false; HLK_LD2410C_LAST_FRAME_AT_MS = 0; }
}

static inline void setup_HLK_LD2410C() {
  if (gHlkLd2410cSerial) { gHlkLd2410cSerial->end(); delete gHlkLd2410cSerial; }
  gHlkLd2410cSerial = new HardwareSerial(HLK_LD2410C_UART_PORT);
  hlkLd2410cBeginSerial();

  HLK_LD2410C_LINK_STATUS = "Ожидание данных";
  HLK_LD2410C_VALID_FRAMES = 0;
  HLK_LD2410C_INVALID_FRAMES = 0;
  HLK_LD2410C_LAST_FRAME_AT_MS = 0;
  HLK_LD2410C_LAST_DISTANCE_AGE_MS = 0;
  HLK_LD2410C_LAST_LINE = "";
  HLK_LD2410C_HAS_TARGET = false;
  HLK_LD2410C_DISTANCE_M = 0.0f;
  HLK_LD2410C_DISTANCE_CM = 0.0f;
  HLK_LD2410C_DISTANCE_GRAPH_M = 0.0f;
  HLK_LD2410C_MOVING_DISTANCE_CM = 0;
  HLK_LD2410C_STILL_DISTANCE_CM = 0;
  HLK_LD2410C_DETECT_DISTANCE_CM = 0;
  HLK_LD2410C_TARGET_STATE = 0;
  gHlkLd2410cLastDistanceM = 0.0f;
  gHlkLd2410cRxLen = 0;
  gHlkLd2410cTextLineBuffer = "";
  gHlkLd2410cLastRxByteAt = millis();
}

static inline void loop_HLK_LD2410C() {
  const uint32_t now = millis();
  hlkLd2410cApplyRuntimeUartConfigIfChanged();
  hlkLd2410cHandleUserCommands();

  const int maxFrames = constrain(HLK_LD2410C_MAX_FRAMES_PER_LOOP, 1, 20); // Ограничиваем верхний предел, чтобы не зациклить loop на шумном UART.
  for (int i = 0; i < maxFrames; ++i) { // Обрабатываем несколько кадров за один проход для ускорения актуализации значений.
    HlkLd2410cReading reading = hlkLd2410cReadFrame();
    if (!reading.frameValid) { break; } // Как только новых кадров нет, выходим из цикла.
    HLK_LD2410C_VALID_FRAMES++;
    HLK_LD2410C_LAST_FRAME_AT_MS = static_cast<int>(now);
    HLK_LD2410C_LINK_STATUS = "Связь OK";
    HLK_LD2410C_HAS_TARGET = reading.hasTarget;
    HLK_LD2410C_TARGET_STATE = static_cast<int>(reading.targetState);
    HLK_LD2410C_MOVING_DISTANCE_CM = static_cast<int>(reading.movingCm);
    HLK_LD2410C_STILL_DISTANCE_CM = static_cast<int>(reading.stillCm);
    HLK_LD2410C_DETECT_DISTANCE_CM = static_cast<int>(reading.detectCm);
    if (reading.distanceM > 0.01f && reading.distanceM < 12.0f) {
      gHlkLd2410cLastDistanceM = hlkLd2410cFilterDistance(reading.distanceM);
    }
  }

  if (HLK_LD2410C_LAST_FRAME_AT_MS > 0) {
    HLK_LD2410C_LAST_DISTANCE_AGE_MS = static_cast<int>(now - static_cast<uint32_t>(HLK_LD2410C_LAST_FRAME_AT_MS));
  } else {
    HLK_LD2410C_LAST_DISTANCE_AGE_MS = 0;
  }

  if (HLK_LD2410C_LAST_FRAME_AT_MS > 0 && (now - static_cast<uint32_t>(HLK_LD2410C_LAST_FRAME_AT_MS)) > 3000UL) {
    HLK_LD2410C_LINK_STATUS = "Нет новых кадров";
  }
  if ((now - gHlkLd2410cLastRxByteAt) > 5000UL) {
    HLK_LD2410C_LINK_STATUS = "UART тишина";
  }

  HLK_LD2410C_DISTANCE_M = gHlkLd2410cLastDistanceM;
  HLK_LD2410C_DISTANCE_CM = HLK_LD2410C_DISTANCE_M * 100.0f;
  HLK_LD2410C_DISTANCE_GRAPH_M = HLK_LD2410C_DISTANCE_M;
}