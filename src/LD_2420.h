#pragma once // Один раз подключаем заголовок, чтобы избежать повторных определений.

#include <Arduino.h> // Подключаем базовые типы Arduino (String, millis, HardwareSerial и т.д.).
#include <cstring> // Подключаем функции работы с памятью (memmove, memset).

// ------------------------------ НАСТРОЙКИ ПОДКЛЮЧЕНИЯ LD2420 ------------------------------
inline int LD2420_RX_PIN = 16; // Номер RX-пина ESP32, куда приходит TX/OT2 с датчика LD2420.
inline int LD2420_TX_PIN = 17; // Номер TX-пина ESP32, который подключается к RX датчика LD2420.
inline uint8_t LD2420_UART_PORT = 2; // Номер аппаратного UART ESP32 (обычно 1 или 2 для внешнего датчика).
inline constexpr uint32_t LD2420_BAUD_FIXED = 115200UL; // Фиксированная скорость UART для LD2420 (из UI не изменяется).
inline bool LD2420_AUTO_ENERGY_RECOVERY = true; // Автоматически отправлять команду ENERGY MODE, если после перезапуска датчик молчит.
inline uint32_t LD2420_RECOVERY_SILENCE_MS = 1800UL; // Через сколько мс тишины считать, что датчик «завис» и нужно восстановление.
inline uint32_t LD2420_RECOVERY_RETRY_MS = 4500UL; // Минимальный интервал между повторными recovery-командами ENERGY MODE.

// ------------------------------ НАСТРОЙКИ ОБРАБОТКИ ДАННЫХ ------------------------------
inline String LD2420_PARSE_MODE = "auto"; // Режим парсинга: "auto" (binary+text), "binary" (только бинарные), "text" (только текст).
inline bool LD2420_DEBUG_RAW_UART = false; // Флаг вывода сырых байтов LD2420 в Serial для диагностики.
inline bool LD2420_FORCE_SIMPLE_MODE = false; // Флаг принудительного перевода датчика в simple mode при старте.
inline float LD2420_FILTER_ALPHA = 0.30f; // Коэффициент сглаживания EMA для стабилизации расстояния (0.05..0.95).
inline float LD2420_DISTANCE_JUMP_REJECT_M = 0.80f; // Порог резкого скачка (м), после которого измерение подавляется как выброс.

// ------------------------------ ПЕРЕМЕННЫЕ ДЛЯ WEB/UI И ГРАФИКА ------------------------------
inline float LD2420_DISTANCE_GRAPH_M = 0.0f; // Текущее расстояние в метрах для источника веб-графика.
inline bool LD2420_HAS_TARGET = false; // Текущее состояние обнаружения цели датчиком.
inline float LD2420_DISTANCE_M = 0.0f; // Расстояние до цели в метрах.
inline float LD2420_DISTANCE_CM = 0.0f; // Расстояние до цели в сантиметрах.
inline float LD2420_DISTANCE_MM = 0.0f; // Расстояние до цели в миллиметрах.
inline int LD2420_SIGNAL_VALUE = 0; // Сырый уровень сигнала/энергии из кадра датчика (если доступен).
inline int LD2420_PRESENCE_BYTE = 0; // Сырой байт presence из бинарного кадра LD2420.
inline int LD2420_VALID_FRAMES = 0; // Счетчик валидных кадров LD2420 (тип int нужен для UI_DISPLAY_INT).
inline int LD2420_INVALID_FRAMES = 0; // Счетчик невалидных/поврежденных кадров LD2420 (тип int нужен для UI_DISPLAY_INT).
inline int LD2420_LAST_FRAME_AT_MS = 0; // Время (millis) получения последнего валидного кадра в int для UI_DISPLAY_INT.
inline int LD2420_LAST_DISTANCE_AGE_MS = 0; // Возраст последнего валидного расстояния в миллисекундах в int для UI_DISPLAY_INT.
inline String LD2420_LAST_LINE = ""; // Последняя принятая текстовая строка (для диагностики).
inline String LD2420_LINK_STATUS = "Нет данных"; // Человекочитаемый статус связи с датчиком.
inline String LD2420_ACTIVE_BAUD_TEXT = "115200 (fixed)"; // Текст текущей рабочей скорости UART.

