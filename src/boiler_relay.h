#pragma once

#include <Arduino.h>
#include "storage_nvs.h"

// Сухие контакты реле подключаются параллельно штатной кнопке котла.
// GPIO21 не используется остальными узлами этого проекта. Большинство
// опторазвязанных релейных модулей включаются низким уровнем.
inline constexpr uint8_t BOILER_RELAY_PIN = 21;
inline constexpr uint8_t BOILER_RELAY_ACTIVE_LEVEL = LOW;
inline constexpr uint8_t BOILER_RELAY_INACTIVE_LEVEL = HIGH;
inline constexpr uint32_t BOILER_RELAY_PULSE_MS = 500;

inline const char *BOILER_RELAY_COMMAND_TOPIC = "homeassistant/anenji/boiler_gas_relay/set";
inline const char *BOILER_RELAY_STATE_TOPIC = "homeassistant/anenji/boiler_gas_relay/state";
inline const char *BOILER_RELAY_CONTACT_TOPIC = "homeassistant/anenji/boiler_gas_relay/contact";
inline const char *BOILER_RELAY_STATUS_TOPIC = "homeassistant/anenji/boiler_gas_relay/status";

enum class BoilerRelayRequest : uint8_t {
  None = 0,
  DesiredOn,
  DesiredOff,
  ForcePulse,
  SyncOn,
  SyncOff
};

enum class BoilerRelayCommandSource : uint8_t {
  Startup = 0,
  Web,
  Mqtt
};

inline String BoilerRelayCommand = "idle";
inline String BoilerRelayPinInfo = "GPIO21 · активный LOW · импульс 500 мс";
inline String BoilerRelayStatus = "Реле ещё не инициализировано";
inline bool BoilerRelayAssumedOn = true;
inline bool BoilerRelayDesiredOn = true;
inline bool BoilerRelayContactActive = false;
inline bool BoilerRelayInitialized = false;
inline volatile bool BoilerRelayMqttStateDirty = true;

namespace boiler_relay_internal {
inline portMUX_TYPE requestMux = portMUX_INITIALIZER_UNLOCKED;
inline volatile uint8_t pendingRequest = static_cast<uint8_t>(BoilerRelayRequest::None);
inline volatile uint8_t pendingSource = static_cast<uint8_t>(BoilerRelayCommandSource::Startup);
inline uint32_t pulseStartedAtMs = 0;
inline bool pulseTargetOn = true;
inline BoilerRelayCommandSource activeSource = BoilerRelayCommandSource::Startup;

inline const char *sourceText(BoilerRelayCommandSource source) {
  switch (source) {
    case BoilerRelayCommandSource::Web: return "WEB";
    case BoilerRelayCommandSource::Mqtt: return "MQTT";
    default: return "STARTUP";
  }
}

inline void setStatus(const String &text) {
  BoilerRelayStatus = text;
  BoilerRelayMqttStateDirty = true;
  Serial.println("[BOILER RELAY] " + text);
}

inline void setAssumedState(bool isOn, BoilerRelayCommandSource source, const char *reason) {
  BoilerRelayAssumedOn = isOn;
  BoilerRelayDesiredOn = isOn;
  saveValue<int>("BoilerAssumed", isOn ? 1 : 0);
  setStatus(String(reason) + ": котёл считается " + (isOn ? "ВКЛ" : "ВЫКЛ") +
            " (источник " + sourceText(source) + ")");
}

inline void beginPulse(bool targetOn, BoilerRelayCommandSource source) {
  pulseTargetOn = targetOn;
  activeSource = source;
  BoilerRelayDesiredOn = targetOn;
  BoilerRelayContactActive = true;
  pulseStartedAtMs = millis();
  digitalWrite(BOILER_RELAY_PIN, BOILER_RELAY_ACTIVE_LEVEL);
  setStatus(String("Импульс штатной кнопки: запрос ") + (targetOn ? "ВКЛ" : "ВЫКЛ") +
            " (источник " + sourceText(source) + ")");
}
}  // namespace boiler_relay_internal

inline void setupBoilerRelay() {
  // Сначала записываем безопасный уровень в выходной latch и только затем
  // переводим GPIO в OUTPUT. Для активного LOW нужен внешний pull-up 10 кОм.
  digitalWrite(BOILER_RELAY_PIN, BOILER_RELAY_INACTIVE_LEVEL);
  pinMode(BOILER_RELAY_PIN, OUTPUT);
  digitalWrite(BOILER_RELAY_PIN, BOILER_RELAY_INACTIVE_LEVEL);

  BoilerRelayAssumedOn = loadValue<int>("BoilerAssumed", 1) != 0;
  BoilerRelayDesiredOn = BoilerRelayAssumedOn;
  BoilerRelayContactActive = false;
  BoilerRelayInitialized = true;
  boiler_relay_internal::setStatus(
      String("Готово; контакты разомкнуты, котёл считается ") +
      (BoilerRelayAssumedOn ? "ВКЛ" : "ВЫКЛ"));
}

