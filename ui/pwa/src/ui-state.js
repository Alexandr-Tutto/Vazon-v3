const entityLabels = {
  climate: 'Клімат',
  pot: 'Ґрунт',
  humidifier: 'Зволоження',
  light: 'Світло',
  fan: 'Вентиляція',
  connection: 'Wi-Fi / звʼязок',
  door: 'Двері',
  system: 'Система',
};

function valueOrUnknown(value, suffix = '') {
  if (value === null || value === undefined || value === 'unknown') {
    return '—';
  }

  return `${value}${suffix}`;
}

function getAffectedLabel(affected) {
  return entityLabels[affected] || 'Система';
}

function tile(entity, title, status, summary, active = false) {
  return { entity, title, status, summary, active };
}

function makeDetail(title, status, summary, message, rows) {
  return { title, status, summary, message, rows };
}

function statusSummaryText(entity) {
  const reasons = {
    rh_delta_high: '- різниця вологості висока',
    temperature_delta_high: '- різниця температур висока',
    temperature_delta_critical: '- різниця температур критична',
    sht31_missing: '- датчик клімату не відповідає',
    sht31_stale: '- дані клімату застаріли',
    invalid_value: '- некоректне значення датчика',
  };
  const statuses = {
    ok: '- норма',
    warning: '- попередження',
    error: '- помилка',
    inactive: '- неактивно',
    unknown: '- невідомо',
  };

  return reasons[entity.status_reason] || statuses[entity.status] || '- норма';
}

function lightOutputText(value) {
  if (value === 'on') return 'Активовано';
  if (value === 'off') return 'Вимкнено';
  return '—';
}

function wifiStateText(value) {
  const labels = {
    connected: 'Добрий',
    good: 'Добрий',
    medium: 'Середній',
    weak: 'Середній',
    poor: 'Слабкий',
    disconnected: 'Недоступний',
    offline: 'Недоступний',
  };

  return labels[value] || value || '—';
}

function serverStateText(value) {
  if (value === 'connected') return 'Підʼєднано';
  return 'Недоступний';
}

function modeText(value) {
  if (value === 'auto') return 'авто';
  if (value === 'manual') return 'ручний';
  return value || '—';
}

function outputText(value) {
  if (value === 'on') return 'увімкнено';
  if (value === 'off') return 'вимкнено';
  return value || '—';
}

function manualStateText(value) {
  if (value === 'on') return 'увімкнено';
  if (value === 'off') return 'вимкнено';
  return value || '—';
}

function fanOutputText(value) {
  if (value === 'on') return 'працює';
  if (value === 'off') return 'заблоковано через відкриті двері';
  return value || '—';
}

function fanAutoStateText(value) {
  const labels = {
    blocked: 'заблоковано',
    running: 'працює',
    pause: 'пауза',
    alert: 'увага',
    off: 'вимкнено',
  };

  return labels[value] || value || '—';
}

function soilClassText(value) {
  const labels = {
    dry: 'сухо',
    normal: 'норма',
    wet: 'мокро',
    overwet: 'перезволожено',
    unknown: 'невідомо',
  };

  return labels[value] || value || 'невідомо';
}

function strategyText(value) {
  if (value === 'delta') return 'за різницею';
  if (value === 'timer') return 'за таймером';
  return value || '—';
}

function waterStatusText(value) {
  const labels = {
    present: 'вода є',
    empty: 'немає води',
    unknown: 'стан води невідомий',
  };

  return labels[value] || value || '—';
}

function humidifierSummary(humidifier) {
  const reasons = {
    door_open: '- заблоковано через відкриті двері',
    no_water: '- немає води',
    water_unknown: '- стан води невідомий',
    manual_mode: '- ручний режим',
  };

  if (humidifier.status_reason) {
    return reasons[humidifier.status_reason] || `- ${humidifier.status_reason}`;
  }

  if (humidifier.mist_output === 'on') {
    return '- працює';
  }

  return '- норма';
}

