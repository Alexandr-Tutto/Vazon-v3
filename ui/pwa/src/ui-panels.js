import { getUiAction } from './ui-actions.js';
import { createMenuView, MENU_LEVELS } from './ui-menu-levels.js';

const noData = '—';

function show(value, suffix = '') {
  if (value === null || value === undefined || value === 'unknown') {
    return noData;
  }

  return `${value}${suffix}`;
}

function optionText(value, labels, fallback = noData) {
  if (value === null || value === undefined || value === 'unknown') {
    return fallback;
  }

  return labels[value] || value;
}

function yesNoText(value) {
  if (value === true) return 'так';
  if (value === false) return 'ні';
  return noData;
}

function outputText(value) {
  return optionText(value, {
    on: 'увімкнено',
    off: 'вимкнено',
  });
}

function modeText(value) {
  return optionText(value, {
    auto: 'авто',
    manual: 'ручний',
    off: 'вимкнено',
  });
}

function runtimeText(value) {
  return optionText(value, {
    day: 'день',
    always: 'завжди',
  });
}

function manualStateText(value) {
  return optionText(value, {
    on: 'увімкнено',
    off: 'вимкнено',
  });
}

function strategyText(value) {
  return optionText(value, {
    delta: 'за різницею',
    timer: 'за таймером',
  });
}

function autoStateText(value) {
  return optionText(value, {
    blocked: 'заблоковано',
    running: 'працює',
    idle: 'очікує',
    off: 'вимкнено',
  });
}

function commandResultText(value) {
  return optionText(value, {
    accepted: 'прийнято',
    rejected: 'відхилено',
    failed: 'помилка',
  });
}

function confirmationText(value) {
  return optionText(value, {
    confirmed: 'підтверджено',
    unavailable: 'недоступно',
    failed: 'не підтверджено',
    unknown: 'невідомо',
  });
}

