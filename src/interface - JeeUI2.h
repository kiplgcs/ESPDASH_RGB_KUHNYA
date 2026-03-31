//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"

inline void interface(){ // Декларатиынве функции интерфейса
    // UI_APP("🏊 Управление подсветкой на кухне");
    UI_MENU("🌈 Управление RGB подсветкой");
    UI_HIDDEN("ThemeColor", ThemeColor);

    UI_MENU("🧰 test");




// Управление RGB подсветкой
    UI_PAGE();
    UI_BUTTON_DEFAULT("button_WS2815", Pow_WS2815, "gray", "🌈 Включить / Отключить : RGB ленту WS2815", 1);
    UI_CHECKBOX("WS2815_Time1", WS2815_Time1, "⏲️ Таймер RGB ленты"); //Галочка - активания/деактивация таймера
    UI_SELECT_CB("SetRGB", SetRGB, (std::initializer_list<UIOption>{{"off", "RGB подсветка отключена постоянно"},
                                   {"on", "RGB подсветка включена постоянно"},
                                   {"auto", "Включение по датчику освещенности (<20%)"},
                                   {"timer", "Включение по таймеру"}}), "🎛️ Режим управления RGB подсветкой", onSetRgbChange);
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





    // test: по 1 примеру каждого вида декларативной функции
    UI_PAGE();

    UI_IMAGE("TestImage", "/Basin.jpg", "width:100%;height:220;");
    UI_TEXT("TestText", OverlayFilterState, "💬 Пример UI_TEXT");

    UI_BUTTON("TestButton", button1, "gray", "🔘 Пример UI_BUTTON");
    UI_BUTTON_DEFAULT("TestButtonDefault", Pow_WS2815, "gray", "🌈 Пример UI_BUTTON_DEFAULT", 1);

    UI_CHECKBOX("TestCheckbox", Filtr_Time1, "☑️ Пример UI_CHECKBOX");

    UI_DISPLAY_INT("TestDisplayInt", RandomVal, "🔢 Пример UI_DISPLAY_INT");
    UI_DISPLAY_FLOAT("TestDisplayFloat", DS1, "🌡 Пример UI_DISPLAY_FLOAT");
    UI_DISPLAY_BOOL("TestDisplayBool", Power_Heat, "⚙️ Пример UI_DISPLAY_BOOL", "ON", "OFF");
    UI_DISPLAY("TestDisplay", Ds18HelpText, "ℹ️ Пример UI_DISPLAY");

    UI_NUMBER("TestNumberInt", IntInput, "🔢 Пример UI_NUMBER int", false);
    UI_NUMBER("TestNumberFloat", FloatInput, "🔣 Пример UI_NUMBER float", true);

    UI_RANGE("TestRange", MotorSpeedSetting, 0, 100, 1, "🎚️ Пример UI_RANGE");
    UI_RANGE_CB("TestRangeCb", LedBrightness, 10, 255, 1, "🔆 Пример UI_RANGE_CB", onLedBrightnessChange);
    UI_DUAL_RANGE_KEYS("TestDualRange", RangeMin, RangeMax, "RangeMin", "RangeMax", 10, 40, 1, "🎛️ Пример UI_DUAL_RANGE_KEYS");

    UI_TIME("TestTime", Timer1, "⏰ Пример UI_TIME");
    UI_TIMER("TestTimer", "⏱️ Пример UI_TIMER", TimertestON, TimertestOFF, noopTimerCallback);
    UI_SELECT_DAYS("TestDays", DaysSelect, "📅 Пример UI_SELECT_DAYS");

    UI_SELECT("TestSelect", LedPattern,
              (std::initializer_list<UIOption>{{"rainbow", "Радуга"}, {"pulse", "Пульс"}}),
              "📚 Пример UI_SELECT");

    UI_SELECT_CB("TestSelectCb", SetLamp,
                 (std::initializer_list<UIOption>{{"off", "Выкл"}, {"on", "Вкл"}, {"auto", "Авто"}}),
                 "💡 Пример UI_SELECT_CB", onSetLampChange);

    UI_COLOR("TestColor", LEDColor, "🎨 Пример UI_COLOR");

    UI_GRAPH_SOURCE("TestGraph", "📈 Пример UI_GRAPH_SOURCE",
                    "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:40;width:100%;height:240;"
                    "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
                    "lineWidth:1;pointRadius:3;smooth:false", DS1);

    UI_POPUP_BEGIN("TestPopup", "🧪 Пример UI_POPUP", "🪟 Открыть popup");
        UI_TEXT("TestPopupText", OverlayPoolTemp, "Текст внутри popup");
    UI_POPUP_END();
}