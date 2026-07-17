import { getUiAction } from './ui-actions.js';
import { createMenuView, MENU_LEVELS } from './ui-menu-levels.js';

const noData = '—';

function show(value, suffix = '') {
  if (value === null || value === undefined || value === 'unknown') {
    return noData;
  }

  return `${value}${suffix}`;
}

function secondsToMinutes(value) {
  if (value === null || value === undefined || value === 'unknown') {
    return value;
  }

  return Math.round(value / 60);
}

function showMinutes(value) {
  return show(secondsToMinutes(value), ' хв');
}

function serverStateText(value) {
  if (value === 'connected') return 'Підʼєднано';
  return 'Недоступний';
}

function connectionTileSummary(uiState) {
  return uiState.tiles.find((tile) => tile.entity === 'connection')?.summary || noData;
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
    pause: 'пауза',
    alert: 'увага',
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
    'Система налаштувань',
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

function activeAction(id, active, label = null) {
  return {
    ...getUiAction(id),
    ...(label ? { label } : {}),
    active,
  };
}

function maintenanceAction(active) {
  return {
    id: 'service.maintenance.toggle',
    label: active ? 'Обслуговування активовано' : 'Перейти до обслуговування',
    command: null,
    active,
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

function timeField(label, name, value) {
  return {
    label,
    name,
    value,
    type: 'text',
    inputMode: 'numeric',
    pattern: '[0-9]{2}:[0-9]{2}',
    maxLength: 5,
    size: 5,
    width: '7ch',
  };
}

function fanModeCard(fan) {
  return {
    label: 'Режим',
    sections: [
      {
        label: 'Ручний',
        controls: [
          activeAction('fan.mode.manual', fan.settings.mode === 'manual', 'Ручний'),
          activeAction('fan.manual_run', fan.output === 'on', `Включити - ${showMinutes(fan.settings.manual_duration_sec)}`),
          activeAction('fan.stop', fan.output === 'off', 'Виключити'),
        ],
      },
      {
        label: 'Авто',
        controls: [
          activeAction('fan.mode.auto', fan.settings.mode === 'auto', 'Авто'),
          activeAction('fan.runtime.always', fan.settings.runtime === 'always', 'Цілодобово'),
          activeAction('fan.runtime.day', fan.settings.runtime === 'day', 'Денний'),
        ],
      },
      {
        label: 'Автоматизація',
        columns: [
          {
            control: activeAction('fan.strategy.delta', fan.settings.auto_strategy === 'delta', 'Різниця вологості, %'),
            pairs: [
              ['Включення', show(fan.settings.auto_delta_on_pct, '%')],
              ['Виключення', show(fan.settings.auto_delta_off_pct, '%')],
            ],
          },
          {
            control: activeAction('fan.strategy.timer', fan.settings.auto_strategy === 'timer', 'Таймер, хв'),
            pairs: [
              ['Робота', showMinutes(fan.settings.auto_timer_on_sec)],
              ['Пауза', showMinutes(fan.settings.auto_timer_off_sec)],
            ],
          },
        ],
      },
    ],
    wide: true,
  };
}

function fanPowerCard(fan) {
  const levels = [20, 40, 60, 80, 100];

  return {
    label: 'Інтенсивність',
    controls: levels.map((level) => activeAction(
      `fan.power.${level}`,
      fan.settings.power_level_pct === level,
      `${level}%`,
    )),
    controlClass: 'power-level-row',
    wide: true,
  };
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
          { controls: [getUiAction('light.mode.manual'), getUiAction('light.mode.auto'), getUiAction('light.manual.on'), getUiAction('light.manual.off')] },
          {
            label: 'Графік',
            fields: [
              timeField('Час включення', 'time_on', raw.system.global_context.day_window.time_on),
              timeField('Час відключення', 'time_off', raw.system.global_context.day_window.time_off),
            ],
            controls: [getUiAction('system.day_window.edit')],
          },
        ],
        wide: true,
      },
    ];
  }

  if (entity === 'fan') {
    return [
      fanPowerCard(raw.fan),
      fanModeCard(raw.fan),
      { label: 'Розширені параметри', controls: [settingsOpenAction(entity)], wide: true },
    ];
  }

  if (entity === 'connection') {
    return [
      {
        label: 'Звʼязок',
        pairs: [
          ['Wi-Fi', connectionTileSummary(uiState)],
          ['Сервер', serverStateText(raw.system.global_context.connection.mqtt_state)],
        ],
      },
      { label: 'Актуальність даних', value: raw.system.global_context.connection.mqtt_state === 'connected' ? 'Дані актуальні' : 'Немає актуальних даних' },
    ];
  }

  return [];
}

