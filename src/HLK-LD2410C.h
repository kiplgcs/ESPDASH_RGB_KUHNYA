#pragma once // Защита от повторного включения заголовка.

#include <Arduino.h> // Подключаем базовые типы и функции Arduino.
#include "LED_WS2815.h" // Подключаем управление лентой WS2815 и вспомогательные функции отрисовки.

// -------------------- Логика работы подсветки (попунктно) --------------------
// 1) IDLE: лента выключена, контроллер только слушает датчик и копит стабильные измерения дистанции.
// 2) APPROACH: при подтвержденном приближении (по времени + фильтру) запускается белая анимация от краев к центру.
// 3) WORK_LIGHT: после APPROACH включается постоянный белый рабочий свет, таймер ухода сбрасывается пока человек рядом.
// 4) TRANSITION: если человек ушел далеко, включается ожидание 20 секунд; возврат в ближнюю зону отменяет переход.
// 5) AMBIENT: если в TRANSITION никто не вернулся, запускается декоративный цветной режим на 10 минут.
// 6) Возврат в IDLE: по истечении AMBIENT выполняется плавное выключение и система снова ждет новое приближение.
// 7) Стабилизация: переключения не делаются по одному кадру — используется скользящее среднее, debounce и hold сигнала.
// 8) Гистерезис: пороги входа/выхода из ближней и дальней зон различаются, чтобы убрать дребезг на границах расстояний.

// -------------------- Настройки подключения HLK-LD2410C --------------------
inline int HLK_LD2410C_RX_PIN = 16; // RX-пин ESP32 для приема данных от TX HLK-LD2410C.
inline int HLK_LD2410C_TX_PIN = 17; // TX-пин ESP32 для передачи команд в RX HLK-LD2410C.
inline uint32_t HLK_LD2410C_BAUD = 256000UL; // Скорость UART для HLK-LD2410C (стандартная для модуля).
inline uint8_t HLK_LD2410C_UART_PORT = 2; // Используем аппаратный порт Serial2 на ESP32.

// -------------------- Настройки автоматики кухни --------------------
inline float KITCHEN_DISTANCE_NEAR_ENTER_M = 3.0f; // Порог входа в ближнюю зону (подход к столешнице).
inline float KITCHEN_DISTANCE_NEAR_EXIT_M = 3.4f; // Порог выхода из ближней зоны (гистерезис вверх).
inline float KITCHEN_DISTANCE_FAR_ENTER_M = 3.8f; // Порог входа в дальнюю зону (гистерезис вниз/вверх).
inline float KITCHEN_DISTANCE_FAR_EXIT_M = 3.2f; // Порог выхода из дальней зоны (возврат к рабочей зоне).
inline float KITCHEN_NEAR_HYSTERESIS_M = 0.4f; // Гистерезис ближней зоны (добавляется к порогу входа, 0..1 м).
inline float KITCHEN_FAR_HYSTERESIS_M = 0.5f; // Гистерезис дальней зоны (добавляется к порогу входа, 0..1 м).
inline float KITCHEN_ZONE_MIN_GAP_M = 0.15f; // Минимальный технологический разрыв между соседними порогами зон.
inline int KITCHEN_LED_COUNT = 180; // Количество светодиодов, используемых автоматикой HLK (1..NUM_LEDS).
inline String KITCHEN_NEAR_ENTRY_EFFECT = "edge_white"; // Эффект перехода из дальней зоны в ближнюю.
inline uint32_t KITCHEN_NEAR_ENTRY_EFFECT_MS = 700UL; // Длительность эффекта перехода far->near.

inline uint32_t KITCHEN_APPROACH_ANIMATION_MS = 1100UL; // Длительность анимации включения от краев к центру.
inline uint32_t KITCHEN_TRANSITION_WAIT_MS = 700UL; // Задержка перехода из ближней зоны (белый) в дальнюю (RGB).
inline uint32_t KITCHEN_LIGHTS_OFF_DELAY_MS = 20000UL; // Задержка полного выключения при отсутствии цели в зонах.
inline uint32_t KITCHEN_AMBIENT_DURATION_MS = 600000UL; // Время работы декоративного режима (10 минут).
inline uint32_t KITCHEN_SENSOR_CONFIRM_MS = 700UL; // Время подтверждения смены зоны по датчику.
inline uint32_t KITCHEN_SIGNAL_HOLD_MS = 1500UL; // Время удержания последней валидной цели при кратком пропадании.
inline uint32_t KITCHEN_FADE_IN_MS = 400UL; // Время плавного включения/подъема яркости.
inline uint32_t KITCHEN_FADE_OUT_MS = 900UL; // Время плавного выключения/спада яркости.