inline bool queueBoilerRelayRequest(BoilerRelayRequest request,
                                    BoilerRelayCommandSource source) {
  if (!BoilerRelayInitialized || request == BoilerRelayRequest::None) return false;
  portENTER_CRITICAL(&boiler_relay_internal::requestMux);
  boiler_relay_internal::pendingRequest = static_cast<uint8_t>(request);
  boiler_relay_internal::pendingSource = static_cast<uint8_t>(source);
  portEXIT_CRITICAL(&boiler_relay_internal::requestMux);
  return true;
}

inline bool handleBoilerRelayCommand(const String &payload,
                                     BoilerRelayCommandSource source) {
  String command = payload;
  command.trim();
  command.toUpperCase();
  if (command == "ON" || command == "1") {
    return queueBoilerRelayRequest(BoilerRelayRequest::DesiredOn, source);
  }
  if (command == "OFF" || command == "0") {
    return queueBoilerRelayRequest(BoilerRelayRequest::DesiredOff, source);
  }
  if (command == "PRESS" || command == "PULSE" || command == "TOGGLE") {
    return queueBoilerRelayRequest(BoilerRelayRequest::ForcePulse, source);
  }
  if (command == "SYNC_ON") {
    return queueBoilerRelayRequest(BoilerRelayRequest::SyncOn, source);
  }
  if (command == "SYNC_OFF") {
    return queueBoilerRelayRequest(BoilerRelayRequest::SyncOff, source);
  }
  return false;
}

inline void onBoilerRelayUiCommand(const String &value) {
  if (value == "on") {
    queueBoilerRelayRequest(BoilerRelayRequest::DesiredOn, BoilerRelayCommandSource::Web);
  } else if (value == "off") {
    queueBoilerRelayRequest(BoilerRelayRequest::DesiredOff, BoilerRelayCommandSource::Web);
  } else if (value == "pulse") {
    queueBoilerRelayRequest(BoilerRelayRequest::ForcePulse, BoilerRelayCommandSource::Web);
  } else if (value == "sync_on") {
    queueBoilerRelayRequest(BoilerRelayRequest::SyncOn, BoilerRelayCommandSource::Web);
  } else if (value == "sync_off") {
    queueBoilerRelayRequest(BoilerRelayRequest::SyncOff, BoilerRelayCommandSource::Web);
  }

  // Команда является одноразовым действием, поэтому select всегда возвращается
  // в нейтральное положение и не повторяет импульс после перезагрузки.
  BoilerRelayCommand = "idle";
}

inline void loopBoilerRelay() {
  using namespace boiler_relay_internal;

  if (BoilerRelayContactActive) {
    if (millis() - pulseStartedAtMs < BOILER_RELAY_PULSE_MS) return;
    digitalWrite(BOILER_RELAY_PIN, BOILER_RELAY_INACTIVE_LEVEL);
    BoilerRelayContactActive = false;
    setAssumedState(pulseTargetOn, activeSource, "Импульс завершён");
  }

  uint8_t rawRequest = static_cast<uint8_t>(BoilerRelayRequest::None);
  uint8_t rawSource = static_cast<uint8_t>(BoilerRelayCommandSource::Startup);
  portENTER_CRITICAL(&requestMux);
  rawRequest = pendingRequest;
  rawSource = pendingSource;
  pendingRequest = static_cast<uint8_t>(BoilerRelayRequest::None);
  portEXIT_CRITICAL(&requestMux);

  const BoilerRelayRequest request = static_cast<BoilerRelayRequest>(rawRequest);
  const BoilerRelayCommandSource source = static_cast<BoilerRelayCommandSource>(rawSource);
  if (request == BoilerRelayRequest::None) return;

  if (request == BoilerRelayRequest::SyncOn || request == BoilerRelayRequest::SyncOff) {
    setAssumedState(request == BoilerRelayRequest::SyncOn, source, "Ручная синхронизация без импульса");
    return;
  }

  const bool targetOn = request == BoilerRelayRequest::DesiredOn
                            ? true
                            : request == BoilerRelayRequest::DesiredOff
                                  ? false
                                  : !BoilerRelayAssumedOn;
  if (request != BoilerRelayRequest::ForcePulse && targetOn == BoilerRelayAssumedOn) {
    BoilerRelayDesiredOn = targetOn;
    setStatus(String("Повторная команда ") + (targetOn ? "ВКЛ" : "ВЫКЛ") +
              " пропущена; состояние уже совпадает (источник " +
              sourceText(source) + ")");
    return;
  }

  beginPulse(targetOn, source);
}

