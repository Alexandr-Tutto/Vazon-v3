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
    confirmation_failed: '- обертання не підтверджено',
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
  if (value === 'off') return 'вимкнено';
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

function fanConfirmationText(value) {
  if (value === 'true') return 'Підтверджено';
  if (value === 'false') return 'Не підтверджено';
  return '—';
}

function aggregatePotStatus(pots) {
  const priority = {
    ok: 0,
    inactive: 1,
    unknown: 2,
    warning: 3,
    error: 4,
  };

  if (pots.length === 0) return 'unknown';

  return pots.reduce((worst, pot) => {
    const status = pot.status in priority ? pot.status : 'unknown';
    return priority[status] > priority[worst] ? status : worst;
  }, 'ok');
}

function potSummary(pots, status) {
  if (pots.some((pot) => pot.status_reason === 'calibration_invalid')) {
    return 'Потрібне калібрування';
  }

  const labels = {
    ok: 'Норма',
    warning: 'Потрібна увага',
    error: 'Потрібна дія',
    inactive: 'Вимкнено',
    unknown: 'Немає даних',
  };

  return labels[status] || labels.unknown;
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

function humidifierFanText(value) {
  const labels = {
    on: 'працює',
    post_run: 'післяпродувка',
    off: 'вимкнено',
    unknown: 'стан невідомий',
  };

  return labels[value] || value || '—';
}

function maintenanceDetailSummary(maintenance, doorState, blocked) {
  if (maintenance.door_active === true && doorState === 'open') {
    return blocked
      ? '- заблоковано через відкриті двері'
      : '- примусово активовано через відкриті двері';
  }

  if (maintenance.door_active === true) {
    return blocked
      ? '- заблоковано через невідомий стан дверей'
      : '- примусово активовано через невідомий стан дверей';
  }

  return blocked
    ? '- заблоковано через режим обслуговування'
    : '- примусово активовано в режимі обслуговування';
}

function maintenanceTileStatus(status) {
  return status === 'error' ? 'error' : 'warning';
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
  const deviceOffline =
    raw.system.global_context.connection.mqtt_state !== 'connected';
  const maintenance = raw.system.global_context.maintenance || {};
  const maintenanceActive = maintenance.active === true;
  const humidifierFanActive = raw.humidifier.fan_output === 'on'
    || raw.humidifier.fan_output === 'post_run';
  const fanDoorServiceRun = maintenanceActive
    && raw.door.state === 'open'
    && raw.fan.settings.mode === 'manual'
    && raw.fan.auto_state === 'running'
    && raw.fan.output === 'on';
  const system = deviceOffline ? {
    ...raw.system,
    status: 'warning',
    status_reason: 'device_offline',
    affected_system: 'connection',
    affected_system_label: getAffectedLabel('connection'),
  } : {
    ...raw.system,
    affected_system_label: getAffectedLabel(raw.system.affected_system),
  };

  const climate = {
    top: {
      temperature_c: deviceOffline ? '—' : valueOrUnknown(raw.climate.sensor_0x44.temperature_c, '°'),
      humidity_pct: deviceOffline ? '—' : valueOrUnknown(raw.climate.sensor_0x44.humidity_pct, '%'),
    },
    bottom: {
      temperature_c: deviceOffline ? '—' : valueOrUnknown(raw.climate.sensor_0x45.temperature_c, '°'),
      humidity_pct: deviceOffline ? '—' : valueOrUnknown(raw.climate.sensor_0x45.humidity_pct, '%'),
    },
  };

  const potItems = raw.pot || [];
  const potStatus = aggregatePotStatus(potItems);
  const potAggregate = {
    status: potStatus,
    summary: potSummary(potItems, potStatus),
  };
  const humidifierStatus = maintenanceActive
    ? maintenanceTileStatus(raw.humidifier.status)
    : raw.humidifier.status;
  const lightStatus = maintenanceActive
    ? maintenanceTileStatus(raw.light.status)
    : raw.light.status;
  const fanStatus = maintenanceActive
    ? maintenanceTileStatus(raw.fan.status)
    : raw.fan.status;
  const humidifierTileSummary = maintenanceActive
    ? 'Заблоковано'
    : (raw.humidifier.status_reason
      ? 'Увага'
      : (raw.humidifier.mist_output === 'on'
        ? 'Працює'
        : (raw.humidifier.fan_output === 'post_run'
          ? 'Післяпродувка'
          : (humidifierFanActive ? 'Вентилятор працює' : 'Норма'))));
  const lightTileSummary = maintenanceActive
    ? 'Примусово активовано'
    : lightOutputText(raw.light.output);
  const fanTileSummary = maintenanceActive
    ? (fanDoorServiceRun ? 'Працює вручну' : 'Заблоковано')
    : (raw.fan.status_reason
      ? 'Увага'
      : (raw.fan.output === 'on' ? 'Працює' : 'Вимкнено'));

  const liveTiles = [
    tile('climate', 'Клімат', raw.climate.status, raw.climate.status_reason ? 'Увага' : 'Норма'),
    tile('pot', 'Ґрунт', potAggregate.status, potAggregate.summary),
    tile('humidifier', 'Зволоження', humidifierStatus, humidifierTileSummary, !maintenanceActive && (raw.humidifier.mist_output === 'on' || humidifierFanActive)),
    tile('light', 'Світло', lightStatus, lightTileSummary, raw.light.output === 'on'),
    tile('fan', 'Вентиляція', fanStatus, fanTileSummary, raw.fan.output === 'on'),
    tile('connection', 'Wi-Fi / звʼязок', raw.system.global_context.connection.mqtt_state === 'connected' ? 'ok' : 'warning', wifiStateText(raw.system.global_context.connection.wifi_state)),
  ];
  const tiles = deviceOffline
    ? liveTiles.map((item) => item.entity === 'connection'
      ? tile('connection', 'Wi-Fi / звʼязок', 'warning', 'Пристрій недоступний')
      : tile(item.entity, item.title, 'unknown', 'Дані неактуальні', false))
    : liveTiles;

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

    humidifier: makeDetail('Зволоження', humidifierStatus, maintenanceActive ? maintenanceDetailSummary(maintenance, raw.door.state, true) : humidifierSummary(raw.humidifier), '', [
      { label: 'Вода', value: waterStatusText(raw.humidifier.water_status) },
      { label: 'Пар', value: outputText(raw.humidifier.mist_output) },
      { label: 'Локальний вентилятор', value: humidifierFanText(raw.humidifier.fan_output) },
      { label: 'Режим', value: modeText(raw.humidifier.settings.mode) },
    ]),

    light: makeDetail('Світло', lightStatus, maintenanceActive ? maintenanceDetailSummary(maintenance, raw.door.state, false) : lightOutputText(raw.light.output), 'Світло є actuator-модулем. При обслуговуванні працює як сервісне підсвічування.', [
      { label: 'Вихід', value: lightOutputText(raw.light.output) },
      { label: 'Режим', value: modeText(raw.light.settings.mode) },
      { label: 'Ручний стан', value: manualStateText(raw.light.settings.manual_state) },
    ]),

    fan: makeDetail('Вентиляція', fanStatus, fanDoorServiceRun ? '- працює вручну — двері відкриті' : (maintenanceActive ? maintenanceDetailSummary(maintenance, raw.door.state, true) : fanOutputText(raw.fan.output)), 'Показано основний вентилятор шафи.', [
      { label: 'Автостан', value: fanAutoStateText(raw.fan.auto_state) },
      { label: 'Режим', value: modeText(raw.fan.settings.mode) },
      { label: 'Стратегія', value: strategyText(raw.fan.settings.auto_strategy) },
      { label: 'Інтенсивність', value: valueOrUnknown(raw.fan.settings.power_level_pct, '%') },
      { label: 'Поточна потужність', value: valueOrUnknown(raw.fan.applied_power_pct, '%') },
      { label: 'Оберти', value: valueOrUnknown(raw.fan.rpm, ' об/хв') },
      { label: 'Обертання', value: fanConfirmationText(raw.fan.last_output_confirmed) },
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
    deviceOffline,
    tiles,
    details,
    connectionLabel: `${wifiStateText(raw.system.global_context.connection.wifi_state)} / ${serverStateText(raw.system.global_context.connection.mqtt_state)}`,
  };
}