inline uint8_t KITCHEN_WORK_WHITE_BRIGHTNESS = 220; // Яркость белого рабочего режима.
inline uint8_t KITCHEN_AMBIENT_BRIGHTNESS = 96; // Яркость декоративного режима.
inline uint16_t KITCHEN_FILTER_WINDOW = 6; // Окно скользящего среднего для сглаживания дистанции.
constexpr uint16_t KITCHEN_FILTER_WINDOW_MAX = 32; // Максимальный размер окна сглаживания для статического буфера.

// -------------------- Состояния автоматики подсветки --------------------
enum class LightingState : uint8_t { // Определяем конечный автомат подсветки кухни.
    IDLE = 0, // Лента выключена, ожидание присутствия.
    APPROACH, // Анимация подхода от краев к центру.
    WORK_LIGHT, // Основной ровный белый рабочий свет.
    TRANSITION, // Пауза перед переходом в декоративный режим.
    AMBIENT // Декоративный цветной режим на ограниченное время.
};

struct SensorReading { // Структура для унифицированного чтения данных датчика.
    bool hasTarget = false; // Флаг наличия цели в текущем измерении.
    float distanceM = 99.0f; // Текущая дистанция до цели в метрах.
};

static HardwareSerial *gHlkSerial = nullptr; // Указатель на UART-экземпляр для HLK-LD2410C.
static LightingState gLightingState = LightingState::IDLE; // Текущее состояние конечного автомата подсветки.
static uint32_t gStateStartedAt = 0; // Время входа в текущее состояние.
static uint32_t gLastSeenAt = 0; // Время последнего достоверного обнаружения цели.
static float gDistanceWindow[KITCHEN_FILTER_WINDOW_MAX] = {0}; // Буфер скользящего среднего по расстоянию.
static uint16_t gDistanceCount = 0; // Количество фактически заполненных элементов буфера.
static uint16_t gDistanceIndex = 0; // Текущий индекс записи в кольцевой буфер.
static float gFilteredDistanceM = 99.0f; // Сглаженное расстояние до цели.
static uint8_t gHlkRxBuffer[192] = {0}; // Буфер сырых UART-байт для бинарных кадров протокола LD2410.
static size_t gHlkRxLen = 0; // Текущее число валидных байт в буфере бинарного парсера.
static LightingState gTransitionSourceState = LightingState::IDLE; // Запоминаем режим, из которого вошли в таймерное выключение.
static uint32_t gOutsideZonesSince = 0; // Время, с которого цель отсутствует в ближней/дальней зоне.
static LightingState gOutsideHoldState = LightingState::IDLE; // Режим, который удерживаем во время таймера отключения.
static bool gNearSessionArmed = false; // Разрешение включать RGB в дальней зоне только после факта входа в ближнюю зону.
enum class ProximityZone : uint8_t { NONE = 0, FAR, NEAR }; // Дискретные зоны присутствия для устойчивого автомата.
static ProximityZone gStableZone = ProximityZone::NONE; // Подтвержденная (debounced) зона.
static ProximityZone gCandidateZone = ProximityZone::NONE; // Кандидат зоны до истечения debounce.
static uint32_t gZoneCandidateSince = 0; // Время старта кандидата зоны.
static uint32_t gFarTransitionSince = 0; // Время начала перехода ближняя->дальняя до включения RGB.

static inline void kitchenSetState(LightingState next, uint32_t now) { // Единая точка смены состояния автомата.
    gLightingState = next; // Запоминаем новое состояние автомата.
    gStateStartedAt = now; // Фиксируем время входа в состояние.
} // Закрываем функцию перехода состояния.

