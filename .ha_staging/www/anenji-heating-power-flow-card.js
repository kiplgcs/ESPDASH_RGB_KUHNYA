class AnenjiHeatingPowerFlowCard extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    this._config = {};
    this._hass = null;
    this._history = { boiler: [], pool: [] };
    this._historyLastFetchMs = 0;
    this._historyLoading = false;
    this._ticker = null;
  }

  connectedCallback() {
    if (!this._ticker) {
      this._ticker = window.setInterval(() => {
        this._sampleLiveHistory();
        this._render();
      }, 1000);
    }
  }

  disconnectedCallback() {
    if (this._ticker) {
      window.clearInterval(this._ticker);
      this._ticker = null;
    }
  }

  setConfig(config) {
    this._config = {
      pv_entity: "sensor.moshchnost_pv_pv_power",
      grid_entity: "sensor.moshchnost_seti_grid_power",
      grid_voltage_entity: "sensor.napriazhenie_seti_grid_voltage",
      grid_frequency_entity: "sensor.chastota_seti_grid_frequency",
      battery_entity: "sensor.anenji_flow_battery_power_signed",
      load_entity: "sensor.moshchnost_nagruzki_load_power",
      total_limit_entity: "input_number.anenji_dynamic_heater_limit_w",
      boiler_limit_entity: "input_number.anenji_dynamic_boiler_limit_w",
      pool_limit_entity: "input_number.anenji_dynamic_pool_limit_w",
      boiler_need_entity: "binary_sensor.anenji_boiler_heat_demand",
      pool_need_entity: "binary_sensor.anenji_pool_heat_demand",
      boiler_need_stable_entity: "binary_sensor.anenji_boiler_heat_demand_stable",
      pool_need_stable_entity: "binary_sensor.anenji_pool_heat_demand_stable",
      boiler_window_entity: "binary_sensor.anenji_okno_nagreva_boilera",
      pool_window_entity: "binary_sensor.anenji_okno_nagreva_basseina",
      status_entity: "input_text.anenji_surplus_distribution_status",
      forced_mode_entity: "input_select.anenji_surplus_forced_output_mode",
      actual_mode_entity: "sensor.prioritet_vykhoda_output_priority",
      grid_allowance_entity: "input_number.anenji_surplus_grid_import_allowance_w",
      boiler_echo_entity: "sensor.upravlenie_boilerom_upravlenie_bolerom_izbytok_invertora",
      boiler_power_entity: "sensor.pzem004t_pzem004t_moshchnost",
      mqtt_connected_entity: "binary_sensor.sistema_sistema_mqtt_podkliuchen",
      mqtt_fresh_entity: "binary_sensor.sistema_sistema_mqtt_surplus_svezhii",
      ...config,
    };
    this._render();
  }

  set hass(hass) {
    this._hass = hass;
    this._sampleLiveHistory();
    this._loadHistoryIfNeeded();
    this._render();
  }

  getCardSize() {
    return 14;
  }

  getGridOptions() {
    return {
      columns: "full",
      min_columns: 12,
      max_columns: 12,
    };
  }

  _entity(id) {
    return id && this._hass && this._hass.states ? this._hass.states[id] : null;
  }

  _state(id) {
    const entity = this._entity(id);
    return entity ? entity.state : "unavailable";
  }

  _available(id) {
    const state = this._state(id);
    return !["unknown", "unavailable", "none", ""].includes(state);
  }

  _number(id) {
    if (!this._available(id)) return null;
    const value = Number.parseFloat(this._state(id));
    return Number.isFinite(value) ? value : null;
  }

  _isOn(id) {
    return this._state(id) === "on";
  }

  _escape(value) {
    return String(value ?? "")
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#039;");
  }

  _power(value) {
    if (value === null) return "нет данных";
    const sign = value < 0 ? "−" : "";
    const absolute = Math.abs(value);
    if (absolute >= 1000) {
      return `${sign}${(absolute / 1000).toFixed(2).replace(".", ",")} кВт`;
    }
    return `${sign}${Math.round(absolute).toLocaleString("ru-RU")} Вт`;
  }

  _decimal(value, digits, unit) {
    if (value === null) return "—";
    return `${value.toFixed(digits).replace(".", ",")} ${unit}`;
  }

  _age(id) {
    const entity = this._entity(id);
    if (!entity || !entity.last_updated) return null;
    const age = Math.max(0, (Date.now() - new Date(entity.last_updated).getTime()) / 1000);
    return age;
  }

  _ageText(id) {
    const age = this._age(id);
    if (age === null) return "нет данных";
    if (age < 2) return "сейчас";
    if (age < 60) return `${Math.round(age)} с`;
    return `${Math.round(age / 60)} мин`;
  }

  _appendHistory(kind, value, timestamp = Date.now()) {
    if (!Number.isFinite(value)) return;
    const history = this._history[kind] || [];
    const last = history[history.length - 1];
    if (last && timestamp - last.t < 900 && last.v === value) return;
    history.push({ t: timestamp, v: value });
    const earliest = Date.now() - 60 * 60 * 1000;
    this._history[kind] = history.filter((point) => point.t >= earliest);
  }

  _sampleLiveHistory() {
    if (!this._hass) return;
    const c = this._config;
    this._appendHistory("boiler", this._number(c.boiler_limit_entity));
    this._appendHistory("pool", this._number(c.pool_limit_entity));
  }

  async _loadHistoryIfNeeded() {
    if (!this._hass || typeof this._hass.callApi !== "function" ||
        this._historyLoading || Date.now() - this._historyLastFetchMs < 5 * 60 * 1000) {
      return;
    }
    this._historyLoading = true;
    this._historyLastFetchMs = Date.now();
    const c = this._config;
    const ids = [c.boiler_limit_entity, c.pool_limit_entity].filter(Boolean);
    const now = new Date();
    const start = new Date(now.getTime() - 60 * 60 * 1000);
    const query = new URLSearchParams({
      filter_entity_id: ids.join(","),
      end_time: now.toISOString(),
      no_attributes: "1",
    });
    try {
      const result = await this._hass.callApi(
        "GET",
        `history/period/${encodeURIComponent(start.toISOString())}?${query.toString()}`
      );
      const source = Array.isArray(result) ? result : [];
      const entityToKind = {
        [c.boiler_limit_entity]: "boiler",
        [c.pool_limit_entity]: "pool",
      };
      for (const states of source) {
        if (!Array.isArray(states) || !states.length) continue;
        const kind = entityToKind[states[0].entity_id];
        if (!kind) continue;
        const points = states
          .map((state) => ({
            t: new Date(state.last_changed || state.last_updated || now).getTime(),
            v: Number.parseFloat(state.state),
          }))
          .filter((point) => Number.isFinite(point.t) && Number.isFinite(point.v))
          .sort((a, b) => a.t - b.t);
        if (points.length) this._history[kind] = points;
      }
      this._sampleLiveHistory();
    } catch (_) {
      // История может быть выключена в Recorder; тогда продолжаем локально
      // собирать секундные точки с момента открытия панели.
    } finally {
      this._historyLoading = false;
      this._render();
    }
  }

  _miniChart(kind, color, currentValue) {
    const now = Date.now();
    const start = now - 60 * 60 * 1000;
    const history = (this._history[kind] || [])
      .filter((point) => point.t >= start && Number.isFinite(point.v));
    if (Number.isFinite(currentValue)) {
      const last = history[history.length - 1];
      if (!last || now - last.t > 900 || last.v !== currentValue) {
        history.push({ t: now, v: currentValue });
      }
    }
    const values = history.map((point) => point.v);
    const max = Math.max(100, ...values.map((value) => Math.abs(value)));
    const toPoint = (point) => {
      const x = 2 + ((point.t - start) / (60 * 60 * 1000)) * 196;
      const y = 33 - Math.min(30, (Math.max(0, point.v) / max) * 30);
      return `${Math.max(2, Math.min(198, x)).toFixed(1)},${y.toFixed(1)}`;
    };
    const points = history.map(toPoint).join(" ");
    const path = points || "2,33 198,33";
    return `
      <div class="mini-chart" title="Изменение выданного лимита за последний час; обновляется каждую секунду">
        <span>лимит · 1 ч</span>
        <svg viewBox="0 0 200 36" aria-hidden="true">
          <path class="chart-grid" d="M2 33 H198 M2 18 H198"></path>
          <polyline points="${path}" style="--chart:${color}"></polyline>
        </svg>
      </div>
    `;
  }

  _modeText() {
    const desired = this._state(this._config.forced_mode_entity);
    const actualRaw = this._state(this._config.actual_mode_entity);
    const actualCode = Number.parseInt(actualRaw, 10);
    const actual = actualCode === 1 ? "SOL" : actualCode === 3 ? "SUB" : actualRaw;
    if (!this._available(this._config.actual_mode_entity)) return `${desired} · нет подтверждения`;
    return desired === actual ? `${desired} · подтверждён` : `${desired} → ожидается, сейчас ${actual}`;
  }

  _request(rawEntity, stableEntity) {
    const rawState = this._state(rawEntity);
    const stableOn = this._isOn(stableEntity);
    const active = rawState === "on" || stableOn;
    const unavailable = ["unknown", "unavailable", "none", ""].includes(rawState);
    const held = active && rawState !== "on";
    return {
      active,
      unavailable,
      held,
      label: rawState === "on"
        ? "ЕСТЬ ЗАПРОС"
        : held
          ? "УДЕРЖАНИЕ 5 С"
          : unavailable
            ? "НЕТ ДАННЫХ"
            : "ЗАПРОСА НЕТ",
    };
  }

  _sourcePath(id, path, color, active, marker, label, labelX, labelY) {
    const stateClass = active ? "active" : "idle";
    return `
      <path class="flow-base ${stateClass}" d="${path}" style="--flow:${color}" marker-end="url(#${marker})"></path>
      ${active ? `<path class="flow-pulse" d="${path}" style="--flow:${color}"></path>` : ""}
      <text class="line-label" x="${labelX}" y="${labelY}">${this._escape(label)}</text>
    `;
  }

  _requestPath(path, request, color, marker, label, x, y) {
    const css = request.unavailable && !request.active
      ? "request-error"
      : request.active
        ? "request-active"
        : "request-idle";
    const stroke = request.unavailable && !request.active ? "#ef5350" : request.active ? color : "#68727d";
    return `
      <path class="request-line ${css}" d="${path}" style="--request:${stroke}" marker-end="url(#${marker})"></path>
      <text class="request-label ${css}" x="${x}" y="${y}">${this._escape(label)}</text>
    `;
  }

  _chip(label, value, kind = "neutral", entity = "") {
    const entityAttr = entity ? ` data-entity="${this._escape(entity)}"` : "";
    return `<button class="chip ${kind}"${entityAttr}><span>${this._escape(label)}</span><b>${this._escape(value)}</b></button>`;
  }

  _render() {
    if (!this._hass || !this.shadowRoot) return;
    const viewportHeight = window.visualViewport?.height || window.innerHeight || 0;
    const hasTouchInput = (navigator.maxTouchPoints || 0) > 0
      || window.matchMedia?.("(hover: none)").matches;
    const touchLandscape = hasTouchInput
      && window.matchMedia?.("(orientation: landscape)").matches
      && viewportHeight <= 900;
    const compactDiagram = touchLandscape
      || window.matchMedia?.("(max-width: 700px)").matches;
    this.classList.toggle("mobile-landscape", touchLandscape);
    const c = this._config;
    const pv = this._number(c.pv_entity);
    const grid = this._number(c.grid_entity);
    const gridVoltage = this._number(c.grid_voltage_entity);
    const gridFrequency = this._number(c.grid_frequency_entity);
    const battery = this._number(c.battery_entity);
    const load = this._number(c.load_entity);
    const totalLimit = this._number(c.total_limit_entity);
    const boilerLimit = this._number(c.boiler_limit_entity);
    const poolLimit = this._number(c.pool_limit_entity);
    const boilerEcho = this._number(c.boiler_echo_entity);
    const boilerPower = this._number(c.boiler_power_entity);
    const gridAllowance = this._number(c.grid_allowance_entity);
    const boilerRequest = this._request(c.boiler_need_entity, c.boiler_need_stable_entity);
    const poolRequest = this._request(c.pool_need_entity, c.pool_need_stable_entity);

    const pvActive = pv !== null && pv > 0;
    const gridActive = grid !== null && Math.abs(grid) >= 1;
    const batteryActive = battery !== null && Math.abs(battery) >= 1;
    const boilerActive = boilerLimit !== null && boilerLimit > 0;
    const poolActive = poolLimit !== null && poolLimit > 0;

    const gridPath = grid !== null && grid < 0
      ? "M 600 224 L 600 126"
      : "M 600 126 L 600 224";
    const gridColor = grid !== null && grid < 0 ? "#26c6da" : "#42a5f5";
    const gridMarker = grid !== null && grid < 0 ? "arrow-cyan" : "arrow-blue";
    const gridDirection = grid === null
      ? "нет данных"
      : grid < 0
        ? `экспорт ${this._power(Math.abs(grid))}`
        : grid > 0
          ? `импорт ${this._power(grid)}`
          : "0 Вт · проверка сети";

    const batteryPath = battery !== null && battery > 0
      ? "M 690 246 C 790 205 895 160 1015 130"
      : "M 1015 130 C 895 160 790 205 690 246";
    const batteryColor = battery !== null && battery < 0 ? "#ef5350" : "#66bb6a";
    const batteryMarker = battery !== null && battery < 0 ? "arrow-red" : "arrow-green";
    const batteryDirection = battery === null
      ? "нет данных"
      : battery < 0
        ? `разряд ${this._power(Math.abs(battery))}`
        : battery > 0
          ? `заряд ${this._power(battery)}`
          : "без заряда/разряда";

    const status = this._available(c.status_entity)
      ? this._state(c.status_entity)
      : "Нет данных о состоянии автоматизации";
    const dangerousBattery = battery !== null && battery < 0 && (totalLimit || 0) > 0;
    const missingCritical = pv === null || grid === null || battery === null;
    const hasActiveRequest = boilerRequest.active || poolRequest.active;
    const statusKind = dangerousBattery || missingCritical
      ? "error"
      : (totalLimit || 0) > 0
        ? "ok"
        : hasActiveRequest
          ? "warning"
          : "neutral";

    const mqttConnectedState = this._state(c.mqtt_connected_entity);
    const mqttFreshState = this._state(c.mqtt_fresh_entity);
    const mqttConnected = mqttConnectedState === "on";
    const mqttFresh = mqttFreshState === "on";
    const mqttKnown = this._available(c.mqtt_connected_entity);
    const mqttFreshKnown = this._available(c.mqtt_fresh_entity);

    const flowSvg = `
      <svg class="wires" viewBox="0 0 1200 500" preserveAspectRatio="${compactDiagram ? "none" : "xMidYMid meet"}" role="img" aria-label="Потоки источников, лимитов и запросов нагрева">
        <defs>
          <marker id="arrow-amber" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#f9a825"></path></marker>
          <marker id="arrow-blue" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#42a5f5"></path></marker>
          <marker id="arrow-cyan" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#26c6da"></path></marker>
          <marker id="arrow-green" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#66bb6a"></path></marker>
          <marker id="arrow-red" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#ef5350"></path></marker>
          <marker id="arrow-boiler" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#ffb300"></path></marker>
          <marker id="arrow-pool" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#ab47bc"></path></marker>
          <marker id="arrow-request-boiler" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="context-stroke"></path></marker>
          <marker id="arrow-request-pool" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="context-stroke"></path></marker>
        </defs>
        ${this._sourcePath("pv", "M 185 132 C 300 160 410 205 510 246", "#f9a825", pvActive, "arrow-amber", pvActive ? this._power(pv) : "нет потока", 315, 179)}
        ${this._sourcePath("grid", gridPath, gridColor, gridActive, gridMarker, gridDirection, 620, 177)}
        ${this._sourcePath("battery", batteryPath, batteryColor, batteryActive, batteryMarker, batteryDirection, 826, 178)}
        ${this._sourcePath("boiler", "M 525 292 C 470 318 390 328 315 336", "#ffb300", boilerActive, "arrow-boiler", `лимит ${this._power(boilerLimit)}`, 395, 328)}
        ${this._sourcePath("pool", "M 675 292 C 730 318 810 328 885 336", "#ab47bc", poolActive, "arrow-pool", `лимит ${this._power(poolLimit)}`, 757, 328)}
        ${this._requestPath("M 330 470 C 430 498 520 400 565 310", boilerRequest, "#ffb300", "arrow-request-boiler", `P1 · ${boilerRequest.label}`, 405, 490)}
        ${this._requestPath("M 870 470 C 770 498 680 400 635 310", poolRequest, "#ab47bc", "arrow-request-pool", `P2 · ${poolRequest.label}`, 712, 490)}
      </svg>
    `;

    const node = (css, entity, kicker, title, value, details, topic = "", chart = "") => `
      <button class="node ${css}" data-entity="${this._escape(entity)}" title="${this._escape(topic)}">
        <span class="kicker">${this._escape(kicker)}</span>
        <strong>${this._escape(title)}</strong>
        <b class="node-value">${this._escape(value)}</b>
        <small>${details}</small>
        ${chart}
      </button>
    `;

    const pvNode = node("pv-node", c.pv_entity, "ИСТОЧНИК", "Солнечные панели", this._power(pv),
      `обновление ${this._escape(this._ageText(c.pv_entity))}`);
    const gridNode = node("grid-node", c.grid_entity, "ИСТОЧНИК ↔ ПРИЁМНИК", "Сеть ТНС", this._power(grid),
      `${this._escape(this._decimal(gridVoltage, 1, "В"))} · ${this._escape(this._decimal(gridFrequency, 2, "Гц"))}`);
    const batteryNode = node("battery-node", c.battery_entity, "ИСТОЧНИК ↔ ПРИЁМНИК", "Аккумуляторы", this._power(battery),
      this._escape(batteryDirection));
    const hubNode = node("hub-node", c.total_limit_entity, "ОБЩАЯ ШИНА", "Распределитель нагрева", this._power(totalLimit),
      `${this._escape(this._modeText())}<br>Дом: ${this._escape(this._power(load))} · ТНС разрешено: ${this._escape(this._power(gridAllowance))}`);
    const boilerNode = node("boiler-node", c.boiler_limit_entity, "ПРИОРИТЕТ 1", "Бойлер", this._power(boilerLimit),
      `принято ESP32: ${this._escape(this._power(boilerEcho))}<br>факт PZEM: ${this._escape(this._power(boilerPower))}`,
      "homeassistant/anenji/esp32_boiler_need · homeassistant/anenji/surplus_power/esp32_boiler_state",
      this._miniChart("boiler", "#ffb300", boilerLimit));
    const poolNode = node("pool-node", c.pool_limit_entity, "ПРИОРИТЕТ 2", "Бассейн", this._power(poolLimit),
      `выданная команда лимита<br>отдельного датчика фактической мощности нет`,
      "homeassistant/anenji/esp32_pool_need · homeassistant/anenji/surplus_power/esp32_pool_state",
      this._miniChart("pool", "#ab47bc", poolLimit));

    const boilerRequestKind = boilerRequest.unavailable && !boilerRequest.active ? "error" : boilerRequest.active ? "ok" : "neutral";
    const poolRequestKind = poolRequest.unavailable && !poolRequest.active ? "error" : poolRequest.active ? "ok" : "neutral";
    const chips = [
      this._chip("Запрос бойлера · P1", boilerRequest.label, boilerRequestKind, c.boiler_need_entity),
      this._chip("Запрос бассейна · P2", poolRequest.label, poolRequestKind, c.pool_need_entity),
      this._chip("Окно бойлера", this._isOn(c.boiler_window_entity) ? "разрешено" : "закрыто", this._isOn(c.boiler_window_entity) ? "ok" : "neutral", c.boiler_window_entity),
      this._chip("Окно бассейна", this._isOn(c.pool_window_entity) ? "разрешено" : "закрыто", this._isOn(c.pool_window_entity) ? "ok" : "neutral", c.pool_window_entity),
      this._chip("MQTT ESP32", mqttKnown ? (mqttConnected ? "подключён" : "отключён") : "нет данных", mqttConnected ? "ok" : "error", c.mqtt_connected_entity),
      this._chip("Лимит MQTT", mqttFreshKnown ? (mqttFresh ? "свежий" : "устарел") : "нет данных", mqttFresh ? "ok" : "error", c.mqtt_fresh_entity),
      this._chip("Grid / PV", `${this._ageText(c.grid_entity)} / ${this._ageText(c.pv_entity)}`, (grid === null || pv === null) ? "error" : "neutral"),
    ].join("");

    this.shadowRoot.innerHTML = `
      <style>
        :host{display:block;color:var(--primary-text-color,#eceff1)}
        *{box-sizing:border-box}
        ha-card{display:block;overflow:hidden;border:1px solid var(--divider-color,rgba(127,127,127,.22));border-radius:var(--ha-card-border-radius,12px);background:linear-gradient(145deg,var(--ha-card-background,var(--card-background-color,#171a1f)),color-mix(in srgb,var(--ha-card-background,var(--card-background-color,#171a1f)) 88%,#0b2634));box-shadow:var(--ha-card-box-shadow)}
        .header{display:flex;align-items:flex-start;justify-content:space-between;gap:16px;padding:18px 20px 0}
        .header h2{margin:0;font-size:21px;line-height:1.25;font-weight:650}.header p{margin:5px 0 0;color:var(--secondary-text-color,#9aa4ae);font-size:12px}
        .legend{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:12px;padding-top:3px;color:var(--secondary-text-color,#aeb7c0);font-size:11px;white-space:nowrap}
        .legend span::before{content:"";display:inline-block;width:26px;margin-right:6px;vertical-align:middle;border-top:3px solid #4fc3f7}.legend .demand::before{border-top-style:dashed;border-color:#ffb300}
        .scroll{overflow-x:auto;overscroll-behavior-inline:contain;padding:0 10px}.canvas{position:relative;width:100%;min-width:850px;max-width:1240px;height:500px;margin:0 auto}
        .wires{position:absolute;inset:0;width:100%;height:100%;overflow:visible}
        .flow-base{fill:none;stroke:var(--flow);stroke-width:5;stroke-linecap:round;opacity:.9}.flow-base.idle{stroke:var(--divider-color,#59636d);stroke-width:3;opacity:.42}
        .flow-pulse{fill:none;stroke:var(--flow);stroke-width:2;stroke-linecap:round;stroke-dasharray:2 15;animation:power-move 1.15s linear infinite;filter:drop-shadow(0 0 4px var(--flow))}
        .request-line{fill:none;stroke:var(--request);stroke-width:3;stroke-linecap:round;stroke-dasharray:9 8;opacity:.65}.request-line.request-active{stroke-width:4;opacity:1;animation:demand-move 1.25s linear infinite;filter:drop-shadow(0 0 3px var(--request))}.request-line.request-error{opacity:.95}
        .line-label,.request-label{fill:var(--secondary-text-color,#bdc5cc);font:600 13px sans-serif;paint-order:stroke;stroke:var(--ha-card-background,var(--card-background-color,#171a1f));stroke-width:5px;stroke-linejoin:round}.request-label{font-size:12px}.request-label.request-active{fill:var(--primary-text-color,#fff)}.request-label.request-error{fill:#ef9a9a}
        .node{position:absolute;z-index:2;display:flex;flex-direction:column;align-items:center;justify-content:center;width:210px;min-height:94px;padding:10px 12px;border:1px solid color-mix(in srgb,var(--node-color) 48%,var(--divider-color,#45515c));border-radius:15px;background:color-mix(in srgb,var(--ha-card-background,var(--card-background-color,#171a1f)) 92%,var(--node-color));color:var(--primary-text-color,#f5f7fa);font:inherit;text-align:center;cursor:pointer;box-shadow:0 7px 22px rgba(0,0,0,.22);transition:transform .15s ease,border-color .2s ease}
        .node:hover{transform:translate(-50%,-50%) scale(1.025);border-color:var(--node-color)}.node .kicker{font-size:9px;letter-spacing:.12em;color:var(--node-color);font-weight:750}.node strong{margin-top:2px;font-size:14px}.node-value{margin-top:3px;font-size:22px;font-variant-numeric:tabular-nums}.node small{margin-top:3px;color:var(--secondary-text-color,#aeb7c0);font-size:10px;line-height:1.35}
        .pv-node{--node-color:#f9a825;left:15.4%;top:20%}.grid-node{--node-color:#42a5f5;left:50%;top:16%}.battery-node{--node-color:#66bb6a;left:84.6%;top:20%}.hub-node{--node-color:#26c6da;left:50%;top:53%;width:270px;min-height:110px}.boiler-node{--node-color:#ffb300;left:25%;top:77%;width:245px;min-height:132px}.pool-node{--node-color:#ab47bc;left:75%;top:77%;width:245px;min-height:132px}
        .mini-chart{width:100%;margin-top:4px;padding-top:3px;border-top:1px solid color-mix(in srgb,var(--node-color) 26%,transparent)}.mini-chart span{display:block;margin-bottom:1px;color:var(--secondary-text-color,#aab4bd);font-size:8px;line-height:10px;text-transform:uppercase;letter-spacing:.08em}.mini-chart svg{display:block;width:100%;height:30px}.mini-chart .chart-grid{fill:none;stroke:var(--divider-color,#56616b);stroke-width:.8;opacity:.55}.mini-chart polyline{fill:none;stroke:var(--chart);stroke-width:2.2;stroke-linecap:round;stroke-linejoin:round;filter:drop-shadow(0 0 2px var(--chart))}
        .pv-node,.grid-node,.battery-node,.hub-node,.boiler-node,.pool-node{transform:translate(-50%,-50%)}.node:hover{transform:translate(-50%,-50%) scale(1.025)}
        .caption{margin:4px 18px 12px;text-align:center;color:var(--secondary-text-color,#aab4bd);font-size:11px}
        .status{margin:0 16px 12px;padding:13px 15px;border-left:4px solid var(--status-color);border-radius:9px;background:color-mix(in srgb,var(--status-color) 10%,transparent);font-size:14px;line-height:1.42}.status b{display:block;margin-bottom:3px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--status-color)}.status.ok{--status-color:#43a047}.status.warning{--status-color:#f9a825}.status.error{--status-color:#ef5350}.status.neutral{--status-color:#78909c}
        .chips{display:flex;flex-wrap:wrap;gap:7px;padding:0 16px 16px}.chip{display:flex;gap:6px;align-items:center;padding:7px 9px;border:1px solid var(--divider-color,rgba(127,127,127,.25));border-radius:999px;background:rgba(127,127,127,.07);color:var(--secondary-text-color,#b5bec6);font:inherit;font-size:10px;cursor:pointer}.chip b{color:var(--primary-text-color,#edf1f4);font-size:10px}.chip.ok{border-color:rgba(67,160,71,.45)}.chip.warning{border-color:rgba(249,168,37,.5)}.chip.error{border-color:rgba(239,83,80,.55);background:rgba(239,83,80,.08)}.chip.neutral[data-entity]:hover,.chip.ok:hover,.chip.error:hover{background:rgba(127,127,127,.14)}
        @keyframes power-move{to{stroke-dashoffset:-34}}@keyframes demand-move{to{stroke-dashoffset:34}}
        @media(max-width:700px){
          .header{display:block;padding:16px 16px 0}.header h2{font-size:20px;line-height:1.24}.header p{font-size:12px;line-height:1.45}.legend{justify-content:flex-start;gap:10px;padding-top:9px;font-size:10px;white-space:normal}
          .scroll{overflow:hidden;padding:0 4px}.canvas{width:100%;min-width:0;max-width:430px;height:545px}.wires{opacity:.68}.line-label,.request-label{display:none}
          .node{width:30%;min-height:100px;padding:8px 5px;border-radius:13px}.node .kicker{font-size:7px;letter-spacing:.08em}.node strong{margin-top:2px;font-size:12px;line-height:1.2}.node-value{margin-top:4px;font-size:20px}.node small{margin-top:4px;font-size:8px;line-height:1.25}
          .pv-node{left:16%;top:20%}.grid-node{left:50%;top:20%}.battery-node{left:84%;top:20%}.hub-node{left:50%;top:49%;width:72%;min-height:96px}.boiler-node{left:25%;top:78%;width:45%;min-height:140px}.pool-node{left:75%;top:78%;width:45%;min-height:140px}
          .mini-chart{margin-top:3px;padding-top:2px}.mini-chart span{font-size:7px;line-height:9px}.mini-chart svg{height:25px}
          .caption{margin:6px 14px 12px;font-size:11px;line-height:1.5}.status{margin:0 12px 11px;padding:11px 12px;font-size:13px;line-height:1.4}.chips{gap:6px;padding:0 12px 14px}.chip{gap:4px;padding:6px 8px;font-size:9px}.chip b{font-size:9px}
        }
        @media(max-width:360px){
          .canvas{height:525px}.node{min-height:94px}.node .kicker{font-size:6px}.node strong{font-size:11px}.node-value{font-size:18px}.node small{font-size:7px}.hub-node{min-height:90px}.boiler-node,.pool-node{min-height:132px}
        }
        @media(orientation:landscape) and (max-height:700px) and (max-width:1100px) and (pointer:coarse){
          .header{padding:12px 16px 0}.header h2{font-size:18px}.header p{font-size:11px}.legend{padding-top:5px;font-size:10px}
          .scroll{overflow:hidden;padding:0 5px}.canvas{width:100%;min-width:0;max-width:960px;height:430px}.wires{opacity:.76}.line-label,.request-label{display:none}
          .node{width:21%;min-height:86px;padding:7px 6px;border-radius:13px}.node .kicker{font-size:7px;letter-spacing:.08em}.node strong{font-size:12px;line-height:1.15}.node-value{margin-top:3px;font-size:19px}.node small{margin-top:3px;font-size:8px;line-height:1.2}
          .pv-node{left:16%;top:20%}.grid-node{left:50%;top:20%}.battery-node{left:84%;top:20%}.hub-node{left:50%;top:53%;width:31%;min-height:96px}.boiler-node{left:25%;top:78%;width:26%;min-height:120px}.pool-node{left:75%;top:78%;width:26%;min-height:120px}
          .mini-chart{margin-top:2px;padding-top:2px}.mini-chart span{font-size:7px;line-height:8px}.mini-chart svg{height:22px}.caption{margin:4px 14px 10px;font-size:10px}.status{margin:0 12px 10px;padding:10px 12px;font-size:12px}.chips{padding:0 12px 12px}
        }
        :host(.mobile-landscape) .header{padding:12px 16px 0}:host(.mobile-landscape) .header h2{font-size:18px}:host(.mobile-landscape) .header p{font-size:11px}:host(.mobile-landscape) .legend{padding-top:5px;font-size:10px}
        :host(.mobile-landscape) .scroll{overflow:hidden;padding:0 5px}:host(.mobile-landscape) .canvas{width:100%;min-width:0;max-width:960px;height:430px}:host(.mobile-landscape) .wires{opacity:.76}:host(.mobile-landscape) .line-label,:host(.mobile-landscape) .request-label{display:none}
        :host(.mobile-landscape) .node{width:21%;min-height:86px;padding:7px 6px;border-radius:13px}:host(.mobile-landscape) .node .kicker{font-size:7px;letter-spacing:.08em}:host(.mobile-landscape) .node strong{font-size:12px;line-height:1.15}:host(.mobile-landscape) .node-value{margin-top:3px;font-size:19px}:host(.mobile-landscape) .node small{margin-top:3px;font-size:8px;line-height:1.2}
        :host(.mobile-landscape) .pv-node{left:16%;top:20%}:host(.mobile-landscape) .grid-node{left:50%;top:20%}:host(.mobile-landscape) .battery-node{left:84%;top:20%}:host(.mobile-landscape) .hub-node{left:50%;top:53%;width:31%;min-height:96px}:host(.mobile-landscape) .boiler-node{left:25%;top:78%;width:26%;min-height:120px}:host(.mobile-landscape) .pool-node{left:75%;top:78%;width:26%;min-height:120px}
        :host(.mobile-landscape) .mini-chart{margin-top:2px;padding-top:2px}:host(.mobile-landscape) .mini-chart span{font-size:7px;line-height:8px}:host(.mobile-landscape) .mini-chart svg{height:22px}:host(.mobile-landscape) .caption{margin:4px 14px 10px;font-size:10px}:host(.mobile-landscape) .status{margin:0 12px 10px;padding:10px 12px;font-size:12px}:host(.mobile-landscape) .chips{padding:0 12px 12px}
        @media(prefers-reduced-motion:reduce){.flow-pulse,.request-line.request-active{animation:none}}
      </style>
      <ha-card>
        <div class="header">
          <div><h2>Потоки мощности и запросы нагрева</h2><p>Текущая картина распределения солнечной мощности</p></div>
          <div class="legend"><span>мощность / лимит</span><span class="demand">запрос ESP32</span></div>
        </div>
        <div class="scroll"><div class="canvas">${flowSvg}${pvNode}${gridNode}${batteryNode}${hubNode}${boilerNode}${poolNode}</div></div>
        <div class="caption">Источники показаны на общей шине дома; ветви бойлера и бассейна — выданные команды лимита, а не приписанная конкретному источнику мощность.</div>
        <div class="status ${statusKind}"><b>Что происходит сейчас</b>${this._escape(status)}</div>
        <div class="chips">${chips}</div>
      </ha-card>
    `;

    this.shadowRoot.querySelectorAll("[data-entity]").forEach((element) => {
      element.addEventListener("click", () => {
        const entityId = element.dataset.entity;
        if (!entityId) return;
        this.dispatchEvent(new CustomEvent("hass-more-info", {
          detail: { entityId },
          bubbles: true,
          composed: true,
        }));
      });
    });
  }
}

if (!customElements.get("anenji-heating-power-flow-card")) {
  customElements.define("anenji-heating-power-flow-card", AnenjiHeatingPowerFlowCard);
}

window.customCards = window.customCards || [];
window.customCards.push({
  type: "anenji-heating-power-flow-card",
  name: "ANENJI Heating Power Flow",
  description: "Потоки PV/Grid/АКБ, лимиты и запросы бойлера/бассейна",
});