function getFunctionSettingsCards(entity, uiState) {
  const raw = uiState.raw;

  if (entity === 'climate') {
    return [
      {
        label: 'Датчики',
        pairs: [
          ['Верх стан', raw.climate.sensor_0x44.status],
          ['Верх причина', raw.climate.sensor_0x44.status_reason || '—'],
          ['Низ стан', raw.climate.sensor_0x45.status],
          ['Низ причина', raw.climate.sensor_0x45.status_reason || '—'],
        ],
        wide: true,
      },
      {
        label: 'Налаштування попереджень',
        sections: [
          {
            label: 'Пороги попередження',
            fields: [
              { label: 'Нижній поріг температури', name: 'temperature_low_warn', value: raw.climate.settings.temperature_low_warn, unit: '°C', step: 1 },
              { label: 'Верхній поріг температури', name: 'temperature_high_warn', value: raw.climate.settings.temperature_high_warn, unit: '°C', step: 1 },
              { label: 'Нижній поріг вологості', name: 'humidity_low_warn', value: raw.climate.settings.humidity_low_warn, unit: '%', step: 1 },
              { label: 'Верхній поріг вологості', name: 'humidity_high_warn', value: raw.climate.settings.humidity_high_warn, unit: '%', step: 1 },
            ],
          },
          {
            label: 'Аномальна різниця температур',
            fields: [
              { label: 'Жовте попередження', name: 'temperature_delta_warn', value: raw.climate.settings.temperature_delta_warn, unit: '°C', step: 1 },
              { label: 'Критичне попередження', name: 'temperature_delta_error', value: raw.climate.settings.temperature_delta_error, unit: '°C', step: 1 },
            ],
          },
        ],
        wide: true,
      },
      { bare: true, controls: [getUiAction('climate.thresholds.edit'), getUiAction('navigation.close')] },
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
    return [];
  }

  if (entity === 'fan') {
    return [
      {
        label: 'Налаштування за різницею, %',
        fields: [
          { label: 'Увімкнення', name: 'auto_delta_on_pct', value: raw.fan.settings.auto_delta_on_pct, unit: '%', step: 1 },
          { label: 'Вимкнення', name: 'auto_delta_off_pct', value: raw.fan.settings.auto_delta_off_pct, unit: '%', step: 1 },
        ],
        controls: [getUiAction('fan.settings.edit')],
      },
      {
        label: 'Налаштування за таймером, хв',
        fields: [
          { label: 'Увімкнення', name: 'auto_timer_on_min', value: secondsToMinutes(raw.fan.settings.auto_timer_on_sec), unit: 'хв', step: 1 },
          { label: 'Вимкнення', name: 'auto_timer_off_min', value: secondsToMinutes(raw.fan.settings.auto_timer_off_sec), unit: 'хв', step: 1 },
        ],
        controls: [getUiAction('fan.settings.edit')],
      },
      {
        label: 'Ручний запуск, хв',
        fields: [
          { label: 'Тривалість', name: 'manual_duration_min', value: secondsToMinutes(raw.fan.settings.manual_duration_sec), unit: 'хв', step: 1 },
        ],
        controls: [getUiAction('fan.settings.edit')],
      },
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
  const maintenanceActive = raw.system.global_context.maintenance.active;

  return [
    {
      label: 'Обслуговування',
      value: maintenanceActive ? 'Автоматичне зволоження та вентиляція відключені' : 'Буде відключено автоматичне зволоження та вентиляція',
      controls: [maintenanceAction(maintenanceActive)],
      controlsFirst: true,
      wide: true,
    },
    { label: 'Звʼязок', pairs: [['Wi-Fi', connectionTileSummary(uiState)], ['Сервер', serverStateText(raw.system.global_context.connection.mqtt_state)]] },
    { label: 'OTA / оновлення', value: 'Vazon V3 draft', controls: [getUiAction('service.update.check'), getUiAction('service.update.install')] },
    { label: 'Остання команда', value: 'Команд ще не було', wide: true },
  ];
}
