#pragma once // защита от повторного включения файла

#include <Arduino.h> // базовые функции Arduino
#include <WiFi.h> // работа с Wi-Fi
#include <SPIFFS.h> // файловая система SPIFFS
#include <PubSubClient.h> // MQTT клиент
#include <ArduinoJson.h> // работа с JSON

#include "wifi_manager.h" // менеджер Wi-Fi
#include "fs_utils.h" // утилиты файловой системы

inline WiFiClient mqttWifiClient; // WiFi-клиент для MQTT
inline PubSubClient mqttClient(mqttWifiClient); // MQTT-клиент поверх WiFi

inline const char* mqttConfigPath = "/mqtt.json"; // путь к файлу настроек MQTT
inline String mqttHost = "192.168.0.100"; //"broker.emqx.io"; // адрес брокера
inline uint16_t mqttPort = 1883; // порт брокера
inline String mqttUsername = ""; // имя пользователя MQTT
inline String mqttPassword = ""; // пароль MQTT
inline bool mqttEnabled = true; // флаг включения MQTT
inline bool mqttIsConnected = false; // флаг подключения MQTT
inline unsigned long mqttPublishInterval = 10000; // интервал публикации
inline unsigned long mqttLastPublish = 0; // время последней публикации
inline const char* mqttAvailabilityTopic = "home/esp32/availability"; // топик доступности устройства

#if 0 // MQTT Discovery отключен (слишком много кода и не нужен)
inline const char* mqttDiscoveryPrefix = "homeassistant"; // префикс MQTT Discovery
inline bool mqttDiscoveryPending = false; // публикация после первого успешного MQTT loop

inline unsigned long mqttDiscoveryLastAttempt = 0; // время последней попытки discovery
inline const unsigned long mqttDiscoveryInterval = 250; // интервал между пакетами discovery
inline const uint8_t mqttDiscoveryBatchSize = 4; // максимум сущностей за loop
inline const uint8_t mqttDiscoveryMaxRetries = 3; // максимум попыток публикации на сущность
inline bool mqttDiscoveryFullDevicePublished = false; // полный device блок опубликован
inline size_t mqttDiscoveryLastMaxPayload = 0; // максимум payload discovery
inline size_t mqttDiscoveryRetryIndex = 0; // индекс повторной публикации
inline uint8_t mqttDiscoveryRetryCount = 0; // число повторов на сущность
inline bool mqttDiscoveryLegacyCleanupDone = false; // очистка удалённых legacy-сущностей
#endif

inline void publishMqttAvailability(const char* payload, bool retain = true){ // публикация доступности
  if(!mqttClient.connected()) return; // выход если нет подключения
  mqttClient.publish(mqttAvailabilityTopic, payload, retain); // публикация статуса
}

inline unsigned long mqttLastConnectAttempt = 0; // время последней попытки подключения
inline const unsigned long mqttConnectInterval = 5000; // интервал попыток подключения
inline unsigned long mqttLastResolveAttempt = 0; // время последней попытки DNS
inline const unsigned long mqttResolveInterval = 30000; // интервал резолва хоста
inline bool mqttHasResolvedIp = false; // флаг успешного резолва
inline IPAddress mqttResolvedIp; // резолвенный IP адрес
inline String mqttResolvedHostName; // хост для которого выполнен резолв


#if 0 // MQTT Discovery отключен (слишком много кода и не нужен)
enum DiscoveryStage {
  DISCOVERY_NONE,
  DISCOVERY_MAIN_ENTITIES,
  DISCOVERY_DONE
};

inline DiscoveryStage mqttDiscoveryStage = DISCOVERY_NONE; // этап discovery
inline size_t mqttDiscoveryIndex = 0; // индекс публикации сущностей

// mqttDiscovery публикуется один раз после успешного подключения и availability=online.
#endif

