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

inline uint32_t KITCHEN_APPROACH_ANIMATION_MS = 1100UL; // Длительность анимации включения от краев к центру.
inline uint32_t KITCHEN_TRANSITION_WAIT_MS = 20000UL; // Задержка перед переходом из TRANSITION в AMBIENT.
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
static uint32_t gNearCandidateSince = 0; // Время начала кандидата на ближнюю зону.
static uint32_t gFarCandidateSince = 0; // Время начала кандидата на дальнюю зону.
static uint32_t gLastSeenAt = 0; // Время последнего достоверного обнаружения цели.
static float gDistanceWindow[KITCHEN_FILTER_WINDOW_MAX] = {0}; // Буфер скользящего среднего по расстоянию.
static uint16_t gDistanceCount = 0; // Количество фактически заполненных элементов буфера.
static uint16_t gDistanceIndex = 0; // Текущий индекс записи в кольцевой буфер.
static float gFilteredDistanceM = 99.0f; // Сглаженное расстояние до цели.

static inline void kitchenSetState(LightingState next, uint32_t now) { // Единая точка смены состояния автомата.
    gLightingState = next; // Запоминаем новое состояние автомата.
    gStateStartedAt = now; // Фиксируем время входа в состояние.
} // Закрываем функцию перехода состояния.

static inline SensorReading readHlkSensor() { // Читаем сырые данные от HLK-LD2410C через UART.
    SensorReading reading; // Создаем результат чтения с дефолтными значениями.
    if (!gHlkSerial) { return reading; } // Если UART еще не инициализирован, возвращаем пустое чтение.
    while (gHlkSerial->available() > 0) { // Вычитываем все накопленные символы из UART-буфера.
        String line = gHlkSerial->readStringUntil('\n'); // Берем строку до перевода строки.
        line.trim(); // Убираем пробелы и служебные символы по краям.
        if (line.length() == 0) { continue; } // Пустые строки игнорируем.
        int idx = line.indexOf("distance="); // Ищем ожидаемый ключ расстояния в телеметрии.
        if (idx < 0) { idx = line.indexOf("dist="); } // Пробуем альтернативный короткий ключ.
        if (idx >= 0) { // Если нашли один из поддерживаемых ключей.
            int eq = line.indexOf('=', idx); // Находим позицию символа '='.
            if (eq >= 0) { // Проверяем, что значение действительно присутствует.
                float distM = line.substring(eq + 1).toFloat(); // Преобразуем хвост строки в число метров.
                if (distM > 0.01f && distM < 12.0f) { // Фильтруем заведомо невалидные дистанции.
                    reading.hasTarget = true; // Отмечаем, что цель обнаружена.
                    reading.distanceM = distM; // Запоминаем прочитанную дистанцию.
                } // Завершаем проверку диапазона дистанции.
            } // Завершаем проверку наличия '='.
        } // Завершаем ветку обработки строки с дистанцией.
    } // Завершаем чтение очереди UART.
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
    const float progress = min(1.0f, static_cast<float>(elapsed) / KITCHEN_APPROACH_ANIMATION_MS); // Нормируем прогресс 0..1.
    const uint16_t half = NUM_LEDS / 2; // Вычисляем половину длины ленты.
    const uint16_t litPerSide = static_cast<uint16_t>(half * progress); // Сколько пикселей на каждой стороне уже зажечь.
    clearToOrdered(RgbColor(0, 0, 0)); // Очищаем ленту перед кадром анимации.
    for (uint16_t i = 0; i < litPerSide; ++i) { // Проходим по зажигаемой зоне слева и справа.
        setPixelColorOrdered(i, RgbColor(255, 255, 255)); // Зажигаем симметричный пиксель слева белым.
        setPixelColorOrdered(NUM_LEDS - 1 - i, RgbColor(255, 255, 255)); // Зажигаем симметричный пиксель справа белым.
    } // Завершаем заполнение до центра.
    if ((NUM_LEDS % 2U) != 0U && litPerSide >= half) { // Отдельно обрабатываем центральный пиксель для нечетной длины.
        setPixelColorOrdered(half, RgbColor(255, 255, 255)); // Включаем центр белым при схождении фронтов.
    } // Завершаем обработку центрального пикселя.
    uint8_t dynamicBrightness = map(min<uint32_t>(elapsed, KITCHEN_FADE_IN_MS), 0, KITCHEN_FADE_IN_MS, 1, KITCHEN_WORK_WHITE_BRIGHTNESS); // Формируем плавный рост яркости.
    ledStrip.SetBrightness(dynamicBrightness); // Применяем плавную яркость кадра APPROACH.
    ledStrip.Show(); // Отправляем кадр на ленту.
} // Закрываем отрисовку APPROACH.

