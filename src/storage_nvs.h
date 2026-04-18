#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>
#include <cstdio>
#include <type_traits>

inline Preferences prefs;

inline String normalizeNvsKey(const char *key) {
  if (!key) return String("k00000000");
  const size_t keyLen = strlen(key);
  if (keyLen <= 15) return String(key); // Короткие ключи оставляем как есть для совместимости.

  uint32_t hash = 2166136261UL; // FNV-1a 32-bit.
  for (size_t i = 0; i < keyLen; ++i) {
    hash ^= static_cast<uint8_t>(key[i]);
    hash *= 16777619UL;
  }

  char compact[16] = {0}; // NVS: максимум 15 символов для ключа.
  snprintf(compact, sizeof(compact), "k%08lx", static_cast<unsigned long>(hash));
  return String(compact);
}

inline void saveButtonState(const char *key, int val) {
  const String nvsKey = normalizeNvsKey(key);
  prefs.begin("buttons", false);
  prefs.putInt(nvsKey.c_str(), val);
  prefs.end();
}

inline int loadButtonState(const char *key, int def = 0) {
  const String nvsKey = normalizeNvsKey(key);
  prefs.begin("buttons", false);
  int val = prefs.getInt(nvsKey.c_str(), def);
  prefs.end();
  return val;
}

template <typename T> void saveValue(const char *key, T val);

template <typename T> T loadValue(const char *key, T def) {
  const String nvsKey = normalizeNvsKey(key);
  prefs.begin("MINIDASH", true);
  T val;
  if (prefs.isKey(nvsKey.c_str())) {
    if constexpr (std::is_same<T, float>::value)
      val = prefs.getFloat(nvsKey.c_str(), def);
    else if constexpr (std::is_same<T, int>::value)
      val = prefs.getInt(nvsKey.c_str(), def);
    else if constexpr (std::is_same<T, uint32_t>::value)
      val = prefs.getULong(nvsKey.c_str(), def);
    else if constexpr (std::is_same<T, uint16_t>::value)
      val = prefs.getUShort(nvsKey.c_str(), def);
    else if constexpr (std::is_same<T, uint8_t>::value)
      val = prefs.getUChar(nvsKey.c_str(), def);
    else if constexpr (std::is_same<T, bool>::value)
      val = prefs.getBool(nvsKey.c_str(), def);
    else if constexpr (std::is_same<T, String>::value)
      val = prefs.getString(nvsKey.c_str(), def);
    else
      val = def;
    prefs.end();
    return val;
  }
  prefs.end();
  val = def;
  saveValue<T>(key, val);
  return val;
}

template <typename T> void saveValue(const char *key, T val) {
  const String nvsKey = normalizeNvsKey(key);
  prefs.begin("MINIDASH", false);
  if constexpr (std::is_same<T, float>::value)
    prefs.putFloat(nvsKey.c_str(), val);
  else if constexpr (std::is_same<T, int>::value)
    prefs.putInt(nvsKey.c_str(), val);
  else if constexpr (std::is_same<T, uint32_t>::value)
    prefs.putULong(nvsKey.c_str(), val);
  else if constexpr (std::is_same<T, uint16_t>::value)
    prefs.putUShort(nvsKey.c_str(), val);
  else if constexpr (std::is_same<T, uint8_t>::value)
    prefs.putUChar(nvsKey.c_str(), val);
  else if constexpr (std::is_same<T, bool>::value)
    prefs.putBool(nvsKey.c_str(), val);
  else if constexpr (std::is_same<T, String>::value)
    prefs.putString(nvsKey.c_str(), val.c_str());
  prefs.end();
}