extern float DS1; // температура воды
extern float PH; // pH воды
extern float ppmCl; // уровень хлора
extern int corrected_ORP_Eh_mV; // ORP, мВ
extern String OverlayPoolTemp; // температура воды (оверлей)
extern String OverlayHeaterTemp; // температура после нагревателя (оверлей)
extern String OverlayLevelUpper; // верхний уровень (оверлей)
extern String OverlayLevelLower; // нижний уровень (оверлей)
extern String OverlayPh; // pH (оверлей)
extern String OverlayChlorine; // хлор (оверлей)
extern String OverlayFilterState; // состояние фильтрации (строка)
extern String InfoString2; // строка статуса лампы
extern String InfoStringDIN; // строка статуса DIN
extern String ThemeColor; // цвет темы
extern int MotorSpeedSetting; // скорость мотора
extern int RangeMin; // минимум диапазона
extern int RangeMax; // максимум диапазона
extern int IntInput; // целочисленный ввод
extern float FloatInput; // ввод float
extern String Timer1; // время таймера
extern String Comment; // комментарий
extern int RandomVal; // случайное значение
extern int button1; // кнопка 1
extern int button2; // кнопка 2
extern bool Power_H2O2; // состояние насоса NaOCl
extern bool Power_ACO; // состояние насоса ACO
extern bool Power_Heat; // состояние нагрева
extern bool Power_Topping_State; // состояние соленоида долива
extern bool Power_Topping; // ручной долив
extern bool Power_Drain_State; // состояние режима слива (насос включен)
extern bool Power_Drain; // ручной слив
extern bool Power_Filtr; // ручная фильтрация
extern bool Filtr_Time1; // таймер фильтрации №1
extern bool Filtr_Time2; // таймер фильтрации №2
extern bool Filtr_Time3; // таймер фильтрации №3
extern bool Power_Clean; // промывка фильтра
extern bool Clean_Time1; // таймер промывки
extern bool Pow_Ul_light; // ручное освещение
extern bool Ul_light_Time; // таймер уличного освещения
extern bool Activation_Heat; // управление нагревом
extern bool Activation_Water_Level; // контроль уровня воды
extern bool WaterLevelSensorUpper; // датчик верхнего уровня
extern bool WaterLevelSensorLower; // датчик нижнего уровня
extern bool WaterLevelSensorDrain; // датчик уровня для остановки слива
extern float PH_setting; // уставка pH
extern bool PH_Control_ACO; // контроль pH ACO
extern int ACO_Work; // период дозирования ACO
extern float PH1; // нижняя граница pH
extern float PH2; // верхняя граница pH
extern float PH1_CAL; // калибровка pH1
extern float PH2_CAL; // калибровка pH2
extern float Temper_Reference; // температура референса
extern float Temper_PH; // температура компенсации pH
extern int analogValuePH_Comp; // значение АЦП PH
extern bool NaOCl_H2O2_Control; // контроль хлора
extern int ORP_setting; // уставка ORP
extern int H2O2_Work; // период дозирования NaOCl
extern int CalRastvor256mV; // калибровочный раствор
extern int Calibration_ORP_mV; // калибровочный коэффициент ORP
extern int Lumen_Ul; // освещенность на улице
extern bool Pow_WS2815; // RGB лента (ручная)
extern bool WS2815_Time1; // таймер RGB
extern String LEDColor; // цвет RGB
extern String LedColorMode; // режим цвета
extern int LedBrightness; // яркость
extern String LedPattern; // режим подсветки
extern int LedAutoplayDuration; // период автосмены
extern bool LedAutoplay; // автосмена
extern String LedColorOrder; // порядок цветов
extern int LampTimerON; // таймер лампы ON
extern int LampTimerOFF; // таймер лампы OFF
extern int RgbTimerON; // таймер RGB ON
extern int RgbTimerOFF; // таймер RGB OFF
extern int UlLightTimerON; // таймер уличного освещения ON
extern int UlLightTimerOFF; // таймер уличного освещения OFF
extern int Sider_heat; // уставка нагрева
extern bool RoomTemper; // контроль температуры в помещении
extern float RoomTempOn; // граница включения отопления
extern float RoomTempOff; // граница выключения отопления
extern bool Power_Warm_floor_heating; // обогрев пола
extern String SetLamp; // режим лампы
extern String SetRGB; // режим RGB
extern bool Lamp; // состояние лампы
extern bool Lamp_autosvet; // авто-режим лампы
extern bool Power_Time1; // режим таймера лампы
extern bool Pow_WS2815_autosvet; // авто-режим RGB
extern String DaysSelect; // выбранные дни промывки
class UIRegistry; // forward declaration
extern UIRegistry ui; // доступ к UI-реестру таймеров
void syncCleanDaysFromSelection(); // синхронизация дней промывки
String uiValueForId(const String &id); // получение значения UI по id
bool uiApplyValueForId(const String &id, const String &value); // применение значения UI по id