static inline void renderWorkLight() { // Отрисовываем стабильный рабочий белый свет.
    ledStrip.SetBrightness(KITCHEN_WORK_WHITE_BRIGHTNESS); // Устанавливаем целевую яркость рабочего режима.
    clearToOrdered(RgbColor(255, 255, 255)); // Заполняем всю ленту ровным белым цветом.
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

static inline bool isNearConfirmed(uint32_t now) { // Проверяем подтверждение ближней зоны с debounce.
    if (gFilteredDistanceM <= KITCHEN_DISTANCE_NEAR_ENTER_M) { // Если усредненная дистанция в пороге входа в near.
        if (gNearCandidateSince == 0) { gNearCandidateSince = now; } // Запоминаем старт кандидата на near.
        return (now - gNearCandidateSince) >= KITCHEN_SENSOR_CONFIRM_MS; // Подтверждаем near после выдержки времени.
    } // Завершаем ветку обработки near.
    gNearCandidateSince = 0; // Сбрасываем кандидат near при выходе за порог.
    return false; // Не подтверждаем near при отсутствии выдержки/условия.
} // Закрываем проверку near.

static inline bool isFarConfirmed(uint32_t now) { // Проверяем подтверждение дальней зоны с debounce.
    if (gFilteredDistanceM >= KITCHEN_DISTANCE_FAR_ENTER_M) { // Если усредненная дистанция перешла в дальнюю зону.
        if (gFarCandidateSince == 0) { gFarCandidateSince = now; } // Фиксируем начало кандидата на far.
        return (now - gFarCandidateSince) >= KITCHEN_SENSOR_CONFIRM_MS; // Подтверждаем far после выдержки.
    } // Завершаем ветку обработки far.
    gFarCandidateSince = 0; // Сбрасываем кандидат far, если дистанция снова уменьшилась.
    return false; // Не подтверждаем far без стабильного условия.
} // Закрываем проверку far.

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
    SensorReading reading = readHlkSensor(); // Получаем новое измерение от датчика.
    if (reading.hasTarget) { // Если обнаружена валидная цель.
        gLastSeenAt = now; // Обновляем время последнего уверенного обнаружения.
        updateDistanceFilter(reading); // Обновляем сглаженную дистанцию.
    } else if ((now - gLastSeenAt) <= KITCHEN_SIGNAL_HOLD_MS) { // Если цель кратко пропала, но в допустимом hold-окне.
        reading.hasTarget = true; // Считаем цель условно присутствующей для устойчивости логики.
    } // Завершаем обработку кратковременного пропадания цели.

    const bool nearConfirmed = isNearConfirmed(now); // Вычисляем подтвержденный факт близости к столешнице.
    const bool farConfirmed = isFarConfirmed(now); // Вычисляем подтвержденный факт ухода в дальнюю зону.

    switch (gLightingState) { // Запускаем обработку переходов конечного автомата.
        case LightingState::IDLE: { // Лента выключена, ждем подхода.
            if (nearConfirmed) { kitchenSetState(LightingState::APPROACH, now); } // При подтвержденном подходе стартуем анимацию включения.
            renderIdleFade(now); // Поддерживаем плавное погашение до полного off.
            break; // Завершаем обработку состояния IDLE.
        } // Закрываем ветку IDLE.
        case LightingState::APPROACH: { // Идет анимация от краев к центру.
            renderApproach(now); // Отрисовываем текущий кадр APPROACH.
            if ((now - gStateStartedAt) >= KITCHEN_APPROACH_ANIMATION_MS) { kitchenSetState(LightingState::WORK_LIGHT, now); } // После завершения анимации фиксируем рабочий свет.
            break; // Завершаем обработку APPROACH.
        } // Закрываем ветку APPROACH.
        case LightingState::WORK_LIGHT: { // Ровный белый свет над столешницей.
            renderWorkLight(); // Удерживаем стабильный рабочий белый.
            if (nearConfirmed || gFilteredDistanceM <= KITCHEN_DISTANCE_NEAR_EXIT_M) { gFarCandidateSince = 0; } // Пока человек рядом, сбрасываем сценарий ухода.
            if (farConfirmed && gFilteredDistanceM >= KITCHEN_DISTANCE_NEAR_EXIT_M) { kitchenSetState(LightingState::TRANSITION, now); } // При подтвержденном удалении уходим в TRANSITION.
            break; // Завершаем обработку WORK_LIGHT.
        } // Закрываем ветку WORK_LIGHT.
        case LightingState::TRANSITION: { // Выжидание перед декоративным режимом.
            renderWorkLight(); // Во время ожидания сохраняем рабочий белый свет.
            if (nearConfirmed || gFilteredDistanceM <= KITCHEN_DISTANCE_FAR_EXIT_M) { kitchenSetState(LightingState::WORK_LIGHT, now); } // Если человек вернулся, мгновенно возвращаем WORK_LIGHT.
            else if ((now - gStateStartedAt) >= KITCHEN_TRANSITION_WAIT_MS) { kitchenSetState(LightingState::AMBIENT, now); } // После 20 секунд без возврата включаем AMBIENT.
            break; // Завершаем обработку TRANSITION.
        } // Закрываем ветку TRANSITION.
        case LightingState::AMBIENT: { // Декоративный фоновый режим.
            renderAmbient(now); // Отрисовываем цветную программу ambient-режима.
            if (nearConfirmed) { kitchenSetState(LightingState::APPROACH, now); } // При новом приближении снова запускаем анимацию подхода.
            else if ((now - gStateStartedAt) >= KITCHEN_AMBIENT_DURATION_MS) { kitchenSetState(LightingState::IDLE, now); } // Через 10 минут уходим в IDLE и плавно гасим.
            break; // Завершаем обработку AMBIENT.
        } // Закрываем ветку AMBIENT.
    } // Закрываем switch по состояниям автомата.
} // Закрываем основной update автоматики кухни.