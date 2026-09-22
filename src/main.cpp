// main.cpp — UI ESPDASH-PRO проект для ESP32 в Visual Studio Code
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_system.h>
#include "NPT_Time.h"

#include "wifi_manager.h"        // Логика Wi-Fi и сохранение параметров
#include "fs_utils.h"    // Функции для работы с файловой системой SPIFFS
#include "boiler_relay.h" // Импульсное реле, подключённое параллельно кнопке котла
#include "graph.h"       // Функции для графиков и визуализации
#include "web.h"         // Функции работы Web-панели (ESP-DASH)
#include "ui - JeeUI2.h"         // Построитель UI в стиле JeeUI2
#include "interface - JeeUI2.h"  // Описание веб-интерфейса
#include "settings_MQTT.h"       // Настройки и работа с MQTT
#include "WebUpdate.h"    // OTA-обновление через AsyncOTA

#include "ds18.h"


#include "LED_WS2815.h"
#include "LED_WS2815_sensor.h"
#include "LD_2420.h"

bool ReadRelayArray[16] = {false}; // Заглушка: Modbus удален, состояния реле всегда неактивны.
bool ReadInputArray[16] = {false}; // Заглушка: Modbus удален, состояния входов всегда неактивны.



constexpr BaseType_t kWebMqttCore = 0; // Назначаем ядро 0 для общего сетевого контура WiFi+WEB+MQTT.
constexpr BaseType_t kMainLogicCore = 1; // Назначаем ядро 1 для всей основной прикладной логики.
TaskHandle_t webMqttTaskHandle = nullptr; // Сохраняем дескриптор задачи Web+MQTT для диагностики и контроля.
TaskHandle_t lightingTaskHandle = nullptr; // Сохраняем дескриптор выделенной задачи освещения и опроса радара.
bool webSelfTestDone = false; // Однократная локальная проверка, что HTTP-сокет действительно принимает соединения.