static inline SensorReading readHlkSensor() { // Читаем сырые данные от HLK-LD2410C через UART.
    SensorReading reading; // Создаем результат чтения с дефолтными значениями.
    if (!gHlkSerial) { return reading; } // Если UART еще не инициализирован, возвращаем пустое чтение.
while (gHlkSerial->available() > 0) { // Вычитываем сырые байты протокола.
        int b = gHlkSerial->read(); // Забираем очередной байт из UART.
        if (b < 0) { break; } // На случай редкой гонки по буферу UART прекращаем чтение.
        if (gHlkRxLen >= sizeof(gHlkRxBuffer)) { // Если буфер переполнен шумом/сдвигом кадров.
            memmove(gHlkRxBuffer, gHlkRxBuffer + 1, sizeof(gHlkRxBuffer) - 1); // Сдвигаем окно на 1 байт влево.
            gHlkRxLen = sizeof(gHlkRxBuffer) - 1; // Корректируем длину после сдвига.
        } // Завершаем ветку защиты от переполнения.
        gHlkRxBuffer[gHlkRxLen++] = static_cast<uint8_t>(b); // Добавляем байт в хвост накопителя.
    } // Завершаем набор доступных байт текущей итерации.

    // Парсим бинарные кадры датчика согласно протоколу HLK-LD2410: F4 F3 F2 F1 ... F8 F7 F6 F5.
    while (gHlkRxLen >= 10) { // Минимальный размер валидного кадра: header(4)+len(2)+data(>=0)+footer(4).
        size_t start = 0; // Индекс начала заголовка кадра.
        while (start + 3 < gHlkRxLen) { // Ищем сигнатуру заголовка в накопленном буфере.
            if (gHlkRxBuffer[start] == 0xF4 && gHlkRxBuffer[start + 1] == 0xF3 &&
                gHlkRxBuffer[start + 2] == 0xF2 && gHlkRxBuffer[start + 3] == 0xF1) { break; }
            start++; // Сдвигаем окно поиска до совпадения заголовка.
        }
        if (start > 0) { // Отбрасываем весь мусор перед найденным заголовком.
            memmove(gHlkRxBuffer, gHlkRxBuffer + start, gHlkRxLen - start);
            gHlkRxLen -= start;
        }
        if (gHlkRxLen < 6) { break; } // Пока не хватает даже заголовка+длины — ждем следующую порцию.

        const uint16_t frameDataLen = static_cast<uint16_t>(gHlkRxBuffer[4]) |
                                      (static_cast<uint16_t>(gHlkRxBuffer[5]) << 8); // LE длина payload.
        if (frameDataLen == 0 || frameDataLen > 128) { // Защита от ложного заголовка и битого потока.
            memmove(gHlkRxBuffer, gHlkRxBuffer + 1, gHlkRxLen - 1);
            gHlkRxLen -= 1;
            continue;
        }

        const size_t frameLen = static_cast<size_t>(frameDataLen) + 10; // header+len+data+footer.
        if (gHlkRxLen < frameLen) { break; } // Ждем дочитывания полного кадра.

        const size_t footerPos = 6 + frameDataLen; // Позиция начала footer в буфере.
        const bool footerOk = gHlkRxBuffer[footerPos] == 0xF8 && gHlkRxBuffer[footerPos + 1] == 0xF7 &&
                              gHlkRxBuffer[footerPos + 2] == 0xF6 && gHlkRxBuffer[footerPos + 3] == 0xF5;
        if (!footerOk) { // При поврежденном footer сдвигаемся на байт и продолжаем ресинхронизацию.
            memmove(gHlkRxBuffer, gHlkRxBuffer + 1, gHlkRxLen - 1);
            gHlkRxLen -= 1;
            continue;
        }

        const uint8_t *frameData = gHlkRxBuffer + 6; // Указатель на внутренние данные кадра.
        // Нормальный кадр содержит: type(0x02), 0xAA, payload..., 0x55, 0x00.
        if (frameDataLen >= 13 && frameData[0] == 0x02 && frameData[1] == 0xAA &&
            frameData[frameDataLen - 2] == 0x55 && frameData[frameDataLen - 1] == 0x00) {
            const uint8_t targetState = frameData[2]; // 0x00 нет цели, 0x01/0x02/0x03 цель есть.
            const uint16_t movingCm = static_cast<uint16_t>(frameData[3]) |
                                      (static_cast<uint16_t>(frameData[4]) << 8); // Дистанция движущейся цели (см).
            const uint16_t stillCm = static_cast<uint16_t>(frameData[6]) |
                                     (static_cast<uint16_t>(frameData[7]) << 8); // Дистанция статичной цели (см).
            const uint16_t detectCm = static_cast<uint16_t>(frameData[9]) |
                                      (static_cast<uint16_t>(frameData[10]) << 8); // Общая дистанция обнаружения (см).

            uint16_t chosenCm = 0; // Целевая дистанция для автоматики.
            if (movingCm > 0 && stillCm > 0) { chosenCm = min(movingCm, stillCm); } // Берем ближнюю цель.
            else if (movingCm > 0) { chosenCm = movingCm; } // Если есть только движущаяся цель.
            else if (stillCm > 0) { chosenCm = stillCm; } // Если есть только статичная цель.
            else { chosenCm = detectCm; } // Иначе используем общую дистанцию обнаружения.

            if (targetState != 0x00 && chosenCm > 0) { // Подтверждаем наличие цели в бинарном протоколе.
                reading.hasTarget = true;
                reading.distanceM = static_cast<float>(chosenCm) / 100.0f; // Приводим сантиметры к метрам.
            }
        } else {
            // Fallback для редких прошивок/шлюзов, которые отдают ASCII-строки вместо бинарных кадров.
            String line;
            line.reserve(frameDataLen);
            for (uint16_t i = 0; i < frameDataLen; ++i) { line += static_cast<char>(frameData[i]); }
            line.trim();
            String lineLower = line;
            lineLower.toLowerCase();
            int idx = lineLower.indexOf("distance");
            if (idx < 0) { idx = lineLower.indexOf("dist"); }
            if (idx >= 0) {
                int sep = line.indexOf('=', idx);
                if (sep < 0) { sep = line.indexOf(':', idx); }
                if (sep >= 0) {
                    String rawValue = line.substring(sep + 1);
                    rawValue.trim();
                    String rawValueLower = rawValue;
                    rawValueLower.toLowerCase();
                    float distM = rawValue.toFloat();
                    if (rawValueLower.indexOf("mm") >= 0) distM /= 1000.0f;
                    else if (rawValueLower.indexOf("cm") >= 0) distM /= 100.0f;
                    else if (distM > 12.0f) distM = (distM <= 1200.0f) ? (distM / 100.0f) : (distM / 1000.0f);
                    if (distM > 0.01f && distM < 12.0f) {
                        reading.hasTarget = true;
                        reading.distanceM = distM;
                    }
                }
            }
        }

        // Удаляем обработанный кадр и продолжаем разбор следующих кадров в буфере.
        memmove(gHlkRxBuffer, gHlkRxBuffer + frameLen, gHlkRxLen - frameLen);
        gHlkRxLen -= frameLen;
    }

    return reading; // Возвращаем унифицированный результат чтения датчика.
} // Закрываем функцию чтения датчика.