String formatMinutesToTime(uint16_t minutes); // форматирование минут в HH:MM
bool mqttApplyTimerField(const String &fieldId, const String &value); // применить значение таймера
uint16_t mqttTimerOnMinutes(const String &id); // время включения таймера
uint16_t mqttTimerOffMinutes(const String &id); // время выключения таймера
void mqttApplyDaysSelect(const String &value); // применить выбор дней промывки
bool mqttDaysSelectContains(const char* dayToken); // проверить включен ли день в DaysSelect

// MQTT-помощники для таймеров/дней (перенесены сюда, чтобы MQTT-логика была в одном файле)
inline bool mqttApplyTimerField(const String &fieldId, const String &value){
  return ui.updateTimerField(fieldId, value);
}

inline uint16_t mqttTimerOnMinutes(const String &id){
  return ui.timer(id).on;
}

inline uint16_t mqttTimerOffMinutes(const String &id){
  return ui.timer(id).off;
}

inline void mqttApplyDaysSelect(const String &value){
  String next = value;
  next.trim();

  if(next.startsWith("toggle:") || next.startsWith("add:") || next.startsWith("remove:")){
    bool removeDay = next.startsWith("remove:");
    const int prefixLen = next.startsWith("toggle:") ? 7 : (removeDay ? 7 : 4);
    const String day = next.substring(prefixLen);
    String marked = "," + DaysSelect + ",";
    const String token = day + ",";
    const bool hasDay = marked.indexOf(token) >= 0;
    if((next.startsWith("toggle:") && hasDay) || (removeDay && hasDay)){
      marked.replace(token, ",");
    } else if((next.startsWith("toggle:") && !hasDay) || (!removeDay && !next.startsWith("toggle:") && !hasDay)){
      marked += day + ",";
    }
    while(marked.indexOf(",,") >= 0) marked.replace(",,", ",");
    if(marked.startsWith(",")) marked.remove(0, 1);
    if(marked.endsWith(",")) marked.remove(marked.length() - 1);
    next = marked;
  }

  DaysSelect = next;
  syncCleanDaysFromSelection();
  saveValue<String>("DaysSelect", DaysSelect);
}

inline bool mqttDaysSelectContains(const char* dayToken){
  if(dayToken == nullptr || dayToken[0] == '\0') return false;
  const String marked = "," + DaysSelect + ",";
  const String token = String(dayToken) + ",";
  return marked.indexOf(token) >= 0;
}

inline bool mqttApplyDualRangeInt(const String &payload, int &minRef, int &maxRef,
                                  const char* minKey, const char* maxKey){
  int sep = payload.indexOf('-');
  if(sep < 0) return false;
  int nextMin = payload.substring(0, sep).toInt();
  int nextMax = payload.substring(sep + 1).toInt();
  if(nextMin > nextMax){
    int temp = nextMin;
    nextMin = nextMax;
    nextMax = temp;
  }
  minRef = nextMin;
  maxRef = nextMax;
  saveValue<int>(minKey, minRef);
  saveValue<int>(maxKey, maxRef);
  return true;
}

inline bool mqttApplyDualRangeFloat(const String &payload, float &minRef, float &maxRef,
                                    const char* minKey, const char* maxKey){
  int sep = payload.indexOf('-');
  if(sep < 0) return false;
  float nextMin = payload.substring(0, sep).toFloat();
  float nextMax = payload.substring(sep + 1).toFloat();
  if(nextMin > nextMax){
    float temp = nextMin;
    nextMin = nextMax;
    nextMax = temp;
  }
  minRef = nextMin;
  maxRef = nextMax;
  saveValue<float>(minKey, minRef);
  saveValue<float>(maxKey, maxRef);
  return true;
}