// ------------------------------ УПРАВЛЕНИЕ LD2420 (КОМАНДЫ ИЗ WEB) ------------------------------
inline bool LD2420_CMD_READ_VERSION = false; // Флаг команды запроса версии прошивки датчика.
inline bool LD2420_CMD_REBOOT = false; // Флаг команды перезагрузки датчика.
inline bool LD2420_CMD_FACTORY_RESET = false; // Флаг команды сброса датчика к заводским параметрам.
inline bool LD2420_CMD_ENABLE_CONFIG = false; // Флаг команды входа в конфигурационный режим.
inline bool LD2420_CMD_DISABLE_CONFIG = false; // Флаг команды выхода из конфигурационного режима.
inline bool LD2420_CMD_SET_SIMPLE_MODE = false; // Флаг команды переключения датчика в simple mode.
inline bool LD2420_CMD_SET_ENERGY_MODE = false; // Флаг команды переключения датчика в energy mode.

// ------------------------------ ВНУТРЕННИЕ ПЕРЕМЕННЫЕ ДРАЙВЕРА ------------------------------
struct LD2420Reading { // Структура одного декодированного измерения LD2420.
  bool frameValid = false; // Признак, что получен валидный кадр.
  bool hasTarget = false; // Признак обнаружения цели.
  float distanceM = 0.0f; // Измеренное расстояние в метрах.
  int signal = 0; // Измеренный уровень сигнала/энергии.
  uint8_t presence = 0; // Сырой байт presence из кадра.
};

static HardwareSerial *gLd2420Serial = nullptr; // Указатель на объект аппаратного UART, выделяемый динамически.
static uint8_t gRxBuffer[256] = {0}; // Кольцевой буфер сырых байтов для бинарного парсера.
static size_t gRxLen = 0; // Текущее число байтов, накопленных в буфере gRxBuffer.
static String gTextLineBuffer; // Буфер накопления одной текстовой строки до символа новой строки.
static float gLastDistanceM = 0.0f; // Последнее корректное расстояние для удержания значения между кадрами.
static uint32_t gLastRxByteAt = 0; // Время прихода последнего байта UART.
static uint32_t gCurrentBaud = LD2420_BAUD_FIXED; // Текущая активная скорость UART.
static int gAppliedRxPin = -1; // Запоминаем последний примененный RX-пин для отслеживания изменений из UI.
static int gAppliedTxPin = -1; // Запоминаем последний примененный TX-пин для отслеживания изменений из UI.
static uint8_t gAppliedUartPort = 255; // Запоминаем последний примененный UART-порт для отслеживания изменений из UI.
static uint32_t gAppliedBaud = 0; // Запоминаем последний примененный baud для отслеживания изменений из UI.
static uint32_t gDriverStartedAt = 0; // Время старта драйвера для логики восстановления после перезагрузки датчика/ESP32.
static uint32_t gLastRecoveryAttemptAt = 0; // Время последней автоматической попытки вернуть датчик в ENERGY MODE.
static bool gRecoveryEnergyForced = false; // Флаг, что хотя бы одна автоматическая команда ENERGY MODE уже отправлялась.
static float gDistanceWindow[3] = {0.0f, 0.0f, 0.0f}; // Окно последних трех измерений для медианной фильтрации.
static uint8_t gDistanceWindowCount = 0; // Счетчик валидных значений в окне для медианы.
static uint8_t gDistanceWindowIndex = 0; // Текущая позиция кольцевой записи в окне медианы.

static inline void ld2420WriteFrame(const uint8_t *data, size_t len) { // Отправляем бинарную команду датчику.
  if (!gLd2420Serial || !data || len == 0) { return; } // Прерываем отправку, если UART не готов или данные пустые.
  gLd2420Serial->write(data, len); // Пишем все байты команды в UART.
  gLd2420Serial->flush(); // Ждем завершения физической передачи команды.
} // Завершаем функцию отправки бинарного кадра.

static inline void ld2420SendEnableConfig() { // Отправляем команду включения конфигурационного режима.
  static const uint8_t kEnableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x02, 0x00, 0x04, 0x03, 0x02, 0x01}; // Бинарный кадр Enable Config.
  ld2420WriteFrame(kEnableCfg, sizeof(kEnableCfg)); // Передаем команду в датчик.
} // Завершаем функцию включения config mode.

static inline void ld2420SendDisableConfig() { // Отправляем команду отключения конфигурационного режима.
  static const uint8_t kDisableCfg[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xFE, 0x00, 0x04, 0x03, 0x02, 0x01}; // Бинарный кадр Disable Config.
  ld2420WriteFrame(kDisableCfg, sizeof(kDisableCfg)); // Передаем команду в датчик.
} // Завершаем функцию отключения config mode.

