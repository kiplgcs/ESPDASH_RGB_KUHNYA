// Подключаем необходимые библиотеки
#pragma once

#include <WiFiUdp.h> // Библиотека для работы с UDP-протоколом
#include <NTPClient.h> // Библиотека для работы с NTP-клиентом
#include <Wire.h>
#include <time.h>
#include <stdlib.h>
#include "wifi_manager.h"
//#include <RTClib.h>
//#include <TimeLib.h>
//#include <Adafruit_I2CDevice.h

// Параметры NTP-сервера
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.google.com";
//int gmtOffset_correct = 3; //GMT+3:00, Москва/Краснодар
const int daylightOffset = 0; // Смещение летнего времени (0 - отключено)

inline int period_get_NPT_Time = 600000; // Период NTP-синхронизации (мс), при ошибках временно уменьшается

// Внешние зависимости из других модулей
extern WiFiUDP ntpUDP;
extern int Saved_gmtOffset_correct;
extern int gmtOffset_correct;

// Переменные времени по умолчанию
inline int npt_seconds, seconds;
inline int npt_minutes, minutes;
inline int npt_hours, hours;
inline int npt_Day, Day;        //День месяца
inline int npt_Month, Month;      //Месяц
inline int npt_Year, Year;       //Год

inline int npt_DayOfWeek=1, DayOfWeek=1;  //День недели - Значение от 1 (Понедельник) до 7 (воскресенье)
const char* daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};


inline const char *kLastEpochKey = "lastEpoch"; // Ключ в NVS для хранения последнего известного времени (epoch)
inline time_t baseEpoch = 0;                    // Базовый epoch, от которого ведем отсчет
inline unsigned long baseEpochMillis = 0;       // millis() в момент установки baseEpoch
inline bool baseEpochReady = false;             // Флаг, что базовое время валидно
inline time_t lastSavedEpoch = 0;               // Последний epoch, сохраненный в NVS

inline int normalizeGmtOffset(int offset) {
  if (offset < -12) return -12;
  if (offset > 14) return 14;
  return offset;
}


bool isValidDateTime(int year, int month, int day, int hour, int minute, int second) {
  if (year < 2024 || year > 2040) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > 31) return false;
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;
  return true;
}

// Собираем epoch из компонентов даты/времени, чтобы работать единым форматом времени
time_t buildEpoch(int year, int month, int day, int hour, int minute, int second) {
  struct tm timeInfo = {};
  timeInfo.tm_year = year - 1900;
  timeInfo.tm_mon = month - 1;
  timeInfo.tm_mday = day;
  timeInfo.tm_hour = hour;
  timeInfo.tm_min = minute;
  timeInfo.tm_sec = second;
  return mktime(&timeInfo);
}
// Раскладываем epoch обратно в компоненты даты/времени для UI/логики
void syncComponentsFromEpoch(time_t epoch) {
  struct tm *tmInfo = localtime(&epoch);
  if (!tmInfo) return;
  seconds = tmInfo->tm_sec;
  minutes = tmInfo->tm_min;
  hours = tmInfo->tm_hour;
  Day = tmInfo->tm_mday;
  Month = tmInfo->tm_mon + 1;
  Year = tmInfo->tm_year + 1900;
  int normalizedDayOfWeek = (tmInfo->tm_wday + 6) % 7 + 1;
  DayOfWeek = normalizedDayOfWeek;
}
// Устанавливаем базовое время: фиксируем epoch, стартовую метку millis и сохраняем в NVS
// Сохраняем не чаще раза в минуту, чтобы не изнашивать память
void setBaseEpoch(time_t epoch) {
  if (epoch <= 0) return;
  baseEpoch = epoch;
  baseEpochMillis = millis();
  baseEpochReady = true;
  syncComponentsFromEpoch(epoch);
  if (lastSavedEpoch == 0 || epoch > lastSavedEpoch + 60 || epoch + 60 < lastSavedEpoch) {
    saveValue<int>(kLastEpochKey, static_cast<int>(epoch));
    lastSavedEpoch = epoch;
  }
}
// Загружаем последнее известное время из NVS (если нет интернета)
bool loadBaseEpochFromStorage() {
  int storedEpoch = loadValue<int>(kLastEpochKey, 0);
  if (storedEpoch > 0) {
    setBaseEpoch(static_cast<time_t>(storedEpoch));
    return true;
  }
  return false;
}
// Получаем текущее время: baseEpoch + прошедшие секунды от millis
time_t getCurrentEpoch() {
  if (!baseEpochReady) return 0;
  unsigned long elapsedSeconds = (millis() - baseEpochMillis) / 1000;
  return baseEpoch + elapsedSeconds;
}
// Вспомогательная функция для расписаний (часы/минуты из единого источника времени)
bool getCurrentHourMinute(int &currentHour, int &currentMinute) {
  time_t epoch = getCurrentEpoch();
  if (epoch <= 0) return false;
  struct tm *tmInfo = localtime(&epoch);
  if (!tmInfo) return false;
  currentHour = tmInfo->tm_hour;
  currentMinute = tmInfo->tm_min;
  return true;
}
// Форматирование epoch в строку для веб-интерфейса
String formatEpoch(time_t epoch) {
  if (epoch <= 0) return String("--");
  struct tm *tmInfo = localtime(&epoch);
  if (!tmInfo) return String("--");
  char buf[20];
  strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", tmInfo);
  return String(buf);
}
// Текущее время в строковом виде для UI
String getCurrentDateTime() {
  return formatEpoch(getCurrentEpoch());
}