inline void publishMqttStateString(const char* topic, const String &value); // forward declaration
inline void publishMqttStateBool(const char* topic, bool value); // forward declaration
inline void loadMqttSettings(); // forward declaration
inline void saveMqttSettings(); // forward declaration
inline void configureMqttServer(); // forward declaration
inline bool resolveMqttHost(); // forward declaration
inline void connectMqtt(); // forward declaration
inline void stopMqttService(); // forward declaration
inline void applyMqttState(); // forward declaration
inline void handleMqttLoop(); // forward declaration

inline bool mqttPayloadIsOn(String payload){ // проверка включения
  payload.trim(); // очистка
  payload.toLowerCase(); // нижний регистр
  return payload == "1" || payload == "on" || payload == "true"; // ON
}

inline bool mqttPayloadIsOff(String payload){ // проверка выключения
  payload.trim(); // очистка
  payload.toLowerCase(); // нижний регистр
  return payload == "0" || payload == "off" || payload == "false"; // OFF
}

inline bool mqttIsAllowedMode(String value){ // проверка режима
  value.trim();
  value.toLowerCase();
  return value == "off" || value == "on" || value == "auto" || value == "timer"; // допустимые режимы
}

inline String mqttNormalizedMode(String value){
  value.trim();
  value.toLowerCase();
  return value;
}

inline void mqttApplyRgbMode(const String &rawMode){
  SetRGB = mqttNormalizedMode(rawMode);
  if(SetRGB == "on"){
    Pow_WS2815 = true;
    Pow_WS2815_autosvet = false;
    WS2815_Time1 = false;
  } else if(SetRGB == "auto"){
    Pow_WS2815 = false;
    Pow_WS2815_autosvet = true;
    WS2815_Time1 = false;
  } else if(SetRGB == "timer"){
    Pow_WS2815 = false;
    Pow_WS2815_autosvet = false;
    WS2815_Time1 = true;
  } else {
    Pow_WS2815 = false;
    Pow_WS2815_autosvet = false;
    WS2815_Time1 = false;
  }

  saveValue<String>("SetRGB", SetRGB);
  saveButtonState("button_WS2815", Pow_WS2815 ? 1 : 0);
  saveValue<int>("Pow_WS2815", Pow_WS2815 ? 1 : 0);
  saveValue<int>("Pow_WS2815_autosvet", Pow_WS2815_autosvet ? 1 : 0);
  saveValue<int>("WS2815_Time1", WS2815_Time1 ? 1 : 0);
}

inline String mqttLedAutoplayState(){
  return LedAutoplay ? "Автоматически" : "Вручную";
}

inline bool mqttParseLedAutoplay(const String &rawValue, bool &parsedValue){
  String value = rawValue;
  value.trim();
  value.toLowerCase();

  if(value == "1" || value == "true" || value == "on" || value == "auto" || value == "автоматически" || value == "автомат"){
    parsedValue = true;
    return true;
  }

  if(value == "0" || value == "false" || value == "off" || value == "manual" || value == "вручную" || value == "ручной"){
    parsedValue = false;
    return true;
  }

  return false;
}

inline bool mqttIsRgbUiTopicId(const String &id){
  return id == "button_WS2815" || id == "WS2815_Time1" || id == "LEDColor" ||
         id == "LedColorMode" || id == "LedBrightness" || id == "LedPattern" ||
         id == "LedAutoplayDuration" || id == "LedColorOrder";
}