static inline void ld2420SendSetSimpleMode() { // Отправляем команду переключения датчика в simple mode.
  static const uint8_t kSetSimpleMode[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01}; // Кадр Set Simple Mode.
  ld2420SendEnableConfig(); // Сначала включаем конфигурационный режим.
  delay(30); // Даём датчику время перейти в config mode.
  ld2420WriteFrame(kSetSimpleMode, sizeof(kSetSimpleMode)); // Отправляем команду установки режима.
  delay(30); // Небольшая пауза для применения параметров.
  ld2420SendDisableConfig(); // Завершаем конфигурирование и возвращаем рабочий режим.
} // Завершаем функцию установки simple mode.

static inline void ld2420SendSetEnergyMode() { // Отправляем команду переключения датчика в energy mode.
  static const uint8_t kSetEnergyMode[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x08, 0x00, 0x12, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01}; // Кадр Set Energy Mode.
  ld2420SendEnableConfig(); // Сначала включаем конфигурационный режим.
  delay(30); // Даём датчику время перейти в config mode.
  ld2420WriteFrame(kSetEnergyMode, sizeof(kSetEnergyMode)); // Отправляем команду установки режима.
  delay(30); // Небольшая пауза для применения параметров.
  ld2420SendDisableConfig(); // Завершаем конфигурирование и возвращаем рабочий режим.
} // Завершаем функцию установки energy mode.

static inline float ld2420Median3(float a, float b, float c) { // Возвращаем медиану трех значений для подавления одиночных выбросов.
  if (a > b) { const float t = a; a = b; b = t; } // Гарантируем, что a <= b после первой перестановки.
  if (b > c) { const float t = b; b = c; c = t; } // Гарантируем, что b <= c после второй перестановки.
  if (a > b) { const float t = a; a = b; b = t; } // Финальная перестановка обеспечивает корректную медиану в b.
  return b; // Возвращаем среднее по порядку значение как медиану.
} // Завершаем функцию вычисления медианы.

static inline float ld2420FilterDistance(float rawDistanceM) { // Стабилизируем расстояние фильтрами медианы и EMA.
  if (rawDistanceM <= 0.01f || rawDistanceM >= 15.0f) { return gLastDistanceM; } // Некорректный raw сразу игнорируем и оставляем последнее надежное значение.
  if (gLastDistanceM > 0.05f && gLastDistanceM <= 0.45f && rawDistanceM >= 1.50f) { return gLastDistanceM; } // Если объект уже близко, режем ложные "улеты" в даль (типичный глюк 7-10 м на малой дистанции).
  gDistanceWindow[gDistanceWindowIndex] = rawDistanceM; // Пишем новый raw в кольцевое окно.
  gDistanceWindowIndex = static_cast<uint8_t>((gDistanceWindowIndex + 1U) % 3U); // Переходим к следующей позиции кольцевого окна.
  if (gDistanceWindowCount < 3U) { gDistanceWindowCount++; } // Увеличиваем заполненность окна, пока не достигнем трех точек.
  float candidate = rawDistanceM; // По умолчанию берем raw как кандидат для дальнейшей фильтрации.
  if (gDistanceWindowCount == 3U) { candidate = ld2420Median3(gDistanceWindow[0], gDistanceWindow[1], gDistanceWindow[2]); } // Когда окно заполнено, берем медиану трех измерений.
  const float jumpLimit = constrain(LD2420_DISTANCE_JUMP_REJECT_M, 0.10f, 3.00f); // Ограничиваем допустимый порог подавления резких скачков.
  if (gLastDistanceM > 0.01f && fabsf(candidate - gLastDistanceM) > jumpLimit) { candidate = gLastDistanceM + (candidate > gLastDistanceM ? jumpLimit : -jumpLimit); } // Ограничиваем слишком резкие скачки ступенью jumpLimit.
  const float alpha = constrain(LD2420_FILTER_ALPHA, 0.05f, 0.95f); // Ограничиваем коэффициент EMA безопасным диапазоном.
  if (gLastDistanceM <= 0.01f) { return candidate; } // Если это первое значение, возвращаем его без EMA-инерции.
  return gLastDistanceM + (candidate - gLastDistanceM) * alpha; // Применяем экспоненциальное сглаживание для стабилизации графика.
} // Завершаем функцию фильтрации расстояния.



static inline void ld2420SendReadVersion() { // Отправляем команду запроса версии прошивки датчика.
  static const uint8_t kReadVersion[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA0, 0x00, 0x04, 0x03, 0x02, 0x01}; // Кадр Read Version.
  ld2420SendEnableConfig(); // Переходим в конфигурационный режим.
  delay(30); // Даем паузу на вход в config mode.
  ld2420WriteFrame(kReadVersion, sizeof(kReadVersion)); // Отправляем запрос версии прошивки.
  delay(30); // Пауза для ответа датчика.
  ld2420SendDisableConfig(); // Выходим из конфигурационного режима.
} // Завершаем функцию запроса версии.

