const entityLabels = {
  climate: 'Клімат',
  pot: 'Ґрунт',
  humidifier: 'Зволоження',
  light: 'Світло',
  fan: 'Вентиляція',
  connection: 'Звʼязок',
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

  const tiles = [
    tile('climate', 'Клімат', raw.climate.status, raw.climate.status_reason || 'Дві зони'),
    tile('pot', 'Ґрунт', raw.pot_aggregate.status, raw.pot_aggregate.summary),
    tile('humidifier', 'Зволоження', raw.humidifier.status, raw.humidifier.status_reason || raw.humidifier.water_status, raw.humidifier.mist_output === 'on'),
    tile('light', 'Світло', raw.light.status, raw.light.output, raw.light.output === 'on'),
    tile('fan', 'Вентиляція', raw.fan.status, raw.fan.output, raw.fan.output === 'on'),
    tile('connection', 'Wi-Fi / звʼязок', raw.system.global_context.connection.mqtt_state === 'connected' ? 'ok' : 'warning', raw.system.global_context.connection.mqtt_state),
  ];

  const details = {
    climate: makeDetail('Клімат', raw.climate.status, 'Датчики повітря', raw.climate.status_reason || 'Показано дві зони без усереднення.', [
      { label: 'SHT31 0x44 температура', value: valueOrUnknown(raw.climate.sensor_0x44.temperature_c, '°C') },
      { label: 'SHT31 0x44 вологість', value: valueOrUnknown(raw.climate.sensor_0x44.humidity_pct, '%') },
      { label: 'SHT31 0x45 температура', value: valueOrUnknown(raw.climate.sensor_0x45.temperature_c, '°C') },
      { label: 'SHT31 0x45 вологість', value: valueOrUnknown(raw.climate.sensor_0x45.humidity_pct, '%') },
      { label: 'RH delta', value: valueOrUnknown(raw.climate.rh_delta_pct, '%') },
    ]),

    pot: makeDetail('Ґрунт', raw.pot_aggregate.status, raw.pot_aggregate.summary, 'Деталі по вазонах показуються тут, не на головному екрані.', [
      { label: 'pot[0] moisture', value: valueOrUnknown(raw.pot0.soil_moisture.value_pct, '%') },
      { label: 'pot[0] class', value: raw.pot0.soil_moisture.class },
      { label: 'pot[0] temperature', value: valueOrUnknown(raw.pot0.soil_temperature.temperature_c, '°C') },
      { label: 'pot[1] moisture', value: valueOrUnknown(raw.pot1.soil_moisture.value_pct, '%') },
      { label: 'pot[1] class', value: raw.pot1.soil_moisture.class },
      { label: 'pot[1] temperature', value: valueOrUnknown(raw.pot1.soil_temperature.temperature_c, '°C') },
    ]),

    humidifier: makeDetail('Зволоження', raw.humidifier.status, raw.humidifier.status_reason || 'Стан зволоження', 'Вода і локальний вентилятор зволожувача належать цій підсистемі.', [
      { label: 'Вода', value: raw.humidifier.water_status },
      { label: 'Mist', value: raw.humidifier.mist_output },
      { label: 'Local fan', value: raw.humidifier.fan_output },
      { label: 'Mode', value: raw.humidifier.settings.mode },
    ]),

    light: makeDetail('Світло', raw.light.status, raw.light.output, 'Світло є actuator-модулем. При обслуговуванні працює як сервісне підсвічування.', [
      { label: 'Output', value: raw.light.output },
      { label: 'Mode', value: raw.light.settings.mode },
      { label: 'Manual state', value: raw.light.settings.manual_state },
    ]),

    fan: makeDetail('Вентиляція', raw.fan.status, raw.fan.output, 'Показано основний вентилятор шафи. Вентилятор зволожувача не тут.', [
      { label: 'Output', value: raw.fan.output },
      { label: 'Auto state', value: raw.fan.auto_state },
      { label: 'Mode', value: raw.fan.settings.mode },
      { label: 'Strategy', value: raw.fan.settings.auto_strategy },
    ]),

    connection: makeDetail('Wi-Fi / звʼязок', raw.system.status, raw.system.global_context.connection.mqtt_state, 'У звичайному UI технічне слово MQTT не показується на головному екрані.', [
      { label: 'Wi-Fi', value: raw.system.global_context.connection.wifi_state },
      { label: 'MQTT', value: raw.system.global_context.connection.mqtt_state },
    ]),
  };

  return {
    raw,
    system,
    climate,
    tiles,
    details,
    connectionLabel: `${raw.system.global_context.connection.wifi_state} / ${raw.system.global_context.connection.mqtt_state}`,
  };
}