void runWebServerSelfTest(){
  if(webSelfTestDone || WiFi.status() != WL_CONNECTED || millis() < 10000UL) return;
  webSelfTestDone = true;
  WiFiClient probe;
  probe.setTimeout(2000);
  if(!probe.connect(WiFi.localIP(), 80, 2000)){
    Serial.println("[WEB] Self-test failed: TCP port 80 does not accept connections");
    return;
  }
  probe.print("GET /stats HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
  const uint32_t deadline = millis() + 2000UL;
  while(!probe.available() && probe.connected() && static_cast<int32_t>(deadline - millis()) > 0){
    delay(10);
  }
  String statusLine = probe.available() ? probe.readStringUntil('\n') : String("NO HTTP RESPONSE");
  statusLine.trim();
  Serial.println("[WEB] Self-test: " + statusLine);
  probe.stop();
}

void webMqttTask(void * /*parameter*/) { // Определяем задачу FreeRTOS, которая будет крутить только WiFi+WEB+MQTT.
  for (;;) { // Запускаем бесконечный цикл, потому что задача должна работать весь аптайм контроллера.
    wifiModuleLoop(); // Выполняем обслуживание WiFi и WEB-интерфейса в сетевом контуре ядра 0.
    handleMqttLoop(); // Выполняем обслуживание MQTT в том же сетевом контуре ядра 0.
    vTaskDelay(pdMS_TO_TICKS(2)); // Делаем короткую паузу, чтобы отдать CPU другим задачам FreeRTOS.
  } // Закрываем цикл непрерывного обслуживания сетевого контура.
} // Закрываем функцию задачи, закрепляемой за ядром 0.

void lightingTask(void * /*parameter*/) { // Выделяем отдельный детерминированный цикл для радара и WS2815, чтобы убрать рывки анимации.
  const TickType_t periodTicks = pdMS_TO_TICKS(8); // Фиксированный шаг обновления ~125 Гц для плавной анимации ленты.
  TickType_t lastWakeTime = xTaskGetTickCount(); // Инициализируем опорное время для vTaskDelayUntil.
  for (;;) { // Держим задачу активной постоянно, пока работает контроллер.
    loopBoilerRelay(); // Завершаем импульс реле точно по времени независимо от тяжёлого WEB/MQTT.
    loop_LD2420(); // Быстрый опрос LD2420 в отдельной задаче, чтобы парсинг UART не тормозил общий loop().
    loop_LED_WS2815_sensor(); // Логика автоматики света по зонам рядом с датчиком.
    loop_WS2815(); // Отрисовка кадра ленты в стабильном тайминге.
    vTaskDelayUntil(&lastWakeTime, periodTicks); // Стабилизируем период запуска цикла независимо от остального кода.
  } // Завершаем бесконечный цикл задачи освещения.
} // Закрываем функцию выделенной задачи света/датчика.


// ---------- NTP (синхронизация времени) ----------
WiFiUDP ntpUDP;


/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);

  delay(100);
  Serial.println("\n[BOOT] ESPDASH starting...");
  const char *resetReasonText = []() -> const char * {
    switch (esp_reset_reason()) {
      case ESP_RST_POWERON:
        return "Power On";
      case ESP_RST_EXT:
        return "External Reset";
      case ESP_RST_SW:
        return "Software Reset";
      case ESP_RST_PANIC:
        return "Panic";
      case ESP_RST_INT_WDT:
        return "Interrupt Watchdog";
      case ESP_RST_TASK_WDT:
        return "Task Watchdog";
      case ESP_RST_WDT:
        return "Other Watchdog";
      case ESP_RST_DEEPSLEEP:
        return "Deep Sleep";
      case ESP_RST_BROWNOUT:
        return "Brownout";
      case ESP_RST_SDIO:
        return "SDIO";
      default:
        return "Unknown";
    }
  }();
  Serial.printf("[BOOT] Reset reason: %s\n", resetReasonText);
  Serial.printf("[BOOT] Chip model: %s | Cores: %u | Revision: %u\n",
                ESP.getChipModel(), ESP.getChipCores(), ESP.getChipRevision());
  Serial.printf("[BOOT] Flash: %u bytes | PSRAM detected: %s | PSRAM size: %u bytes | free: %u bytes\n",
                ESP.getFlashChipSize(), psramFound() ? "YES" : "NO",
                ESP.getPsramSize(), ESP.getFreePsram());

  Serial.println("[BOOT] Initializing boiler relay in safe state...");
  setupBoilerRelay();

  // Подключение к Wi-Fi с использованием сохранённых данных и кнопок
  StoredAPSSID = loadValue<String>("apSSID", String(apSSID));
  StoredAPPASS = loadValue<String>("apPASS", String(apPASS));
  button1 = loadButtonState("button1", 0);
  button2 = loadButtonState("button2", 0);

    Serial.println("[BOOT] Initializing Wi-Fi...");
  initWiFiModule();

  // Инициализация файловой системы SPIFFS
  Serial.println("[BOOT] Initializing filesystem...");
  initFileSystem();

  // Графики читаем до построения большого UI и запуска сетевых задач: так
  // парсер SPIFFS получает достаточно непрерывной RAM и не конкурирует с HTTP.
  Serial.println("[BOOT] Loading graphs...");
  loadGraph();
  Serial.println("[BOOT] Graphs loaded");

  // Загрузка параметра jpg из файловой системы (по умолчанию 1)
  jpg = loadValue<int>("jpg", 1);

  // Инициализация времени из сохраненного значения (если есть)
    Serial.println("[BOOT] Loading persisted time...");
  loadBaseEpochFromStorage();

  gmtOffset_correct = loadValue<int>("gmtOffset", 3);
  gmtOffset_correct = normalizeGmtOffset(gmtOffset_correct);
  Saved_gmtOffset_correct = gmtOffset_correct;


  // Загрузка и применение MQTT параметров
  Serial.println("[BOOT] Loading MQTT settings...");
  loadMqttSettings();
  Serial.println("[BOOT] Applying MQTT state...");
  applyMqttState();

  // Загрузка настроек доступа к веб-интерфейсу
  authUsername = loadValue<String>("authUser", "");
  authPassword = loadValue<String>("authPass", "");
  adminUsername = loadValue<String>("adminUser", "");
  adminPassword = loadValue<String>("adminPass", "");

  beginWebUpdate();  // Запуск OTA-обновлений на порту 8080


  // setupDs18Bindings(); // Загружаем и применяем связанные настройки DS18B20.

  Serial.println("[BOOT] Building WEB interface registry...");
  interface(); // Первичная сборка UI нужна для загрузки/сохранения связанных значений из EEPROM
  dashInterfaceInitialized = true; // Критический фикс: запрещаем повторный вызов interface() в dash.begin(), иначе вкладки/элементы дублируются
  Serial.println("[BOOT] Starting HTTP server on port 80...");
  dash.begin(); // WEB должен стать доступен до запуска необязательных датчиков и LED.
  Serial.println("[BOOT] HTTP server started");

  xTaskCreatePinnedToCore(webMqttTask, "WebMqttCoreTask", 8192, nullptr, 2, &webMqttTaskHandle, kWebMqttCore); // Сетевой контур запускаем немедленно, чтобы периферия не могла задержать WEB/WiFi.
  Serial.println("[BOOT] WiFi+WEB+MQTT service task started");
  
  Serial.println("[BOOT] Synchronizing UI settings...");
  syncCleanDaysFromSelection();

  ColorRGB = LedColorMode.equalsIgnoreCase("manual");

  if(LedAutoplayDuration < 1) LedAutoplayDuration = 1;

  new_bright = LedBrightness;

  Serial.println("[BOOT] Initializing WS2815 strip...");
  setup_WS2815();
  Serial.println("[BOOT] WS2815 strip initialized");
  Serial.println("[BOOT] Initializing lighting sensor logic...");
  setup_LED_WS2815_sensor();
  Serial.println("[BOOT] Lighting sensor logic initialized");

  Serial.println("[BOOT] Initializing LD2420...");
  setup_LD2420();
  Serial.println("[BOOT] LD2420 initialized");



  xTaskCreatePinnedToCore(lightingTask, "LightingCoreTask", 8192, nullptr, 3, &lightingTaskHandle, kMainLogicCore); // Выделяем отдельную приоритетную задачу света и датчика на ядре 1.
  Serial.printf("[BOOT] CORE MAP -> WiFi+WEB+MQTT: %d | Main logic(loop): %d\n", kWebMqttCore, kMainLogicCore); // Печатаем явную карту ядер, чтобы одним взглядом видеть распределение.