static inline void ld2420SendReboot() { // Отправляем команду программной перезагрузки датчика.
  static const uint8_t kReboot[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA3, 0x00, 0x04, 0x03, 0x02, 0x01}; // Кадр Reboot.
  ld2420SendEnableConfig(); // Переходим в конфигурационный режим.
  delay(30); // Даем паузу на вход в config mode.
  ld2420WriteFrame(kReboot, sizeof(kReboot)); // Отправляем команду перезагрузки.
} // Завершаем функцию перезагрузки датчика.

static inline void ld2420SendFactoryReset() { // Отправляем команду сброса датчика к заводским настройкам.
  static const uint8_t kFactoryReset[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0xA2, 0x00, 0x04, 0x03, 0x02, 0x01}; // Кадр Factory Reset.
  ld2420SendEnableConfig(); // Переходим в конфигурационный режим.
  delay(30); // Даем паузу на вход в config mode.
  ld2420WriteFrame(kFactoryReset, sizeof(kFactoryReset)); // Отправляем команду сброса.
} // Завершаем функцию заводского сброса.

static inline void ld2420BeginSerial(uint32_t baud) { // Запускаем или перезапускаем UART с нужной скоростью.
  if (!gLd2420Serial) { return; } // Прерываем операцию, если объект UART не создан.
  gCurrentBaud = baud; // Сохраняем текущую скорость UART.
  gLd2420Serial->end(); // Останавливаем UART перед повторной инициализацией.
  gLd2420Serial->begin(gCurrentBaud, SERIAL_8N1, LD2420_RX_PIN, LD2420_TX_PIN); // Стартуем UART с текущими параметрами.
  gAppliedRxPin = LD2420_RX_PIN; // Запоминаем примененный RX-пин.
  gAppliedTxPin = LD2420_TX_PIN; // Запоминаем примененный TX-пин.
  gAppliedUartPort = LD2420_UART_PORT; // Запоминаем примененный UART-порт.
  gAppliedBaud = gCurrentBaud; // Запоминаем примененную скорость UART.
  LD2420_ACTIVE_BAUD_TEXT = String(gCurrentBaud); // Обновляем строку текущего baud для web-интерфейса.
} // Завершаем функцию запуска UART.

static inline bool ld2420ExtractDistance(const String &line, float &distanceM) { // Извлекаем числовое значение расстояния из текстовой строки.
  if (!line.length()) { return false; } // Возвращаем false, если строка пустая.
  int start = -1; // Индекс начала числовой последовательности.
  int end = -1; // Индекс конца числовой последовательности.
  for (int i = 0; i < line.length(); ++i) { // Проходим по всем символам строки.
    const char c = line.charAt(i); // Читаем текущий символ.
    if ((c >= '0' && c <= '9') || c == '.' || c == ',') { start = i; break; } // Ищем первый символ числа.
  }
  if (start < 0) { return false; } // Возвращаем false, если число не найдено.
  for (int i = start; i < line.length(); ++i) { // Ищем конец числовой последовательности.
    const char c = line.charAt(i); // Читаем текущий символ.
    if (!((c >= '0' && c <= '9') || c == '.' || c == ',')) { end = i; break; } // Фиксируем конец числа на первом неподходящем символе.
  }
  if (end < 0) { end = line.length(); } // Если конец не найден, значит число идет до конца строки.
  String numeric = line.substring(start, end); // Вырезаем подстроку с числом.
  numeric.replace(',', '.'); // Приводим десятичный разделитель к точке.
  const float raw = numeric.toFloat(); // Преобразуем подстроку в float.
  if (raw <= 0.0f) { return false; } // Некорректные или нулевые значения отвергаем.
  String lower = line; // Создаем копию строки для поиска единиц измерения.
  lower.toLowerCase(); // Переводим строку в нижний регистр.
  if (lower.indexOf("mm") >= 0) { distanceM = raw / 1000.0f; } // Переводим миллиметры в метры.
  else if (lower.indexOf("cm") >= 0) { distanceM = raw / 100.0f; } // Переводим сантиметры в метры.
  else if (lower.indexOf("m") >= 0) { distanceM = raw; } // Значение уже в метрах.
  else { distanceM = (raw > 12.0f) ? (raw / 100.0f) : raw; } // Если единицы не указаны, оцениваем по величине.
  return distanceM > 0.01f && distanceM < 15.0f; // Принимаем только реалистичный диапазон расстояний для LD2420.
} // Завершаем функцию извлечения расстояния.