static inline void updateDistanceFilter(const SensorReading &reading) { // Обновляем фильтрацию шумных измерений.
    if (!reading.hasTarget) { return; } // Без валидной цели не обновляем окно среднего.
    const uint16_t activeWindow = static_cast<uint16_t>(constrain(KITCHEN_FILTER_WINDOW, static_cast<uint16_t>(1), KITCHEN_FILTER_WINDOW_MAX)); // Ограничиваем окно фильтра допустимым диапазоном.
    if (gDistanceCount > activeWindow) { gDistanceCount = activeWindow; } // При уменьшении окна ограничиваем объем валидных данных.
    if (gDistanceIndex >= activeWindow) { gDistanceIndex = 0; } // При смене окна синхронизируем индекс кольцевого буфера.
    gDistanceWindow[gDistanceIndex] = reading.distanceM; // Записываем текущее измерение в кольцевой буфер.
    gDistanceIndex = (gDistanceIndex + 1) % activeWindow; // Сдвигаем индекс записи по кольцу.
    if (gDistanceCount < activeWindow) { gDistanceCount++; } // Наращиваем заполненность буфера до активного размера.
    float sum = 0.0f; // Подготавливаем аккумулятор суммы.
    for (uint16_t i = 0; i < gDistanceCount; ++i) { sum += gDistanceWindow[i]; } // Суммируем все доступные элементы окна.
    gFilteredDistanceM = sum / static_cast<float>(gDistanceCount); // Получаем усредненную дистанцию.
} // Закрываем функцию фильтрации дистанции.