inline void handleMqttCommandMessage(char* topic, byte* payload, unsigned int length){ // обработка MQTT команд
  String topicStr(topic); // топик
  String message; // payload строкой
  message.reserve(length); // резерв
  for(unsigned int i = 0; i < length; ++i){
    message += static_cast<char>(payload[i]); // сбор payload
  }
  message.trim(); // очистка

  if(topicStr == "home/esp32/SetRGB/set"){
    if(mqttIsAllowedMode(message)){
      mqttApplyRgbMode(message);
      publishMqttStateString("home/esp32/SetRGB", SetRGB);
      publishMqttStateBool("home/esp32/Pow_WS2815", Pow_WS2815);
      publishMqttStateBool("home/esp32/WS2815_Time1", WS2815_Time1);
    }
    return;
  }

  if(topicStr == "home/esp32/LedAutoplay/set"){
    bool autoplayState = LedAutoplay;
    if(mqttParseLedAutoplay(message, autoplayState)){
      LedAutoplay = autoplayState;
      saveValue<int>("LedAutoplay", LedAutoplay ? 1 : 0);
      publishMqttStateString("home/esp32/LedAutoplay", mqttLedAutoplayState());
    }
    return;
  }

  if(topicStr == "home/esp32/RgbTimer_ON/set"){
    if(mqttApplyTimerField("RgbTimer_ON", message)){
      publishMqttStateString("home/esp32/RgbTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("RgbTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/RgbTimer_OFF/set"){
    if(mqttApplyTimerField("RgbTimer_OFF", message)){
      publishMqttStateString("home/esp32/RgbTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("RgbTimer")));
    }
    return;
  }

  if(topicStr.startsWith("home/esp32/") && topicStr.endsWith("/set")){
    const String basePrefix = "home/esp32/";
    const size_t prefixLen = basePrefix.length();
    const size_t suffixLen = 4; // "/set"
    if(topicStr.length() > prefixLen + suffixLen){
      const String id = topicStr.substring(prefixLen, topicStr.length() - suffixLen);
      if(mqttIsRgbUiTopicId(id)){
        if(uiApplyValueForId(id, message)){
          publishMqttStateString((basePrefix + id).c_str(), uiValueForId(id));
        }
      }
    }
    return;
  }
}


inline void publishMqttStateString(const char* topic, const String &value){ // публикация строкового состояния
  mqttClient.publish(topic, value.c_str()); // публикация значения (retain не используется)
}

inline void publishMqttStateBool(const char* topic, bool value){ // публикация bool состояния
  mqttClient.publish(topic, value ? "1" : "0"); // публикация 1/0
}

inline void publishMqttStateFloat(const char* topic, float value){ // публикация float состояния
  if(isnan(value)){
    mqttClient.publish(topic, "0"); // защита от NaN
    return;
  }
  String payload = String(value); // формирование строки
  mqttClient.publish(topic, payload.c_str()); // публикация значения
}

inline void publishMqttStateInt(const char* topic, int value){ // публикация int состояния
  String payload = String(value); // формирование строки
  mqttClient.publish(topic, payload.c_str()); // публикация значения
}


inline void persistMqttSettings(){ // сохранение настроек MQTT
  if(!spiffsMounted) return; // выход если SPIFFS не смонтирован
  JsonDocument doc; // JSON-документ
  doc["host"] = mqttHost; // адрес брокера
  doc["port"] = mqttPort; // порт брокера
  doc["user"] = mqttUsername; // пользователь
  doc["pass"] = mqttPassword; // пароль
  doc["interval"] = mqttPublishInterval; // интервал
  doc["enabled"] = mqttEnabled; // флаг включения

  File file = SPIFFS.open(mqttConfigPath, FILE_WRITE); // открытие файла
  if(!file) return; // выход если файл не открыт
  serializeJson(doc, file); // запись JSON в файл
  file.close(); // закрытие файла
}

inline void loadMqttSettings(){ // загрузка настроек MQTT
  if(spiffsMounted && SPIFFS.exists(mqttConfigPath)){ // если файл существует
    File file = SPIFFS.open(mqttConfigPath, FILE_READ); // открытие файла
    if(file){ // если файл открыт
      JsonDocument doc; // JSON-документ
      DeserializationError err = deserializeJson(doc, file); // парсинг JSON
      if(!err){ // если без ошибок
        mqttHost = doc["host"] | mqttHost; // адрес брокера
        mqttPort = static_cast<uint16_t>(doc["port"] | mqttPort); // порт
        mqttUsername = doc["user"] | mqttUsername; // пользователь
        mqttPassword = doc["pass"] | mqttPassword; // пароль
        mqttPublishInterval = doc["interval"] | mqttPublishInterval; // интервал
        mqttEnabled = doc["enabled"] | mqttEnabled; // флаг
      }
      file.close(); // закрытие файла
    }
  } else { // если файл отсутствует
    // fallback to legacy preferences if present
    mqttHost = loadValue<String>("mqttHost", mqttHost); // старый host
    mqttPort = static_cast<uint16_t>(loadValue<int>("mqttPort", mqttPort)); // старый порт
    mqttUsername = loadValue<String>("mqttUser", mqttUsername); // старый пользователь
    mqttPassword = loadValue<String>("mqttPass", mqttPassword); // старый пароль
    mqttEnabled = loadValue<int>("mqttEnabled", mqttEnabled ? 1 : 0) == 1; // старый флаг
    persistMqttSettings(); // сохранение в новом формате
  }
}

inline void saveMqttSettings(){ // сохранение настроек MQTT
  persistMqttSettings(); // запись в SPIFFS
}

inline void configureMqttServer(){ // настройка сервера MQTT
 if(mqttResolvedHostName != mqttHost){
    mqttResolvedHostName = mqttHost; // обновление хоста
    mqttHasResolvedIp = false; // сброс IP
    mqttLastResolveAttempt = 0; // сброс таймера резолва
  }
  if(!mqttHasResolvedIp){
    IPAddress ip;
    if(ip.fromString(mqttHost)){
      mqttResolvedIp = ip; // фиксируем IP
      mqttHasResolvedIp = true;
    }
  }
  if(mqttHasResolvedIp){
    mqttClient.setServer(mqttResolvedIp, mqttPort); // установка IP и port
  } else {
    mqttClient.setServer(mqttHost.c_str(), mqttPort); // установка host и port
  }
  mqttClient.setCallback(handleMqttCommandMessage); // обработчик входящих команд
  if(!mqttClient.setBufferSize(4096)){ // увеличиваем буфер для крупных MQTT Discovery payload
    mqttClient.setBufferSize(2048); // fallback, не ниже 2048
  }
  mqttClient.setSocketTimeout(2); // быстрый таймаут сетевых операций
  mqttClient.setKeepAlive(30); // keep-alive для снижения задержек
}

inline bool resolveMqttHost(){ // резолв MQTT хоста без блокировки
  if(mqttHost.length() == 0) return false; // пустой host
  if(mqttHasResolvedIp && mqttResolvedHostName == mqttHost) return true; // уже есть IP
  IPAddress ip;
  if(ip.fromString(mqttHost)){
    mqttResolvedIp = ip;
    mqttHasResolvedIp = true;
    mqttResolvedHostName = mqttHost;
    return true;
  }
  const unsigned long now = millis();
  if(now - mqttLastResolveAttempt < mqttResolveInterval){
    return mqttHasResolvedIp; // ждём следующего окна резолва
  }
  mqttLastResolveAttempt = now;
  if(WiFi.status() != WL_CONNECTED) return mqttHasResolvedIp; // нет Wi-Fi
  if(WiFi.hostByName(mqttHost.c_str(), ip)){
    mqttResolvedIp = ip;
    mqttHasResolvedIp = true;
    mqttResolvedHostName = mqttHost;
    return true;
  }
  return mqttHasResolvedIp; // используем предыдущий IP если был
}

inline void connectMqtt(){ // подключение к MQTT
  if(!mqttEnabled) return; // выход если MQTT выключен
  if(mqttHost.length() == 0) return; // выход если host пустой
  if(WiFi.status() != WL_CONNECTED) return; // выход если нет Wi-Fi
if(!resolveMqttHost()) return; // хост не резолвится
  const unsigned long now = millis(); // текущее время
  if(now - mqttLastConnectAttempt < mqttConnectInterval) return; // защита от частых попыток
  mqttLastConnectAttempt = now; // фиксация попытки подключения

  if(!mqttClient.connected()){ // если не подключены
    String clientId = "espdash-" + WiFi.macAddress(); // уникальный clientId
bool connected = mqttClient.connect( // подключение с логином и LWT
      clientId.c_str(),
      mqttUsername.c_str(),
      mqttPassword.c_str(),
      mqttAvailabilityTopic,
      0,
      true,
      "offline"
    );

    if(connected){ // если подключение успешно
      mqttIsConnected = true; // обновление флага
       publishMqttAvailability("online", true); // публикация доступности
 #if 0 // MQTT Discovery отключен
      mqttDiscoveryStage = DISCOVERY_NONE; // сброс этапа discovery
      mqttDiscoveryIndex = 0; // сброс индекса
      mqttDiscoveryLastAttempt = 0; // сброс таймера
      mqttDiscoveryPending = true; // публикация MQTT Discovery после первого loop
      mqttDiscoveryFullDevicePublished = false;
      mqttDiscoveryLastMaxPayload = 0;
      mqttDiscoveryRetryIndex = 0;
      mqttDiscoveryRetryCount = 0;
      mqttDiscoveryLegacyCleanupDone = false;
      publishHomeAssistantDiscovery(); // попытка публикации сразу после подключения
      #endif

      mqttClient.subscribe("home/esp32/button_WS2815/set", 0);
      mqttClient.subscribe("home/esp32/WS2815_Time1/set", 0);
      mqttClient.subscribe("home/esp32/SetRGB/set", 0);
      mqttClient.subscribe("home/esp32/RgbTimer_ON/set", 0);
      mqttClient.subscribe("home/esp32/RgbTimer_OFF/set", 0);
      mqttClient.subscribe("home/esp32/LEDColor/set", 0);
      mqttClient.subscribe("home/esp32/LedColorMode/set", 0);
      mqttClient.subscribe("home/esp32/LedBrightness/set", 0);
      mqttClient.subscribe("home/esp32/LedPattern/set", 0);
      mqttClient.subscribe("home/esp32/LedAutoplayDuration/set", 0);
      mqttClient.subscribe("home/esp32/LedAutoplay/set", 0);
      mqttClient.subscribe("home/esp32/LedColorOrder/set", 0);
    
    } else { // если не удалось подключиться
      mqttIsConnected = false; // сброс флага
    }
  }
}

inline void stopMqttService(){ // остановка MQTT
    if(mqttClient.connected()) publishMqttAvailability("offline", true); // публикация offline
  mqttClient.disconnect(); // отключение от брокера
  mqttIsConnected = false; // сброс флага подключения
  mqttLastPublish = 0; // сброс таймера публикаций
  mqttLastConnectAttempt = 0; // сброс таймера подключений
  mqttLastResolveAttempt = 0; // сброс таймера резолва

  #if 0 // MQTT Discovery отключен
  mqttDiscoveryPending = false; // сброс discovery
  mqttDiscoveryStage = DISCOVERY_NONE; // сброс этапа
  mqttDiscoveryIndex = 0; // сброс индекса
  mqttDiscoveryLegacyCleanupDone = false;
  #endif
}

inline void applyMqttState(){ // применение состояния MQTT
  stopMqttService(); // остановка текущего подключения
  configureMqttServer(); // настройка сервера
  if(mqttEnabled){ // если MQTT включен
    mqttLastPublish = 0; // сброс таймера
    connectMqtt(); // подключение
  }
}

inline void handleMqttLoop(){ // основной цикл MQTT
  if(!mqttClient.connected()) connectMqtt(); // переподключение
  mqttIsConnected = mqttClient.connected(); // обновление флага

  if(!mqttEnabled || mqttHost.length() == 0){ // если MQTT выключен или host пустой
    if(mqttClient.connected()) stopMqttService(); // отключение
    return; // выход
  }

  mqttClient.loop(); // обработка MQTT

  #if 0 // MQTT Discovery отключен
    if(mqttDiscoveryPending) publishHomeAssistantDiscovery(); // публикация после первого loop
#endif

  unsigned long now = millis(); // текущее время
  if(now - mqttLastPublish >= mqttPublishInterval){ // проверка интервала
    mqttLastPublish = now; // обновление времени
    if(mqttClient.connected()){ // если подключены
      publishMqttStateBool("home/esp32/Pow_WS2815", Pow_WS2815);
      publishMqttStateBool("home/esp32/WS2815_Time1", WS2815_Time1);
      publishMqttStateString("home/esp32/LEDColor", LEDColor);
      publishMqttStateString("home/esp32/LedColorMode", LedColorMode);
      publishMqttStateInt("home/esp32/LedBrightness", LedBrightness);
      publishMqttStateString("home/esp32/LedPattern", LedPattern);
      publishMqttStateInt("home/esp32/LedAutoplayDuration", LedAutoplayDuration);
      publishMqttStateString("home/esp32/LedAutoplay", mqttLedAutoplayState());
      publishMqttStateString("home/esp32/LedColorOrder", LedColorOrder);
      publishMqttStateString("home/esp32/RgbTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("RgbTimer")));
      publishMqttStateString("home/esp32/RgbTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("RgbTimer")));
      publishMqttStateString("home/esp32/SetRGB", SetRGB);
    
    }
  }
}