static inline LD2420Reading ld2420ReadFrame() { // Читаем и парсим входящие данные датчика в один структурированный результат.
  LD2420Reading reading; // Создаем результат с начальными значениями.
  if (!gLd2420Serial) { return reading; } // Если UART не создан, возвращаем пустой результат.

  String mode = LD2420_PARSE_MODE; // Берем текущий режим парсинга из переменной конфигурации.
  mode.trim(); // Удаляем лишние пробелы.
  mode.toLowerCase(); // Нормализуем режим в нижний регистр.
  const bool textMode = (mode == "text" || mode == "auto"); // Определяем, нужно ли читать текстовый поток.
  const bool binaryMode = (mode == "binary" || mode == "auto"); // Определяем, нужно ли читать бинарные кадры.

  while (gLd2420Serial->available() > 0) { // Читаем все доступные байты из UART.
    const int raw = gLd2420Serial->read(); // Читаем очередной байт.
    if (raw < 0) { break; } // Защита от ошибки чтения UART.
    const uint8_t b = static_cast<uint8_t>(raw); // Приводим прочитанное значение к типу byte.
    gLastRxByteAt = millis(); // Фиксируем время прихода байта для диагностики линка.
    if (LD2420_DEBUG_RAW_UART) { Serial.write(b); } // При отладке печатаем сырой поток в основной Serial.

    if (gRxLen >= sizeof(gRxBuffer)) { memmove(gRxBuffer, gRxBuffer + 1, sizeof(gRxBuffer) - 1); gRxLen = sizeof(gRxBuffer) - 1; } // Не даем буферу переполниться.
    gRxBuffer[gRxLen++] = b; // Добавляем байт в буфер бинарного парсера.

    if (!textMode) { continue; } // Если текстовый парсер выключен, сразу переходим к следующему байту.
    const char c = static_cast<char>(b); // Преобразуем байт в символ для строкового парсера.
    if (c == '\r') { continue; } // Игнорируем символ возврата каретки.
    if (c == '\n') { // Если пришел конец строки, обрабатываем накопленную строку.
      String line = gTextLineBuffer; // Копируем накопленную строку в локальную переменную.
      gTextLineBuffer = ""; // Сбрасываем буфер строки для следующего сообщения.
      line.trim(); // Удаляем лишние пробелы по краям строки.
      if (!line.isEmpty()) { LD2420_LAST_LINE = line; } // Сохраняем непустую строку в диагностику.
      float d = 0.0f; // Временная переменная для извлеченного расстояния.
      if (ld2420ExtractDistance(line, d)) { // Проверяем, удалось ли извлечь расстояние из текста.
        reading.frameValid = true; // Считаем этот пакет валидным измерением.
        reading.distanceM = d; // Записываем распознанное расстояние.
        reading.hasTarget = true; // Для текстового расстояния считаем, что цель присутствует.
      }
      String lower = line; // Создаем строку для поиска ключевых слов on/off.
      lower.toLowerCase(); // Приводим строку к нижнему регистру.
      if (lower.indexOf("off") >= 0) { reading.frameValid = true; reading.hasTarget = false; } // Обрабатываем текстовое событие пропадания цели.
      if (lower.indexOf("on") >= 0) { reading.frameValid = true; reading.hasTarget = true; } // Обрабатываем текстовое событие появления цели.
      continue; // Переходим к чтению следующего байта UART.
    }
    if (gTextLineBuffer.length() < 120) { gTextLineBuffer += c; } // Накопливаем текущую строку, пока она не слишком длинная.
    else { gTextLineBuffer = ""; } // Сбрасываем слишком длинную строку как шум.
  }

  if (!binaryMode) { return reading; } // Если бинарный парсер выключен, возвращаем результат текстового парсинга.

  while (gRxLen >= 10) { // Пока в буфере достаточно данных хотя бы для минимального бинарного кадра.
    size_t start = 0; // Индекс начала заголовка бинарного кадра.
    while (start + 3 < gRxLen) { // Ищем сигнатуру начала кадра F4 F3 F2 F1.
      if (gRxBuffer[start] == 0xF4 && gRxBuffer[start + 1] == 0xF3 && gRxBuffer[start + 2] == 0xF2 && gRxBuffer[start + 3] == 0xF1) { break; } // Нашли начало кадра.
      start++; // Сдвигаем поиск дальше по буферу.
    }
    if (start > 0) { memmove(gRxBuffer, gRxBuffer + start, gRxLen - start); gRxLen -= start; } // Удаляем мусор до заголовка кадра.
    if (gRxLen < 6) { break; } // Ждем, пока накопится длина для чтения поля size.

    const uint16_t frameDataLen = static_cast<uint16_t>(gRxBuffer[4]) | (static_cast<uint16_t>(gRxBuffer[5]) << 8); // Читаем длину полезной части кадра.
    if (frameDataLen == 0 || frameDataLen > 180) { memmove(gRxBuffer, gRxBuffer + 1, gRxLen - 1); gRxLen -= 1; LD2420_INVALID_FRAMES++; continue; } // Отбрасываем кадр с невозможной длиной.

    const size_t frameLen = static_cast<size_t>(frameDataLen) + 10; // Вычисляем полную длину кадра (заголовок+данные+хвост).
    if (gRxLen < frameLen) { break; } // Ждем, пока придут все байты кадра.

    const size_t footerPos = 6 + frameDataLen; // Находим начало хвоста кадра.
    const bool footerOk = gRxBuffer[footerPos] == 0xF8 && gRxBuffer[footerPos + 1] == 0xF7 && gRxBuffer[footerPos + 2] == 0xF6 && gRxBuffer[footerPos + 3] == 0xF5; // Проверяем хвост кадра.
    if (!footerOk) { memmove(gRxBuffer, gRxBuffer + 1, gRxLen - 1); gRxLen -= 1; LD2420_INVALID_FRAMES++; continue; } // Если хвост поврежден, сдвигаем буфер и продолжаем поиск.

    reading.frameValid = true; // Помечаем, что получили корректный кадр.
    reading.presence = (frameDataLen >= 1) ? gRxBuffer[6] : 0; // Читаем presence byte, если он присутствует в кадре.
    reading.hasTarget = (reading.presence != 0x00); // Интерпретируем наличие цели по presence byte.
    if (frameDataLen >= 3) { // Проверяем, что в кадре есть два байта расстояния.
      const uint16_t rangeCm = static_cast<uint16_t>(gRxBuffer[7]) | (static_cast<uint16_t>(gRxBuffer[8]) << 8); // Извлекаем расстояние в сантиметрах.
      if (rangeCm > 0) { reading.distanceM = static_cast<float>(rangeCm) / 100.0f; } // Переводим расстояние в метры.
    }
    if (frameDataLen >= 4) { reading.signal = static_cast<int>(gRxBuffer[9]); } // Считываем условный показатель энергии/сигнала.

    memmove(gRxBuffer, gRxBuffer + frameLen, gRxLen - frameLen); // Удаляем обработанный кадр из буфера.
    gRxLen -= frameLen; // Обновляем длину буфера после удаления кадра.
    break; // Возвращаем первое валидное чтение текущего цикла loop.
  }

  return reading; // Возвращаем результат текущего цикла чтения.
} // Завершаем функцию чтения и парсинга кадров.