function statusText(entity) {
  const reasons = {
    door_open: 'двері відкриті',
    manual_mode: 'ручний режим',
    output_failed: 'вихід не виконав команду',
    confirmation_failed: 'стан виходу не підтвердився',
    confirmation_unavailable: 'підтвердження недоступне',
  };
  const statuses = {
    ok: 'норма',
    warning: 'попередження',
    error: 'помилка',
    inactive: 'неактивно',
    unknown: 'невідомо',
  };

  return reasons[entity.status_reason] || statuses[entity.status] || 'норма';
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

function humidifierModeText(settings) {
  const runtimeLabels = {
    day: 'денний',
    always: 'постійний',
    unknown: 'невідомий',
  };
  const modeLabels = {
    auto: 'авто',
    manual: 'ручний',
    unknown: 'невідомо',
  };

  return `${runtimeLabels[settings.runtime] || settings.runtime || 'невідомий'} ${modeLabels[settings.mode] || settings.mode || 'невідомо'}`;
}

function mistPowerText(value) {
  const labels = {
    low: 'низька інтенсивність',
    medium: 'середня інтенсивність',
    high: 'висока інтенсивність',
    unknown: 'невідома інтенсивність',
  };

  return labels[value] || value || 'невідома інтенсивність';
}

function potStatusCard(index, pot) {
  return {
    label: `Вазон ${index + 1} - ${soilClassText(pot.soil_moisture.class)}`,
    pairs: [
      ['Температура', show(pot.soil_temperature.temperature_c, '°')],
      ['Вологість', show(pot.soil_moisture.value, '%')],
    ],
  };
}

export function getFunctionStatusView(entity, uiState) {
  const detail = uiState.details[entity];

  return createMenuView(
    MENU_LEVELS.FUNCTION_STATUS,
    `${detail.title} ${detail.summary || ''}`,
    getFunctionStatusCards(entity, uiState),
    { entity },
  );
}

export function getFunctionSettingsView(entity, uiState) {
  const detail = uiState.details[entity];

  return createMenuView(
    MENU_LEVELS.FUNCTION_SETTINGS,
    detail.title,
    getFunctionSettingsCards(entity, uiState),
    { entity },
  );
}

export function getAdvancedServiceView(uiState) {
  return createMenuView(
    MENU_LEVELS.ADVANCED_SERVICE,
    'Сервіс / розширено',
    getAdvancedServiceCards(uiState),
  );
}

function settingsOpenAction(entity) {
  return {
    id: `menu.${entity}.settings`,
    label: 'Налаштування',
    menuTarget: {
      level: MENU_LEVELS.FUNCTION_SETTINGS,
      entity,
    },
    command: null,
  };
}

function sensorSwitch(label, name, enabled, enableActionId, disableActionId, passiveLabel = `${label}-відкл.`) {
  return {
    label,
    activeLabel: label,
    passiveLabel,
    hideLabel: true,
    name,
    enabled,
    enableAction: getUiAction(enableActionId),
    disableAction: getUiAction(disableActionId),
  };
}

function potCalibrationAction(index, point) {
  return getUiAction(`pot[${index}].calibrate_soil_moisture.${point}`);
}

function potSettingsCard(index, label, pot) {
  return {
    label,
    sections: [
      {
        label: 'Датчики',
        inlineSwitches: true,
        switches: [
          sensorSwitch('Вологість', `pot${index}_soil_moisture_enabled`, pot.settings.soil_moisture_enabled, `pot[${index}].soil_moisture.enable`, `pot[${index}].soil_moisture.disable`, 'Вологість-відкл.'),
          sensorSwitch('Температура', `pot${index}_soil_temperature_enabled`, pot.settings.soil_temperature_enabled, `pot[${index}].soil_temperature.enable`, `pot[${index}].soil_temperature.disable`, 'Температура-відкл.'),
        ],
      },
      {
        label: 'Калібрування вологості',
        instructions: [
          ['Встановіть датчик у ємність із таким самим сухим грунтом. Зачекайте 1 хвилину, натисніть ', { action: potCalibrationAction(index, 'dry') }, '.'],
          ['Долийте води до бажаного оптимального стану. Зачекайте 1 хвилину, натисніть ', { action: potCalibrationAction(index, 'normal') }, '.'],
          ['Перелийте води, зробіть грунт вологішим, ніж потрібно. Зачекайте 1 хвилину, натисніть ', { action: potCalibrationAction(index, 'wet') }, '.'],
        ],
        note: ['Калібрування закінчено. Переставте датчик у бажаний вазон обережно, не пошкоджуючи коріння. Якщо на якомусь етапі помилились, натисніть ', { action: potCalibrationAction(index, 'reset') }, ' і повторіть процедуру із сухого грунту.'],
      },
      {
        label: 'Пороги та таймаути',
        pairs: [
          ['Таймаут вологості', show(pot.settings.moisture_stale_timeout_sec, ' сек')],
          ['Таймаут температури', show(pot.settings.temperature_stale_timeout_sec, ' сек')],
          ['Темп. мін.', show(pot.settings.temperature_low_warn_c, '°')],
          ['Темп. макс.', show(pot.settings.temperature_high_warn_c, '°')],
        ],
      },
      {
        label: 'Розширені параметри',
        controls: [getUiAction(`pot[${index}].settings.edit`)],
      },
    ],
    wide: true,
  };
}

function getFunctionStatusCards(entity, uiState) {
  const raw = uiState.raw;

  if (entity === 'climate') {
    return [
      { label: 'Температура', pairs: [['Вгорі', uiState.climate.top.temperature_c], ['Внизу', uiState.climate.bottom.temperature_c]] },
      { label: 'Вологість', pairs: [['Вгорі', uiState.climate.top.humidity_pct], ['Внизу', uiState.climate.bottom.humidity_pct]] },
      { label: 'Різниця зон', pairs: [['Температура', show(raw.climate.temperature_delta_c, '°')], ['Вологість', show(raw.climate.rh_delta_pct, '%')]] },
      { label: 'Стан підсистеми', value: statusText(raw.climate) },
      { label: 'Розширені параметри', controls: [settingsOpenAction(entity)], wide: true },
    ];
  }

  if (entity === 'pot') {
    return [
      potStatusCard(0, raw.pot[0]),
      potStatusCard(1, raw.pot[1]),
      { label: 'Розширені параметри', controls: [settingsOpenAction(entity)], wide: true },
    ];
  }

  if (entity === 'humidifier') {
    return [
      { label: 'Режим', pairs: [['Режим', humidifierModeText(raw.humidifier.settings)], ['Випаровування', mistPowerText(raw.humidifier.settings.mist_power_level)]], stacked: true },
      { label: 'Вологість,%', pairs: [['Мінімальна', show(raw.humidifier.settings.rh_start, '%')], ['Максимальна', show(raw.humidifier.settings.rh_stop, '%')]] },
      { label: 'Розширені параметри', controls: [settingsOpenAction(entity)], wide: true },
    ];
  }

  if (entity === 'light') {
    return [
      {
        label: 'Режим роботи',
        sections: [
          { label: 'Режим', controls: [getUiAction('light.mode.manual'), getUiAction('light.mode.auto')] },
          { label: 'Графік', pairs: [['Увімкнення', raw.system.global_context.day_window.time_on], ['Вимкнення', raw.system.global_context.day_window.time_off]] },
        ],
        wide: true,
      },
      { label: 'Розширені параметри', controls: [settingsOpenAction(entity)], wide: true },
    ];
  }

  if (entity === 'fan') {
    return [
      { label: 'Поточний стан', pairs: [['Вихід', outputText(raw.fan.output)], ['Автостан', autoStateText(raw.fan.auto_state)], ['Режим', modeText(raw.fan.settings.mode)]] },
      { label: 'Робота', pairs: [['Час роботи', runtimeText(raw.fan.settings.runtime)], ['Стратегія', strategyText(raw.fan.settings.auto_strategy)]] },
      { label: 'За різницею', pairs: [['Увімкнення', show(raw.fan.settings.auto_delta_on_pct, '%')], ['Вимкнення', show(raw.fan.settings.auto_delta_off_pct, '%')]] },
      { label: 'За таймером', pairs: [['Увімкнення', show(raw.fan.settings.auto_timer_on_sec, ' сек')], ['Вимкнення', show(raw.fan.settings.auto_timer_off_sec, ' сек')]] },
      { label: 'Підтвердження', pairs: [['Остання команда', commandResultText(raw.fan.last_command_result)], ['Вихід підтверджено', confirmationText(raw.fan.last_output_confirmed)]] },
      { label: 'Стан підсистеми', value: statusText(raw.fan), wide: true },
      { label: 'Розширені параметри', controls: [settingsOpenAction(entity)], wide: true },
    ];
  }

  if (entity === 'connection') {
    return [
      { label: 'Звʼязок', pairs: [['Wi-Fi', raw.system.global_context.connection.wifi_state], ['Сервіс', raw.system.global_context.connection.mqtt_state]] },
      { label: 'Актуальність даних', value: raw.system.global_context.connection.mqtt_state === 'connected' ? 'Дані актуальні' : 'Немає актуальних даних' },
      { label: 'Примітка', value: 'Технічні параметри звʼязку приховані у сервісному рівні', wide: true },
    ];
  }

  return [];
}

function getFunctionSettingsCards(entity, uiState) {
  const raw = uiState.raw;

  if (entity === 'climate') {
    return [
      { label: 'Верх', pairs: [['Статус', raw.climate.sensor_0x44.status], ['Причина', raw.climate.sensor_0x44.status_reason || '—']] },
      { label: 'Низ', pairs: [['Статус', raw.climate.sensor_0x45.status], ['Причина', raw.climate.sensor_0x45.status_reason || '—']] },
      {
        label: 'Warning limits',
        fields: [
          { label: 'Temp low', name: 'temperature_low_warn', value: raw.climate.settings.temperature_low_warn, unit: '°C', step: 1 },
          { label: 'Temp high', name: 'temperature_high_warn', value: raw.climate.settings.temperature_high_warn, unit: '°C', step: 1 },
          { label: 'RH low', name: 'humidity_low_warn', value: raw.climate.settings.humidity_low_warn, unit: '%', step: 1 },
          { label: 'RH high', name: 'humidity_high_warn', value: raw.climate.settings.humidity_high_warn, unit: '%', step: 1 },
        ],
      },
      {
        label: 'Delta limits',
        fields: [
          { label: 'Temp warn', name: 'temperature_delta_warn', value: raw.climate.settings.temperature_delta_warn, unit: '°C', step: 1 },
          { label: 'Temp error', name: 'temperature_delta_error', value: raw.climate.settings.temperature_delta_error, unit: '°C', step: 1 },
        ],
      },
      { label: 'Пороги', value: 'climate.set_settings', controls: [getUiAction('climate.thresholds.edit'), getUiAction('navigation.close')], wide: true },
    ];
  }

  if (entity === 'pot') {
    return [
      potSettingsCard(0, 'Вазон 1', raw.pot[0]),
      potSettingsCard(1, 'Вазон 2', raw.pot[1]),
    ];
  }

  if (entity === 'humidifier') {
    return [
      {
        label: 'Режим',
        sections: [
          { label: 'Режим', controls: [getUiAction('humidifier.mode.auto'), getUiAction('humidifier.mode.manual')] },
          { label: 'Час роботи', controls: [getUiAction('humidifier.runtime.day'), getUiAction('humidifier.runtime.always')] },
          { label: 'Інтенсивність пару', controls: [getUiAction('humidifier.power.low'), getUiAction('humidifier.power.medium'), getUiAction('humidifier.power.high')] },
        ],
      },
      {
        label: 'Ручне керування',
        fields: [
          { label: 'Безперервний пар', name: 'manual_mist_duration_sec', value: raw.humidifier.settings.manual_mist_duration_sec, unit: 'сек', step: 1 },
          { label: 'Затримка', name: 'post_fan_sec', value: raw.humidifier.settings.post_fan_sec, unit: 'сек', step: 1 },
        ],
        controls: [getUiAction('humidifier.manual_mist'), getUiAction('humidifier.stop'), getUiAction('humidifier.settings.edit')],
      },
      {
        label: 'Вологість, %',
        fields: [
          { label: 'Мінімальна, %', name: 'rh_start', value: raw.humidifier.settings.rh_start, step: 1 },
          { label: 'Максимальна, %', name: 'rh_stop', value: raw.humidifier.settings.rh_stop, step: 1 },
          { label: 'Різниця, %', name: 'rh_delta', value: raw.humidifier.settings.rh_delta, step: 1 },
        ],
        action: getUiAction('humidifier.settings.edit'),
      },
    ];
  }

  if (entity === 'light') {
    return [
      { label: 'Режим', controls: [getUiAction('light.mode.auto'), getUiAction('light.mode.manual')] },
      { label: 'Ручний стан', controls: [getUiAction('light.manual.on'), getUiAction('light.manual.off')] },
      { label: 'Поточні налаштування', pairs: [['Режим', modeText(raw.light.settings.mode)], ['Ручний стан', manualStateText(raw.light.settings.manual_state)]] },
      { label: 'Графік', pairs: [['Увімкнення', raw.system.global_context.day_window.time_on], ['Вимкнення', raw.system.global_context.day_window.time_off]], action: getUiAction('light.settings.edit') },
    ];
  }

  if (entity === 'fan') {
    return [
      { label: 'Режим', controls: [getUiAction('fan.mode.auto'), getUiAction('fan.mode.manual')] },
      { label: 'Ручне керування', controls: [getUiAction('fan.manual_run'), getUiAction('fan.stop')] },
      { label: 'Час роботи', controls: [getUiAction('fan.runtime.day'), getUiAction('fan.runtime.always')] },
      { label: 'Стратегія', controls: [getUiAction('fan.strategy.delta'), getUiAction('fan.strategy.timer')] },
      { label: 'Налаштування за різницею', pairs: [['Увімкнення', show(raw.fan.settings.auto_delta_on_pct, '%')], ['Вимкнення', show(raw.fan.settings.auto_delta_off_pct, '%')]], action: getUiAction('fan.settings.edit') },
      { label: 'Налаштування за таймером', pairs: [['Увімкнення', show(raw.fan.settings.auto_timer_on_sec, ' сек')], ['Вимкнення', show(raw.fan.settings.auto_timer_off_sec, ' сек')]], action: getUiAction('fan.settings.edit') },
      { label: 'Ручний запуск', value: show(raw.fan.settings.manual_duration_sec, ' сек'), action: getUiAction('fan.settings.edit') },
    ];
  }

  if (entity === 'connection') {
    return [
      { label: 'Звʼязок', value: 'Користувацьких налаштувань на цьому рівні немає', wide: true },
    ];
  }

  return [];
}

function getAdvancedServiceCards(uiState) {
  const raw = uiState.raw;

  return [
    { label: 'System status', pairs: [['Status', raw.system.status], ['Reason', raw.system.status_reason || '—'], ['Affected', raw.system.affected_system || '—']] },
    { label: 'Maintenance', pairs: [['Active', raw.system.global_context.maintenance.active ? 'yes' : 'no'], ['Reason', raw.system.global_context.maintenance.reason || '—']] },
    { label: 'Day window', pairs: [['Enabled', raw.system.global_context.day_window.schedule_enabled ? 'yes' : 'no'], ['Active', raw.system.global_context.day_window.active ? 'yes' : 'no'], ['On', raw.system.global_context.day_window.time_on], ['Off', raw.system.global_context.day_window.time_off]] },
    { label: 'Connection', pairs: [['Wi-Fi', raw.system.global_context.connection.wifi_state], ['Service', raw.system.global_context.connection.mqtt_state]] },
    { label: 'OTA / оновлення', value: 'Vazon V3 draft', controls: [getUiAction('service.update.check'), getUiAction('service.update.install')] },
    { label: 'Діагностика', value: 'Сховано', action: getUiAction('service.diagnostics.open') },
    { label: 'Status LED', pairs: [['Red', raw.status_led.red_output], ['Green', raw.status_led.green_output], ['Pattern', raw.status_led.pattern]] },
    { label: 'Остання команда', value: 'Команд ще не було', wide: true },
    { label: 'Пристрій', value: 'Vazon V3 prototype' },
  ];
}