static inline void renderApproach(uint32_t now) { // Рисуем анимацию APPROACH: от краев к центру.
    const uint32_t elapsed = now - gStateStartedAt; // Считаем, сколько длится текущая анимация.
        const uint16_t activeLeds = static_cast<uint16_t>(constrain(KITCHEN_LED_COUNT, 1, NUM_LEDS)); // Активная длина ленты для автоматики.
    const uint8_t dynamicBrightness = map(min<uint32_t>(elapsed, KITCHEN_FADE_IN_MS), 0, KITCHEN_FADE_IN_MS, 1, KITCHEN_WORK_WHITE_BRIGHTNESS); // Формируем плавный рост яркости.
    ledStrip.SetBrightness(dynamicBrightness); // Применяем плавную яркость кадра APPROACH.
    
    clearToOrdered(RgbColor(0, 0, 0)); // Очищаем ленту перед кадром эффекта.
    const uint8_t frame = static_cast<uint8_t>((elapsed / 35UL) & 0xFFU); // Базовый счетчик кадра эффекта.
    const String effect = KITCHEN_NEAR_ENTRY_EFFECT; // Берем выбранный эффект перехода far->near.

    if (effect == "edge_white") { // Классический белый эффект от краев к центру.
        const float progress = min(1.0f, static_cast<float>(elapsed) / max<uint32_t>(1UL, KITCHEN_NEAR_ENTRY_EFFECT_MS));
        const uint16_t half = activeLeds / 2;
        const uint16_t litPerSide = static_cast<uint16_t>(half * progress);
        for (uint16_t i = 0; i < litPerSide; ++i) {
            setPixelColorOrdered(i, RgbColor(255, 255, 255));
            setPixelColorOrdered(activeLeds - 1 - i, RgbColor(255, 255, 255));
        }
        if ((activeLeds % 2U) != 0U && litPerSide >= half) { setPixelColorOrdered(half, RgbColor(255, 255, 255)); }
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
    else if (effect == "candy") fillCandy(frame);
    else if (effect == "twirl") fillTwirl(frame);
    else { fillRainbowFrame(frame); } // fallback на случай некорректно сохраненного значения.

    for (uint16_t i = activeLeds; i < NUM_LEDS; ++i) { setPixelColorOrdered(i, RgbColor(0, 0, 0)); } // Гасим хвост за пределом активной длины.
    ledStrip.Show(); // Отправляем кадр на ленту.
} // Закрываем отрисовку APPROACH.

static inline void renderWorkLight() { // Отрисовываем стабильный рабочий белый свет.
    const uint16_t activeLeds = static_cast<uint16_t>(constrain(KITCHEN_LED_COUNT, 1, NUM_LEDS)); // Активная длина ленты для белого режима.
    ledStrip.SetBrightness(KITCHEN_WORK_WHITE_BRIGHTNESS); // Устанавливаем целевую яркость рабочего режима.
        clearToOrdered(RgbColor(0, 0, 0)); // Сначала гасим всю ленту.
    for (uint16_t i = 0; i < activeLeds; ++i) { setPixelColorOrdered(i, RgbColor(255, 255, 255)); } // Включаем только активное число светодиодов.
    ledStrip.Show(); // Применяем кадр рабочего режима.
} // Закрываем отрисовку WORK_LIGHT.

static inline void renderIdleFade(uint32_t now) { // Выполняем плавное выключение в IDLE.
    uint32_t elapsed = now - gStateStartedAt; // Вычисляем длительность фазы выключения.
    if (elapsed >= KITCHEN_FADE_OUT_MS) { // Если fade-out завершился.
        ledStrip.SetBrightness(0); // Полностью опускаем яркость до нуля.
        clearToOrdered(RgbColor(0, 0, 0)); // Гарантированно гасим все пиксели.
        ledStrip.Show(); // Применяем финальный выключенный кадр.
        return; // Завершаем функцию после полного выключения.
    } // Завершаем ветку полного выключения.
    uint8_t b = map(elapsed, 0, KITCHEN_FADE_OUT_MS, KITCHEN_WORK_WHITE_BRIGHTNESS, 0); // Интерполируем яркость на спад.
    ledStrip.SetBrightness(b); // Применяем текущую затухающую яркость.
    clearToOrdered(RgbColor(255, 255, 255)); // Сохраняем нейтральный белый в момент затухания.
    ledStrip.Show(); // Обновляем ленту.
} // Закрываем отрисовку IDLE fade-out.

static inline void renderAmbient(uint32_t now) { // Отрисовываем декоративный режим AMBIENT.
ledStrip.SetBrightness(KITCHEN_AMBIENT_BRIGHTNESS); // Ограничиваем яркость, чтобы режим оставался спокойным.
    uint8_t frame = static_cast<uint8_t>((now / 35UL) & 0xFFU); // Строим кадровый счетчик для декоративной анимации.
    fillRainbowFrame(frame); // Используем цветную программу как фоновый ambient-режим.
    ledStrip.Show(); // Применяем текущий кадр декоративной программы.
} // Закрываем отрисовку AMBIENT.

static inline String rgbControlModeNormalized() { // Нормализуем выбранный пользователем режим RGB-управления.
    String mode = SetRGB; // Берем актуальное значение из UI/MQTT.
    mode.trim(); // Убираем случайные пробелы по краям.
    mode.toLowerCase(); // Приводим к нижнему регистру для надежного сравнения.
    return mode; // Возвращаем нормализованный режим.
} // Закрываем helper нормализации режима.

static inline void resetKitchenAutomationState(uint32_t now) { // Сбрасываем состояние автомата присутствия при выходе из режима AUTO.
    gDistanceCount = 0; // Очищаем окно фильтра дистанции.
    gDistanceIndex = 0; // Возвращаем индекс фильтра в начало.
    gFilteredDistanceM = 99.0f; // Ставим безопасную «далекую» дистанцию по умолчанию.
    gLastSeenAt = 0; // Сбрасываем время последнего валидного обнаружения.
    gHlkRxLen = 0; // Очищаем буфер бинарного UART-парсера при полном сбросе автоматики.
    gTransitionSourceState = LightingState::IDLE; // Сбрасываем источник transition при возврате в ручные режимы.
    gOutsideZonesSince = 0; // Сбрасываем таймер отсутствия цели в рабочих зонах.
    gOutsideHoldState = LightingState::IDLE; // Сбрасываем удерживаемый режим перед выключением.
    gNearSessionArmed = false; // Сбрасываем «сессию присутствия», чтобы дальняя зона из IDLE не включала RGB.
    gStableZone = ProximityZone::NONE; // Сбрасываем подтвержденную зону.
    gCandidateZone = ProximityZone::NONE; // Сбрасываем кандидата зоны.
    gZoneCandidateSince = 0; // Сбрасываем таймер debounce зоны.
    gFarTransitionSince = 0; // Сбрасываем таймер перехода белый->RGB.
    kitchenSetState(LightingState::IDLE, now); // Возвращаем автомат в состояние ожидания/выключения.
} // Закрываем helper сброса автоматики.


static inline void setup_HLK_LD2410C() { // Публичная инициализация датчика и автоматики.
    if (gHlkSerial) { // Если UART уже создавался ранее.
        gHlkSerial->end(); // Корректно закрываем предыдущую сессию UART перед переинициализацией.
        delete gHlkSerial; // Освобождаем старый объект HardwareSerial.
    }
    gHlkSerial = new HardwareSerial(HLK_LD2410C_UART_PORT); // Создаем UART с выбранным в UI номером порта.
    gHlkSerial->begin(HLK_LD2410C_BAUD, SERIAL_8N1, HLK_LD2410C_RX_PIN, HLK_LD2410C_TX_PIN); // Поднимаем UART-связь с HLK-LD2410C.
    setup_WS2815(); // Инициализируем подсистему светодиодной ленты.
    kitchenSetState(LightingState::IDLE, millis()); // Стартуем автоматику из состояния покоя.
} // Закрываем функцию setup для HLK + света.

static inline void loop_HLK_LD2410C() { // Основной update-метод state machine, вызывается в loop().
    const uint32_t now = millis(); // Берем текущее системное время для таймеров автомата.
    
    const String mode = rgbControlModeNormalized(); // Читаем режим RGB-управления (off/on/auto).

    if (mode != "auto") { // В режимах OFF/ON датчик присутствия не должен управлять лентой.
        resetKitchenAutomationState(now); // Очищаем состояние датчика, чтобы AUTO стартовал «с нуля».
        loop_WS2815(); // Работаем только штатным RGB-движком (паттерны/цвет/яркость/таймер).
        return; // Выходим, не выполняя автоматику присутствия.
    }

    SensorReading reading = readHlkSensor(); // Получаем новое измерение от датчика.
    if (reading.hasTarget) { // Если обнаружена валидная цель.
        gLastSeenAt = now; // Обновляем время последнего уверенного обнаружения.
        updateDistanceFilter(reading); // Обновляем сглаженную дистанцию.
    } else if ((now - gLastSeenAt) <= KITCHEN_SIGNAL_HOLD_MS) { // Если цель кратко пропала, но в допустимом hold-окне.
        reading.hasTarget = true; // Считаем цель условно присутствующей для устойчивости логики.
    } // Завершаем обработку кратковременного пропадания цели.

const float nearHyst = constrain(KITCHEN_NEAR_HYSTERESIS_M, 0.0f, 1.0f); // Ограничиваем гистерезис ближней зоны 0..1 м.
    const float farHyst = constrain(KITCHEN_FAR_HYSTERESIS_M, 0.0f, 1.0f); // Ограничиваем гистерезис дальней зоны 0..1 м.

    const float nearEnter = constrain(KITCHEN_DISTANCE_NEAR_ENTER_M, 0.0f, 9.0f); // Порог входа в ближнюю зону.
    const float nearExit = constrain(max(KITCHEN_DISTANCE_NEAR_EXIT_M, nearEnter + nearHyst), nearEnter + KITCHEN_ZONE_MIN_GAP_M, 9.4f); // Порог выхода из ближней зоны.
    const float farEnter = constrain(max(KITCHEN_DISTANCE_FAR_ENTER_M, nearExit + KITCHEN_ZONE_MIN_GAP_M), nearExit + KITCHEN_ZONE_MIN_GAP_M, 10.0f); // Порог входа в дальнюю зону (информативный/UI).
    const float farExit = constrain(max(KITCHEN_DISTANCE_FAR_EXIT_M, nearEnter + max(farHyst, KITCHEN_ZONE_MIN_GAP_M)), nearEnter + KITCHEN_ZONE_MIN_GAP_M, max(nearEnter + KITCHEN_ZONE_MIN_GAP_M, farEnter - KITCHEN_ZONE_MIN_GAP_M)); // Порог возврата far->near.
    KITCHEN_DISTANCE_NEAR_ENTER_M = nearEnter; // Синхронизируем UI/настройки с нормализованными порогами.
    KITCHEN_DISTANCE_NEAR_EXIT_M = nearExit;
    KITCHEN_DISTANCE_FAR_ENTER_M = farEnter;
    KITCHEN_DISTANCE_FAR_EXIT_M = farExit;

    ProximityZone measuredZone = ProximityZone::NONE; // Вычисляем мгновенную зону с учетом текущей стабильной зоны (гистерезис).
    if (reading.hasTarget) {
        const float d = gFilteredDistanceM;
        if (gStableZone == ProximityZone::NEAR) { // Из NEAR выходим только после nearExit.
            if (d <= nearExit) { measuredZone = ProximityZone::NEAR; }
else { measuredZone = ProximityZone::FAR; } // Любая дистанция дальше nearExit считается дальней зоной.
        } else if (gStableZone == ProximityZone::FAR) { // Из FAR возвращаемся в near только когда реально подошли близко.
            if (d <= farExit) { measuredZone = ProximityZone::NEAR; }
            else { measuredZone = ProximityZone::FAR; }
        } else { // В состоянии NONE делим пространство на near и far без "мертвой" зоны.
            if (d <= nearEnter) { measuredZone = ProximityZone::NEAR; }
            else { measuredZone = ProximityZone::FAR; }
        }
    }

    if (measuredZone != gCandidateZone) { // Новый кандидат зоны — запускаем debounce.
        gCandidateZone = measuredZone;
        gZoneCandidateSince = now;
    } else if (gStableZone != gCandidateZone && (now - gZoneCandidateSince) >= KITCHEN_SENSOR_CONFIRM_MS) { // Подтверждаем смену зоны.
        gStableZone = gCandidateZone;
    }

    if (gStableZone == ProximityZone::NEAR) { // Стабильная ближняя зона.
        gNearSessionArmed = true; // Разрешаем дальнейший RGB-сценарий после факта входа в near.
        gOutsideZonesSince = 0;
        gOutsideHoldState = LightingState::IDLE;
        gFarTransitionSince = 0;
        if (gLightingState != LightingState::WORK_LIGHT && gLightingState != LightingState::APPROACH) {
            kitchenSetState(LightingState::APPROACH, now); // Переход far->near всегда через эффект.
        } else if (gLightingState == LightingState::APPROACH &&
                   (now - gStateStartedAt) >= max<uint32_t>(1UL, KITCHEN_NEAR_ENTRY_EFFECT_MS)) {
            kitchenSetState(LightingState::WORK_LIGHT, now);
        }
    } else if (gStableZone == ProximityZone::FAR) { // Стабильная дальняя зона.
        gOutsideZonesSince = 0;
        gOutsideHoldState = LightingState::IDLE;
        if (!gNearSessionArmed) { // Из IDLE вход в far ничего не включает.
            gFarTransitionSince = 0;
            if (gLightingState != LightingState::IDLE) { kitchenSetState(LightingState::IDLE, now); }
            } else if (gLightingState == LightingState::WORK_LIGHT || gLightingState == LightingState::APPROACH) {
            if (gFarTransitionSince == 0) { gFarTransitionSince = now; } // Старт задержки near->far.
            if ((now - gFarTransitionSince) >= KITCHEN_TRANSITION_WAIT_MS) { kitchenSetState(LightingState::AMBIENT, now); }
        } else {
gFarTransitionSince = 0;
            if (gLightingState != LightingState::AMBIENT) { kitchenSetState(LightingState::AMBIENT, now); }
                    gOutsideHoldState = LightingState::IDLE; // Удержание для off-таймера больше не требуется.
        }
    } else { // NONE: вне зон.
        gFarTransitionSince = 0;
        if (gOutsideZonesSince == 0) {
            gOutsideZonesSince = now;
            gOutsideHoldState = (gLightingState == LightingState::AMBIENT) ? LightingState::AMBIENT : LightingState::WORK_LIGHT;
        }

        if (gOutsideHoldState == LightingState::AMBIENT && gLightingState != LightingState::AMBIENT) {
            kitchenSetState(LightingState::AMBIENT, now); // Держим RGB до off-таймера.
        } else if (gOutsideHoldState == LightingState::WORK_LIGHT && gLightingState != LightingState::WORK_LIGHT) {
            kitchenSetState(LightingState::WORK_LIGHT, now); // Держим белый до off-таймера.
        }

        if ((now - gOutsideZonesSince) >= KITCHEN_LIGHTS_OFF_DELAY_MS) {
          gNearSessionArmed = false;
            kitchenSetState(LightingState::IDLE, now);
        }
    }

    switch (gLightingState) { // Отрисовываем итоговый режим после вычисления переходов.
        case LightingState::WORK_LIGHT: { // Белый свет до 2 метров.
            Pow_WS2815 = false; // Отключаем RGB-движок, чтобы он не перетирал белый режим.
            renderWorkLight();
            break;
        }
        case LightingState::APPROACH: { // Эффект перехода из дальней зоны в ближнюю.
            Pow_WS2815 = false;
            renderApproach(now);
            break;
        }
        
        case LightingState::AMBIENT: { // RGB режимы в диапазоне 2..5 метров.
            Pow_WS2815 = true; // Включаем штатный движок WS2815 (паттерны/цвета/автоплей).
            loop_WS2815();
            break;
        }
        case LightingState::TRANSITION: { // В паузе удерживаем последний визуальный режим до off-таймера.
            if (gTransitionSourceState == LightingState::AMBIENT) { // Таймер перехода из белого в RGB: держим белый до конца паузы.
                Pow_WS2815 = false;
                renderWorkLight();
            } else if (gTransitionSourceState == LightingState::WORK_LIGHT) { // Обратная совместимость для старых переходов.
                Pow_WS2815 = false;
                renderWorkLight();
            } else {
                Pow_WS2815 = false;
                renderIdleFade(now);
            }
            break;
        }
        case LightingState::IDLE:
        default: { // Полностью выключенный режим.
            Pow_WS2815 = false;
            renderIdleFade(now);
            break;
        }
    }
} // Закрываем основной update автоматики кухни.