static inline void ld2420ApplyRuntimeUartConfigIfChanged() { // Отслеживаем изменение UART-параметров из UI и применяем их на лету.
  const bool portChanged = (gAppliedUartPort != LD2420_UART_PORT); // Проверяем, изменился ли выбранный UART-порт.
  const bool pinsChanged = (gAppliedRxPin != LD2420_RX_PIN) || (gAppliedTxPin != LD2420_TX_PIN); // Проверяем, изменились ли пины RX/TX.
  const bool baudChanged = (gAppliedBaud != LD2420_BAUD_FIXED); // Проверяем, применена ли фиксированная скорость.
  if (!portChanged && !pinsChanged && !baudChanged) { return; } // Если изменений нет, выходим без перезапуска UART.
  if (portChanged) { // Если изменили UART-порт, пересоздаем объект HardwareSerial.
    if (gLd2420Serial) { gLd2420Serial->end(); delete gLd2420Serial; } // Корректно останавливаем и освобождаем старый UART-объект.
    gLd2420Serial = new HardwareSerial(LD2420_UART_PORT); // Создаем новый объект UART для выбранного порта.
  }
  ld2420BeginSerial(LD2420_BAUD_FIXED); // Перезапускаем UART с фиксированной скоростью.
  gRxLen = 0; // Сбрасываем бинарный буфер после смены параметров UART.
  gTextLineBuffer = ""; // Сбрасываем текстовый буфер после смены параметров UART.
} // Завершаем функцию горячего применения UART-настроек.