export function createUiState(raw) {
  const system = {
    ...raw.system,
    affected_system_label: getAffectedLabel(raw.system.affected_system),
  };

  const climate = {
    top: {
      temperature_c: valueOrUnknown(raw.climate.sensor_0x44.temperature_c, '°'),
      humidity_pct: valueOrUnknown(raw.climate.sensor_0x44.humidity_pct, '%'),
    },
    bottom: {
      temperature_c: valueOrUnknown(raw.climate.sensor_0x45.temperature_c, '°'),
      humidity_pct: valueOrUnknown(raw.climate.sensor_0x45.humidity_pct, '%'),
    },
  };

  const potItems = raw.pot || [];
  const potHasIssue = potItems.some((pot) => pot.status !== 'ok');
  const potAggregate = {
    status: potHasIssue ? 'warning' : 'ok',
    summary: `${potItems.length} вазони ok`,
  };

  const tiles = [
    tile('climate', 'Клімат', raw.climate.status, raw.climate.status_reason ? 'Увага' : 'Норма'),
    tile('pot', 'Ґрунт', potAggregate.status, potAggregate.status === 'ok' ? 'Норма' : potAggregate.summary),
    tile('humidifier', 'Зволоження', raw.humidifier.status, raw.humidifier.status_reason ? 'Увага' : 'Норма', raw.humidifier.mist_output === 'on'),
    tile('light', 'Світло', raw.light.status, lightOutputText(raw.light.output), raw.light.output === 'on'),
    tile('fan', 'Вентиляція', raw.fan.status, raw.fan.output === 'on' ? 'Працює' : 'Заблоковано через відкриті двері', raw.fan.output === 'on'),
    tile('connection', 'Wi-Fi / звʼязок', raw.system.global_context.connection.mqtt_state === 'connected' ? 'ok' : 'warning', wifiStateText(raw.system.global_context.connection.wifi_state)),
  ];

  const details = {
    climate: makeDetail('Клімат', raw.climate.status, statusSummaryText(raw.climate), 'Показано дві зони без усереднення.', [
      { label: 'SHT31 0x44 температура', value: valueOrUnknown(raw.climate.sensor_0x44.temperature_c, '°C') },
      { label: 'SHT31 0x44 вологість', value: valueOrUnknown(raw.climate.sensor_0x44.humidity_pct, '%') },
      { label: 'SHT31 0x45 температура', value: valueOrUnknown(raw.climate.sensor_0x45.temperature_c, '°C') },
      { label: 'SHT31 0x45 вологість', value: valueOrUnknown(raw.climate.sensor_0x45.humidity_pct, '%') },
      { label: 'RH delta', value: valueOrUnknown(raw.climate.rh_delta_pct, '%') },
    ]),

    pot: makeDetail('Ґрунт', potAggregate.status, potAggregate.summary, 'Деталі по вазонах показуються тут, не на головному екрані.', [
      { label: 'Вазон 1 вологість', value: valueOrUnknown(raw.pot[0].soil_moisture.value, '%') },
      { label: 'Вазон 1 стан ґрунту', value: soilClassText(raw.pot[0].soil_moisture.class) },
      { label: 'Вазон 1 температура', value: valueOrUnknown(raw.pot[0].soil_temperature.temperature_c, '°C') },
      { label: 'Вазон 2 вологість', value: valueOrUnknown(raw.pot[1].soil_moisture.value, '%') },
      { label: 'Вазон 2 стан ґрунту', value: soilClassText(raw.pot[1].soil_moisture.class) },
      { label: 'Вазон 2 температура', value: valueOrUnknown(raw.pot[1].soil_temperature.temperature_c, '°C') },
    ]),

    humidifier: makeDetail('Зволоження', raw.humidifier.status, humidifierSummary(raw.humidifier), '', [
      { label: 'Вода', value: waterStatusText(raw.humidifier.water_status) },
      { label: 'Пар', value: outputText(raw.humidifier.mist_output) },
      { label: 'Режим', value: modeText(raw.humidifier.settings.mode) },
    ]),

    light: makeDetail('Світло', raw.light.status, lightOutputText(raw.light.output), 'Світло є actuator-модулем. При обслуговуванні працює як сервісне підсвічування.', [
      { label: 'Вихід', value: lightOutputText(raw.light.output) },
      { label: 'Режим', value: modeText(raw.light.settings.mode) },
      { label: 'Ручний стан', value: manualStateText(raw.light.settings.manual_state) },
    ]),

    fan: makeDetail('Вентиляція', raw.fan.status, fanOutputText(raw.fan.output), 'Показано основний вентилятор шафи.', [
      { label: 'Автостан', value: fanAutoStateText(raw.fan.auto_state) },
      { label: 'Режим', value: modeText(raw.fan.settings.mode) },
      { label: 'Стратегія', value: strategyText(raw.fan.settings.auto_strategy) },
    ]),

    connection: makeDetail('Wi-Fi / звʼязок', raw.system.status, serverStateText(raw.system.global_context.connection.mqtt_state), '', [
      { label: 'Wi-Fi', value: wifiStateText(raw.system.global_context.connection.wifi_state) },
      { label: 'Сервер', value: serverStateText(raw.system.global_context.connection.mqtt_state) },
    ]),
  };

  return {
    raw,
    system,
    climate,
    tiles,
    details,
    connectionLabel: `${wifiStateText(raw.system.global_context.connection.wifi_state)} / ${serverStateText(raw.system.global_context.connection.mqtt_state)}`,
  };
}