Serial.printf(
  "Heap Free: %u | Heap Min: %u | Max Block: %u | PSRAM Free: %u\n",
  ESP.getFreeHeap(),
  ESP.getMinFreeHeap(),
  ESP.getMaxAllocHeap(),
  ESP.getFreePsram()
);

}



/* ---------- Loop ---------- */
void loop() {
  runWebServerSelfTest(); // Проверяем HTTP после получения STA-адреса, не полагаясь на маршрут Windows/VPN.

  static bool mainCoreLogged = false; // Запоминаем, что диагностический лог по ядру loop уже был напечатан.
  if (!mainCoreLogged) { // Проверяем, что стартовый лог по ядру основной логики еще не выводился.
    Serial.printf("[BOOT] loop() main logic confirmed on core %d\n", xPortGetCoreID()); // Печатаем фактическое ядро, где реально выполняется основной loop.
    mainCoreLogged = true; // Фиксируем флаг, чтобы не засорять UART повторяющимися логами.
  } // Закрываем одноразовый блок диагностики ядра основной логики.


  // handleDs18ScanButton(); // Обрабатываем кнопку поиска датчиков DS18B20.

  // Обновление времени через NTP/память
  NPT_Time(period_get_NPT_Time);
  CurrentTime = getCurrentDateTime();   // Получение текущего времени




  // Генерация случайных значений для демонстрации
  RandomVal = random(0,50);                     // Случайное число
  Speed = random(150, 350) / 10.0f;            // Случайная скорость
  // Temperatura = random(220, 320) / 10.0f;      // Случайная температура
  Temperatura = DS1;      // Температура в бассейне
  
  InfoString = "Random value is " + String(RandomVal) + " at " + CurrentTime + "Pow_WS2815 = " + String(Pow_WS2815);
  InfoStringRS485Model = "Modbus disabled";
  InfoStringDIN = "Контроль Modbus отключен в этой версии проекта.";
  InfoString1 = /*"Speed " + String(Speed, 1) + " / Temp " + String(Temperatura, 1)*/ + " button1 = " + String(button1)
              + " RangeSlider = " + String(RangeMin) + " / " + String(RangeMax);
  
   
  OverlayPoolTemp = "🌡 Бассейн: " + formatTemperatureString(DS1, DS1Available);
  OverlayHeaterTemp = "♨️ После нагревателя: " + formatTemperatureString(DS2, DS2Available);
  OverlayLevelUpper = String("🛟 Верхний уровень: ") + (WaterLevelSensorUpper ? "Активен" : "Нет уровня");
  OverlayLevelLower = String("🛟 Нижний уровень: ") + (WaterLevelSensorLower ? "Активен" : "Нет уровня");
  OverlayPh = "🧪 pH: отключено";
  OverlayChlorine = "🧴 Cl: отключено";
  OverlayFilterState = "🧽 Фильтр: отключено";
  jpg = 1;

//   // ---------- Рандомный цвет LED ----------
//   // LEDColor = "#" + String((random(0x1000000) | 0x1000000), HEX).substring(1);
//   const char hexDigits[] = "0123456789ABCDEF";
//   String color = "#";
//   int colorValues[] = { random(0,256), random(0,256), random(0,256) };
//   for(int i=0; i<3; i++){
//     color += hexDigits[(colorValues[i] >> 4) & 0xF];
//     color += hexDigits[colorValues[i] & 0xF];
//   }
//   LEDColor = color;

//   // ---------- Рандомный выбор режима ----------
//   // ModeSelect = (String[]){"Normal", "Eco", "Turbo"}[random(0, 3)];
//   const char* modes[] = {"Normal","Eco","Turbo"};
//   ModeSelect = String(modes[random(0,3)]);

  // ---------- Рандомный выбор дней недели ----------
  // DaysSelect = ({ String out=""; String d[7]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}; 
  //   for(int i=0;i<7;i++) if(random(0,2)) out += (out==""?"":",") + d[i]; out == "" ? "Mon" : out; });
  // const char* weekDays[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  // String selectedDays;
  // for(int i=0; i<7; i++){
  //   if(random(0,2)){
  //     if(selectedDays.length()) selectedDays += ",";
  //     selectedDays += weekDays[i];
  //   }
  // }
  // if(selectedDays.length() == 0){
  //   selectedDays = weekDays[random(0,7)]; // хотя бы один день
  // }
  // DaysSelect = selectedDays;

//   // ---------- Рандомные значения для элементов ----------
//   IntInput = random(0,100);
//   FloatInput = random(0,100) / 10.0f;

// // ---------- Рандомные значения времени ----------
//   // Timer1 = String((random(0,24) < 10 ? "0" : "")) + String(random(0,24)) + ":" + String((random(0,60) < 10 ? "0" : "")) + String(random(0,60));
//   int hour = random(0,24);
//   int minute = random(0,60);
//   Timer1 = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);

  //  ---------- Рандомный текст ----------
  static const char* const comments[] = {
 "💧 Вода чистая",
    "🧪 pH в норме",
    "🌡 Температура стабильна",
    "🧹 Фильтр промыт",
    "✅ Все системы в порядке",
    "⚠️ Низкий уровень воды",
    "🔆 Подсветка включена",
    "🕒 Работа по таймеру",
    "Все ок 👍",
    "Работает как зверь 🦾",
    "Сегодня повезёт! ✨",
    "Турбо-режим активирован 🚀",
    "Отличный выбор 😉",
    "Готово! 🔧",
    "Запускаю магию 🪄",
    "Миссия выполнена ✅",
    "Стабильный поток 🌊",
    "Идеальный баланс ⚖️",
    "Система бодра 🧠",
    "Проверка пройдена 🧾",
    "Свет сияет 💡",
    "Насос в тонусе 🏋️",
    "Фильтр крутится 🔄",
    "Датчики на посту 📡",
    "Режим комфорта 😌",
    "Температура в цель 🎯",
    "Плавный ход 🛶",
    "Лёгкий бриз 🌬️",
    "Чистота на уровне 🧽",
    "Скорость стабильна 🧭",
    "Пузырьки счастья 🫧",
    "Охлаждение ровное ❄️",
    "Прогрев идёт 🔥",
    "Свежесть гарантирована 🍃",
    "Система на чеку 🛡️",
    "Таймеры синхронизированы ⏱️",
    "Запас мощности есть ⚡",
    "Сетевое соединение крепкое 📶",
    "Путь открыт 🛤️",
    "Отзыв отличный ⭐",
    "Ровные показатели 📈",
    "Мягкий режим 🧸",
    "Пики сглажены 🪂",
    "Зелёный свет 🟢",
    "Сигнал принят ✅",
    "Отличная циркуляция 🔁",
    "Все параметры в норме 🧩",
    "Безопасность активна 🧯",
    "Экономичный режим 💸",
    "Мощность оптимальна 🧰",
    "Стабильная работа 🧊",
    "Без перебоев 🧯",
    "Уровень точный 🎛️",
    "График красивый 📊",
    "Всё идёт по плану 🗺️",
    "Сервисный режим ✅",
    "Уверенный старт 🏁",
    "Режим тишины 🤫",
    "Комфортно и тихо 🪶",
    "Модуляция плавная 🎚️",
    "Капля к капле 💦",
    "Бережный режим 🤍",
    "Держим курс 🧭",
    "Система улыбается 😄",
    "Дышим ровно 🫁",
    "Всё под контролем 🎮"
  };
    Comment = comments[random(0, 66)];
  // Comment = "Random note " + String(random(1000,9999));

//   MotorSpeedSetting = random(1,50);
//   RangeMin = random(0,49);
//   RangeMax = random(49,100);
//   button1 = random(0,2);
//   button2 = random(0,2);




  // ---------- Добавление точек в графики с интервалом ----------
  addGraphPoint(CurrentTime, RandomVal); // Обновление графика RandomVal
  for(auto &entry : graphValueProviders){
    addSeriesPoint(entry.first, CurrentTime, entry.second()); // Обновление всех графиков
  }


  const uint32_t nowMs = millis(); // Время для оценки «свежести» кадров LD2420.
  const bool ldFresh = (LD2420_LAST_FRAME_AT_MS > 0) && ((nowMs - static_cast<uint32_t>(LD2420_LAST_FRAME_AT_MS)) <= 1500UL); // Учитываем только свежие данные LD2420.
  if (ldFresh && LD2420_DISTANCE_M > 0.01f) {
    RadarAverageValidSensors = 1;
    RadarAverageDistanceM = LD2420_DISTANCE_M;
    RadarAverageAgeMs = LD2420_LAST_DISTANCE_AGE_MS;
  } else {
    RadarAverageValidSensors = 0;
    RadarAverageDistanceM = 0.0f;
    RadarAverageAgeMs = 0;
  }
    RadarCompactLine = "📏 LD2420: " + String(LD2420_DISTANCE_M, 2) + " м"
                   + "  •  🎯 Цель: " + String(LD2420_HAS_TARGET ? "ДА" : "НЕТ")
                   + "  •  ⌛ Age: " + String(LD2420_LAST_DISTANCE_AGE_MS) + " ms";


}