static inline void ld2420HandleUserCommands() { // Выполняем команды управления LD2420 из UI-флагов.
  if (LD2420_CMD_ENABLE_CONFIG) { ld2420SendEnableConfig(); LD2420_CMD_ENABLE_CONFIG = false; } // Включаем config mode и сбрасываем флаг команды.
  if (LD2420_CMD_DISABLE_CONFIG) { ld2420SendDisableConfig(); LD2420_CMD_DISABLE_CONFIG = false; } // Выключаем config mode и сбрасываем флаг команды.
  if (LD2420_CMD_SET_SIMPLE_MODE) { ld2420SendSetSimpleMode(); LD2420_CMD_SET_SIMPLE_MODE = false; } // Переключаемся в simple mode и сбрасываем флаг команды.
  if (LD2420_CMD_SET_ENERGY_MODE) { ld2420SendSetEnergyMode(); LD2420_CMD_SET_ENERGY_MODE = false; } // Переключаемся в energy mode и сбрасываем флаг команды.
  if (LD2420_CMD_READ_VERSION) { ld2420SendReadVersion(); LD2420_CMD_READ_VERSION = false; } // Запрашиваем версию прошивки и сбрасываем флаг команды.
  if (LD2420_CMD_REBOOT) { ld2420SendReboot(); LD2420_CMD_REBOOT = false; gDriverStartedAt = millis(); LD2420_LAST_FRAME_AT_MS = 0; } // Перезагружаем датчик и перезапускаем watchdog восстановления потока.
  if (LD2420_CMD_FACTORY_RESET) { ld2420SendFactoryReset(); LD2420_CMD_FACTORY_RESET = false; gDriverStartedAt = millis(); LD2420_LAST_FRAME_AT_MS = 0; } // После factory reset повторно запускаем recovery-логику как после холодного старта.
} // Завершаем функцию обработки команд из UI.

static inline void ld2420TryAutoRecovery(uint32_t now) { // Автоматически восстанавливаем поток данных, если датчик после рестарта молчит.
  if (!LD2420_AUTO_ENERGY_RECOVERY) { return; } // Если авто-восстановление выключено пользователем, ничего не делаем.
  if (LD2420_LAST_FRAME_AT_MS > 0) { return; } // Если уже были валидные кадры после старта, аварийное восстановление не нужно.
  if ((now - gDriverStartedAt) < 800UL) { return; } // Даем датчику небольшое время «проснуться» после старта ESP32.
  if ((now - gLastRxByteAt) < LD2420_RECOVERY_SILENCE_MS) { return; } // Если байты все же идут, recovery пока не запускаем.
  if (gLastRecoveryAttemptAt > 0 && (now - gLastRecoveryAttemptAt) < LD2420_RECOVERY_RETRY_MS) { return; } // Соблюдаем паузу между recovery-попытками.
  gLastRecoveryAttemptAt = now; // Запоминаем время текущей recovery-попытки.
  gRecoveryEnergyForced = true; // Фиксируем, что автоматический ENERGY MODE уже отправляли.
  ld2420BeginSerial(LD2420_BAUD_FIXED); // Перед recovery принудительно возвращаемся на фиксированный baud для надежной команды.
  gRxLen = 0; // Очищаем буфер бинарного парсера перед новой инициализацией потока.
  gTextLineBuffer = ""; // Очищаем буфер текстового парсера перед новой инициализацией потока.
  ld2420SendSetEnergyMode(); // Отправляем датчику команду перехода в ENERGY MODE, где чаще всего идет стабильный поток данных.
  LD2420_LINK_STATUS = "Auto recovery: ENERGY MODE"; // Показываем в UI, что выполнено авто-восстановление связи.
} // Завершаем функцию автоматического восстановления связи.

