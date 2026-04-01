//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"
#include "HLK-LD2410C.h"

inline void interface(){ // Декларатиынве функции интерфейса
    // UI_APP("🏊 Управление подсветкой на кухне");
    UI_MENU("🌈 Управление RGB подсветкой");
    UI_HIDDEN("ThemeColor", ThemeColor);

    // UI_MENU("🧰 test");


        // UI_POPUP_BEGIN("HLK-LD2410C", "⚙️ Настройка работы RGB ленты и HLK-LD2410C", "⚙️ LED_WS2815 HLK-LD2410C"); 

        // UI_POPUP_END();

// Управление RGB подсветкой
    UI_PAGE();
    UI_BUTTON_DEFAULT("button_WS2815", Pow_WS2815, "gray", "🌈 Включить / Отключить : RGB ленту WS2815", 1);
    UI_CHECKBOX("WS2815_Time1", WS2815_Time1, "⏲️ Таймер RGB ленты"); //Галочка - активания/деактивация таймера
    UI_SELECT_CB("SetRGB", SetRGB, (std::initializer_list<UIOption>{{"off", "RGB подсветка отключена постоянно"},
                                   {"on", "RGB подсветка включена постоянно"},
                                   {"auto", "Автоматически по датчику присутствия"}}), "🎛️ Режим управления RGB подсветкой", onSetRgbChange);
        UI_TIMER("RgbTimer", "⏲️ Таймер RGB ленты", RgbTimerON, RgbTimerOFF, noopTimerCallback);
UI_COLOR("LEDColor", LEDColor, "🎨 Цвет подсветки");
    UI_SELECT_CB("LedColorMode", LedColorMode, (std::initializer_list<UIOption>{{"auto", "Автоматически"},
                                               {"manual", "Ручной цвет"}}), "🎨 Режим цвета", onLedColorModeChange);
    UI_RANGE_CB("LedBrightness", LedBrightness, 10, 255, 1, "🔆 Яркость", onLedBrightnessChange);
    UI_SELECT("LedPattern", LedPattern, (std::initializer_list<UIOption>{{"rainbow", "Радуга"},
                                         {"pulse", "Пульс"},
                                         {"chase", "Шлейф"},
                                         {"comet", "Комета"},
                                         {"color_wipe", "Цветовая заливка"},
                                         {"theater_chase", "Театр"},
                                         {"scanner", "Сканер"},
                                         {"sparkle", "Искры"},
                                         {"twinkle", "Мерцание"},
                                         {"confetti", "Конфетти"},
                                         {"waves", "Волны"},
                                         {"breathe", "Дыхание"},
                                         {"firefly", "Светлячки"},
                                         {"ripple", "Рябь"},
                                         {"dots", "Бегущие точки"},
                                         {"gradient", "Градиент"},
                                         {"meteor", "Метеоры"},
                                         {"juggle", "Жонглирование"},
                                         {"aurora", "Северное сияние"},
                                         {"candy", "Карамель"},
                                         {"twirl", "Завихрение"},
                                         {"sparkle_trails", "Искровые шлейфы"},
                                         {"neon_flow", "Неоновый поток"},
                                         {"calm_sea", "Спокойное море"}}), "✨ Режим подсветки");
    UI_RANGE("LedAutoplayDuration", LedAutoplayDuration, 5, 180, 5, "⏳ Смена режима (сек)");
    UI_SELECT("LedAutoplay", LedAutoplay, (std::initializer_list<UIOption>{{"1", "Автомат"},
                                           {"0", "Вручную"}}), "🔁 Автосмена");
    UI_SELECT("LedColorOrder", LedColorOrder, (std::initializer_list<UIOption>{{"GRB", "WS2811 (RGB)"},
                                               {"RGB", "WS2815 / WS2812 (GRB)"},
                                               {"GBR", "GBR"},
                                               {"RBG", "RBG"},
                                               {"BRG", "BRG"},
                                               {"BGR", "BGR"}}), "🎚️ Порядок цветов ленты");

    UI_POPUP_BEGIN("HLK-LD2410C", "⚙️ Настройка работы RGB ленты и HLK-LD2410C", "⚙️ LED_WS2815 HLK-LD2410C");
        // UI_NUMBER("HLK_LD2410C_RX_PIN", HLK_LD2410C_RX_PIN, "📥 RX-пин ESP32", false);
        // UI_NUMBER("HLK_LD2410C_TX_PIN", HLK_LD2410C_TX_PIN, "📤 TX-пин ESP32", false);
        // UI_NUMBER("HLK_LD2410C_BAUD", HLK_LD2410C_BAUD, "🔌 Скорость UART (baud)", false);
        // UI_NUMBER("HLK_LD2410C_UART_PORT", HLK_LD2410C_UART_PORT, "🧵 UART порт ESP32", false);

        UI_NUMBER("KITCHEN_DISTANCE_NEAR_ENTER_M", KITCHEN_DISTANCE_NEAR_ENTER_M, "📏 Вход в ближнюю зону (м)", true);
        UI_NUMBER("KITCHEN_DISTANCE_NEAR_EXIT_M", KITCHEN_DISTANCE_NEAR_EXIT_M, "📏 Выход из ближней зоны (м)", true);
        UI_NUMBER("KITCHEN_DISTANCE_FAR_ENTER_M", KITCHEN_DISTANCE_FAR_ENTER_M, "📏 Вход в дальнюю зону (м)", true);
        UI_NUMBER("KITCHEN_DISTANCE_FAR_EXIT_M", KITCHEN_DISTANCE_FAR_EXIT_M, "📏 Выход из дальней зоны (м)", true);

        UI_NUMBER("KITCHEN_APPROACH_ANIMATION_MS", KITCHEN_APPROACH_ANIMATION_MS, "⏱️ APPROACH анимация (мс)", false);
        UI_NUMBER("KITCHEN_TRANSITION_WAIT_MS", KITCHEN_TRANSITION_WAIT_MS, "⏱️ Пауза TRANSITION (мс)", false);
        UI_NUMBER("KITCHEN_AMBIENT_DURATION_MS", KITCHEN_AMBIENT_DURATION_MS, "⏱️ Длительность AMBIENT (мс)", false);
        UI_NUMBER("KITCHEN_SENSOR_CONFIRM_MS", KITCHEN_SENSOR_CONFIRM_MS, "⏱️ Подтверждение датчика (мс)", false);
        UI_NUMBER("KITCHEN_SIGNAL_HOLD_MS", KITCHEN_SIGNAL_HOLD_MS, "⏱️ Удержание сигнала (мс)", false);
        UI_NUMBER("KITCHEN_FADE_IN_MS", KITCHEN_FADE_IN_MS, "🌅 Плавное включение (мс)", false);
        UI_NUMBER("KITCHEN_FADE_OUT_MS", KITCHEN_FADE_OUT_MS, "🌙 Плавное выключение (мс)", false);

        UI_RANGE("KITCHEN_WORK_WHITE_BRIGHTNESS", KITCHEN_WORK_WHITE_BRIGHTNESS, 1, 255, 1, "💡 Яркость рабочего света");
        UI_RANGE("KITCHEN_AMBIENT_BRIGHTNESS", KITCHEN_AMBIENT_BRIGHTNESS, 1, 255, 1, "✨ Яркость декоративного света");
        UI_RANGE("KITCHEN_FILTER_WINDOW", KITCHEN_FILTER_WINDOW, 1, KITCHEN_FILTER_WINDOW_MAX, 1, "🧮 Окно сглаживания дистанции");
    UI_POPUP_END();





    // // test: по 1 примеру каждого вида декларативной функции
    // UI_PAGE();

    // UI_IMAGE("TestImage", "/Basin.jpg", "width:100%;height:220;");
    // UI_TEXT("TestText", OverlayFilterState, "💬 Пример UI_TEXT");

    // UI_BUTTON("TestButton", button1, "gray", "🔘 Пример UI_BUTTON");
    // UI_BUTTON_DEFAULT("TestButtonDefault", Pow_WS2815, "gray", "🌈 Пример UI_BUTTON_DEFAULT", 1);

    // UI_CHECKBOX("TestCheckbox", Filtr_Time1, "☑️ Пример UI_CHECKBOX");

    // UI_DISPLAY_INT("TestDisplayInt", RandomVal, "🔢 Пример UI_DISPLAY_INT");
    // UI_DISPLAY_FLOAT("TestDisplayFloat", DS1, "🌡 Пример UI_DISPLAY_FLOAT");
    // UI_DISPLAY_BOOL("TestDisplayBool", Power_Heat, "⚙️ Пример UI_DISPLAY_BOOL", "ON", "OFF");
    // UI_DISPLAY("TestDisplay", Ds18HelpText, "ℹ️ Пример UI_DISPLAY");

    // UI_NUMBER("TestNumberInt", IntInput, "🔢 Пример UI_NUMBER int", false);
    // UI_NUMBER("TestNumberFloat", FloatInput, "🔣 Пример UI_NUMBER float", true);

    // UI_RANGE("TestRange", MotorSpeedSetting, 0, 100, 1, "🎚️ Пример UI_RANGE");
    // UI_RANGE_CB("TestRangeCb", LedBrightness, 10, 255, 1, "🔆 Пример UI_RANGE_CB", onLedBrightnessChange);
    // UI_DUAL_RANGE_KEYS("TestDualRange", RangeMin, RangeMax, "RangeMin", "RangeMax", 10, 40, 1, "🎛️ Пример UI_DUAL_RANGE_KEYS");

    // UI_TIME("TestTime", Timer1, "⏰ Пример UI_TIME");
    // UI_TIMER("TestTimer", "⏱️ Пример UI_TIMER", TimertestON, TimertestOFF, noopTimerCallback);
    // UI_SELECT_DAYS("TestDays", DaysSelect, "📅 Пример UI_SELECT_DAYS");

    // UI_SELECT("TestSelect", LedPattern,
    //           (std::initializer_list<UIOption>{{"rainbow", "Радуга"}, {"pulse", "Пульс"}}),
    //           "📚 Пример UI_SELECT");

    // UI_SELECT_CB("TestSelectCb", SetLamp,
    //              (std::initializer_list<UIOption>{{"off", "Выкл"}, {"on", "Вкл"}, {"auto", "Авто"}}),
    //              "💡 Пример UI_SELECT_CB", onSetLampChange);

    // UI_COLOR("TestColor", LEDColor, "🎨 Пример UI_COLOR");

    // UI_GRAPH_SOURCE("TestGraph", "📈 Пример UI_GRAPH_SOURCE",
    //                 "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:40;width:100%;height:240;"
    //                 "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    //                 "lineWidth:1;pointRadius:3;smooth:false", DS1);

    //       UI_POPUP_BEGIN("Ds18Config", "⚙️ Настройка DS18B20", "⚙️ Настройка DS18B20"); // Уникальный ID popup для DS18B20, чтобы не конфликтовать с другим DataEntry.
    //         UI_DISPLAY("Ds18HelpText", Ds18HelpText, "ℹ️ Подсказка"); // Пояснение по шагам: поиск -> выбор индекса -> назначение.
    //         UI_DISPLAY("Ds18ScanInfo", Ds18ScanInfo, "🔍 Найденные датчики на шине"); // Статус и список адресов после ручного поиска.
    //         UI_BUTTON("ds18ScanButton", Ds18ScanButton, "gray", "🔍 Поиск датчиков на шине"); // Поиск запускается только по нажатию этой кнопки.
    //         UI_DISPLAY("Ds18Sensor0Address", Ds18Sensor0Address, "🏊 Адрес датчика температуры бассейна"); // Текущий адрес, привязанный к DS1.
    //         UI_SELECT_CB("Ds18Sensor0Index", Ds18Sensor0Index, // Выбор индекса найденного датчика для температуры бассейна.
    //         (std::initializer_list<UIOption>{{"-1", "❌ Отвязать датчик"}, {"0", "Индекс 0"}, {"1", "Индекс 1"}, {"2", "Индекс 2"}, {"3", "Индекс 3"}, {"4", "Индекс 4"}, {"5", "Индекс 5"}, {"6", "Индекс 6"}, {"7", "Индекс 7"}, {"8", "Индекс 8"}, {"9", "Индекс 9"}, {"10", "Индекс 10"}, {"11", "Индекс 11"}, {"12", "Индекс 12"}, {"13", "Индекс 13"}, {"14", "Индекс 14"}, {"15", "Индекс 15"}}),
    //                      "➡️ Назначить на температуру бассейна", onDs18Sensor0Select); // Назначаем выбранный адрес в sensor0 и сохраняем в NVS.
    //         UI_DISPLAY("Ds18Sensor1Address", Ds18Sensor1Address, "♨️ Адрес датчика после нагревателя"); // Текущий адрес, привязанный к DS2.
    //         UI_SELECT_CB("Ds18Sensor1Index", Ds18Sensor1Index, // Выбор индекса найденного датчика для температуры после нагревателя.
    //         (std::initializer_list<UIOption>{{"-1", "❌ Отвязать датчик"}, {"0", "Индекс 0"}, {"1", "Индекс 1"}, {"2", "Индекс 2"}, {"3", "Индекс 3"}, {"4", "Индекс 4"}, {"5", "Индекс 5"}, {"6", "Индекс 6"}, {"7", "Индекс 7"}, {"8", "Индекс 8"}, {"9", "Индекс 9"}, {"10", "Индекс 10"}, {"11", "Индекс 11"}, {"12", "Индекс 12"}, {"13", "Индекс 13"}, {"14", "Индекс 14"}, {"15", "Индекс 15"}}),
    //                      "➡️ Назначить на температуру после нагревателя", onDs18Sensor1Select); // Назначаем выбранный адрес в sensor1 и сохраняем в NVS.
    //     UI_POPUP_END();


}