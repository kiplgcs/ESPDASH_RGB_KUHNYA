//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"
#include "LD2420.h" //#include "HLK-LD2410C.h"

inline void interface(){ // Декларатиынве функции интерфейса
    // UI_APP("🏊 Управление подсветкой на кухне");
    UI_MENU("🌈 Управление RGB подсветкой");

    UI_MENU("⚙️ Все возможное управление и все возможные параметры LD2420");

    UI_HIDDEN("ThemeColor", ThemeColor);

    // UI_MENU("🧰 test");


        // UI_POPUP_BEGIN("LD2420", "⚙️ Настройка работы RGB ленты и LD2420", "⚙️ LED_WS2815 LD2420"); 

        // UI_POPUP_END();

// Управление RGB подсветкой
    UI_PAGE();
    UI_BUTTON_DEFAULT("button_WS2815", Pow_WS2815, "gray", "🌈 Включить / Отключить : RGB ленту WS2815", 1);
    UI_CHECKBOX("WS2815_Time1", WS2815_Time1, "⏲️ Таймер RGB ленты"); //Галочка - активания/деактивация таймера
    UI_SELECT_CB("SetRGB", SetRGB, (std::initializer_list<UIOption>{{"off", "RGB подсветка отключена постоянно"},
                                   {"on", "RGB подсветка включена постоянно"},
                                      {"auto", "Автоматически по датчику присутствия"},
                                   {"timer", "По таймеру"}}), "🎛️ Режим управления RGB подсветкой", onSetRgbChange);
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

      UI_POPUP_BEGIN("LD2420", "⚙️ Настройка работы RGB ленты", "⚙️ Настройка работы RGB ленты");
        
        UI_NUMBER("KITCHEN_DISTANCE_NEAR_ENTER_M", KITCHEN_DISTANCE_NEAR_ENTER_M, "📏 Вход в ближнюю зону (м)", true);
        UI_NUMBER("KITCHEN_NEAR_HYSTERESIS_M", KITCHEN_NEAR_HYSTERESIS_M, "↕️ Гистерезис входа-выхода (м) [ближняя зона, 0..1]", true);
        UI_NUMBER("KITCHEN_DISTANCE_FAR_ENTER_M", KITCHEN_DISTANCE_FAR_ENTER_M, "📏 Вход в дальнюю зону (м)", true);
        UI_NUMBER("KITCHEN_FAR_HYSTERESIS_M", KITCHEN_FAR_HYSTERESIS_M, "↕️ Гистерезис входа-выхода (м) [дальняя зона, 0..1]", true);
        UI_NUMBER("KITCHEN_TRANSITION_WAIT_MS", KITCHEN_TRANSITION_WAIT_MS, "⏱️ Задержка перехода Ближняя → Дальняя (мс)", false);
        UI_NUMBER("KITCHEN_LIGHTS_OFF_DELAY_MS", KITCHEN_LIGHTS_OFF_DELAY_MS, "⏱️ Задержка отключения подсветки (мс)", false);
         UI_NUMBER("KITCHEN_LED_COUNT", KITCHEN_LED_COUNT, "🔢 Количество светодиодов в ленте (1..1000)", false);
        UI_SELECT("KITCHEN_NEAR_ENTRY_EFFECT", KITCHEN_NEAR_ENTRY_EFFECT, (std::initializer_list<UIOption>{
            {"edge_white", "Белый от краев к центру"},
            {"rainbow", "Радуга"},
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
            {"aurora", "Северное сияние"}}), "✨ Эффект при переходе Дальняя → Ближняя");
        UI_RANGE("KITCHEN_WORK_WHITE_BRIGHTNESS", KITCHEN_WORK_WHITE_BRIGHTNESS, 1, 255, 1, "💡 Яркость рабочего света");
    UI_POPUP_END();





    //⚙️ Все возможное управление и все возможные параметры LD2420
    UI_PAGE();

    UI_POPUP_BEGIN("LD2420", "⚙️ Настройка работы  LD2420", "⚙️ Настройка работы LD2420");

    UI_GRAPH_SOURCE("LD2420DistanceGraph", "📡 График расстояния LD2420",
                    "value:LD2420_DISTANCE_GRAPH_M;updatePeriod_of_Time:60;updateStep:1;maxPoints:80;width:100%;height:240;"
                    "xLabel:Time;yLabel:Distance (m);pointColor:#6b66ff;lineColor:#43d17a;"
                    "lineWidth:2;pointRadius:2;smooth:true", LD2420_DISTANCE_GRAPH_M);

        UI_NUMBER("LD2420_RX_PIN", LD2420_RX_PIN, "📥 RX pin ESP32 (данные от датчика TX/OT2)", false);
        UI_NUMBER("LD2420_TX_PIN", LD2420_TX_PIN, "📤 TX pin ESP32 (команды в RX датчика)", false);
        UI_NUMBER("LD2420_UART_PORT", LD2420_UART_PORT, "🔌 UART порт ESP32 (0..2)", false);
        UI_NUMBER("LD2420_BAUD", LD2420_BAUD, "🧮 Baud LD2420 (обычно 115200)", false);
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