static inline void setup_LD2420() { // Инициализируем драйвер LD2420 и стартуем UART.
  if (gLd2420Serial) { gLd2420Serial->end(); delete gLd2420Serial; } // Если UART уже был создан, корректно пересоздаем его.
  gLd2420Serial = new HardwareSerial(LD2420_UART_PORT); // Создаем объект HardwareSerial для выбранного UART.
 ld2420BeginSerial(LD2420_BAUD_FIXED); // Запускаем UART на фиксированном baud.
  if (LD2420_FORCE_SIMPLE_MODE) { ld2420SendSetSimpleMode(); } // При необходимости сразу переводим датчик в simple mode.
  else { ld2420SendSetEnergyMode(); } // По умолчанию сразу переводим датчик в ENERGY MODE для стабильного потока измерений.
  LD2420_LINK_STATUS = "Ожидание данных"; // Устанавливаем стартовый статус линка.
  LD2420_VALID_FRAMES = 0; // Обнуляем счетчик валидных кадров.
  LD2420_INVALID_FRAMES = 0; // Обнуляем счетчик невалидных кадров.
  LD2420_LAST_FRAME_AT_MS = 0; // Сбрасываем timestamp последнего валидного кадра.
  LD2420_LAST_DISTANCE_AGE_MS = 0; // Сбрасываем возраст последнего расстояния.
  LD2420_LAST_LINE = ""; // Очищаем буфер последней текстовой строки.
  gDriverStartedAt = millis(); // Фиксируем момент старта драйвера для алгоритма авто-восстановления.
  gLastRecoveryAttemptAt = 0; // Сбрасываем таймер предыдущей попытки recovery.
  gRecoveryEnergyForced = false; // Сбрасываем флаг выполнения auto recovery.
  gDistanceWindowCount = 0; // Очищаем количество точек в окне фильтра.
  gDistanceWindowIndex = 0; // Сбрасываем индекс кольцевого окна фильтра.
  gDistanceWindow[0] = 0.0f; // Обнуляем первую точку окна фильтра.
  gDistanceWindow[1] = 0.0f; // Обнуляем вторую точку окна фильтра.
  gDistanceWindow[2] = 0.0f; // Обнуляем третью точку окна фильтра.
} // Завершаем функцию инициализации LD2420.

static inline void loop_LD2420() { // Главный цикл обслуживания датчика LD2420 (вызывается из loop()).
  const uint32_t now = millis(); // Считываем текущее время для таймеров и диагностики.
  ld2420ApplyRuntimeUartConfigIfChanged(); // Применяем измененные UART-настройки, если пользователь обновил их в UI.
  ld2420HandleUserCommands(); // Обрабатываем команды управления датчиком из UI.


  ld2420TryAutoRecovery(now); // Если датчик молчит после рестарта, автоматически отправляем ENERGY MODE для восстановления потока.
  LD2420Reading reading = ld2420ReadFrame(); // Читаем очередное измерение из входного потока UART.
  if (reading.frameValid) { // Если пришел валидный кадр, обновляем все рабочие переменные.
    LD2420_VALID_FRAMES++; // Увеличиваем счетчик валидных кадров.
    LD2420_LAST_FRAME_AT_MS = static_cast<int>(now); // Запоминаем время последнего валидного кадра.
    LD2420_LINK_STATUS = "Связь OK"; // Отмечаем исправную связь с датчиком.
    LD2420_HAS_TARGET = reading.hasTarget; // Обновляем флаг наличия цели.
    LD2420_PRESENCE_BYTE = static_cast<int>(reading.presence); // Обновляем сырое значение presence byte.
    LD2420_SIGNAL_VALUE = reading.signal; // Обновляем показатель сигнала/энергии.
    if (reading.distanceM > 0.01f && reading.distanceM < 15.0f) { // Проверяем, что расстояние лежит в реалистичном диапазоне.
      gLastDistanceM = ld2420FilterDistance(reading.distanceM); // Пропускаем raw через фильтры для стабильного и правдоподобного расстояния.
    }
  }

  if (LD2420_LAST_FRAME_AT_MS > 0) { LD2420_LAST_DISTANCE_AGE_MS = static_cast<int>(now - static_cast<uint32_t>(LD2420_LAST_FRAME_AT_MS)); } // Вычисляем возраст последнего валидного кадра.
  else { LD2420_LAST_DISTANCE_AGE_MS = 0; } // Если валидных кадров еще не было, возраст равен нулю.

  if (LD2420_LAST_FRAME_AT_MS > 0 && (now - static_cast<uint32_t>(LD2420_LAST_FRAME_AT_MS)) > 3000UL) { LD2420_LINK_STATUS = "Нет новых кадров"; } // Отмечаем потерю потока при долгом отсутствии кадров.
  if ((now - gLastRxByteAt) > 5000UL) { LD2420_LINK_STATUS = "UART тишина"; } // Отмечаем отсутствие любых байтов в UART.

  LD2420_DISTANCE_M = gLastDistanceM; // Публикуем расстояние в метрах для UI.
  LD2420_DISTANCE_CM = LD2420_DISTANCE_M * 100.0f; // Публикуем расстояние в сантиметрах для UI.
  LD2420_DISTANCE_MM = LD2420_DISTANCE_M * 1000.0f; // Публикуем расстояние в миллиметрах для UI.
  LD2420_DISTANCE_GRAPH_M = LD2420_DISTANCE_M; // Публикуем расстояние в источник графика на веб-странице.
} // Завершаем основной цикл обслуживания LD2420.