// Функция для провирки наличия Интернета
bool checkInternetAvailability() {

  if (WiFi.status() != WL_CONNECTED) {return false;}  // WiFi недоступен -  Интернет недоступен

  const char* host = "www.google.com";  // Имя хоста для проверки подключения

  WiFiClient client;
  if (client.connect(host, 80)) {  // Попытка подключения к хосту на порт 80
    client.stop();  // Закрываем соединение
    return true;  // Интернет доступен
  } else {
    return false;  // Интернет недоступен
  }
}

// Основная синхронизация времени.
// Порядок действий:
// 1) Если есть Интернет — запрашиваем NTP и обновляем базу времени.
// 2) Если Интернета нет — используем последнее сохраненное время из NVS.
void NPT_Time(int interval_NPT_Time){
  static unsigned long timerNtp;
  const unsigned long nowMs = millis();

  if (interval_NPT_Time + timerNtp > nowMs) return;
  timerNtp = nowMs;

  if (checkInternetAvailability()) {
    gmtOffset_correct = normalizeGmtOffset(gmtOffset_correct);

    NTPClient timeClient1(ntpUDP, ntpServer1, 3600 * gmtOffset_correct, daylightOffset);
    NTPClient timeClient2(ntpUDP, ntpServer2, 3600 * gmtOffset_correct, daylightOffset);

    timeClient1.begin();
    timeClient1.update();

    time_t epochTime = timeClient1.getEpochTime();
    if (epochTime == 0) {
      timeClient2.begin();
      timeClient2.update();
      epochTime = timeClient2.getEpochTime();
    }

    if (epochTime != 0) {
      tm *timeInfo = localtime(&epochTime);
      if (timeInfo != nullptr) {
        npt_hours = timeInfo->tm_hour;
        npt_minutes = timeInfo->tm_min;
        npt_seconds = timeInfo->tm_sec;
        npt_Day = timeInfo->tm_mday;
        npt_Month = timeInfo->tm_mon + 1;
        npt_Year = timeInfo->tm_year + 1900;
        npt_DayOfWeek = (timeInfo->tm_wday + 6) % 7 + 1;
      }
    }

    if (isValidDateTime(npt_Year, npt_Month, npt_Day, npt_hours, npt_minutes, npt_seconds)) {
      period_get_NPT_Time = 600000;
      setBaseEpoch(epochTime);
      DayOfWeek = npt_DayOfWeek;
    } else {
      period_get_NPT_Time = 15000;
    }
  } else {
    period_get_NPT_Time = 15000;
    if (!baseEpochReady) {
      loadBaseEpochFromStorage();
    }
  }
} //Закрываем общую функцию таймера синхронизации времени