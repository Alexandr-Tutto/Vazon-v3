import { getUiAction } from './ui-actions.js';
import { createMenuView, MENU_LEVELS } from './ui-menu-levels.js';

const noData = '—';

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

function getFunctionStatusCards(entity, uiState) {
  const raw = uiState.raw;

  if (entity === 'climate') {
    return [
      { label: 'Температура', pairs: [['Вгорі', uiState.climate.top.temperature_c], ['Внизу', uiState.climate.bottom.temperature_c]] },
      { label: 'Вологість', pairs: [['Вгорі', uiState.climate.top.humidity_pct], ['Внизу', uiState.climate.bottom.humidity_pct]] },
      { label: 'Стан', value: raw.climate.status_reason || 'Норма' },
      { label: 'Параметри', value: 'Пороги різниці температури та вологості', action: settingsOpenAction(entity), wide: true },
    ];
  }

  if (entity === 'pot') {
    return [
      { label: 'Вазон 1', pairs: [['Вологість', `${raw.pot[0].soil_moisture.value_pct}%`], ['Стан', raw.pot[0].soil_moisture.class], ['Темп.', `${raw.pot[0].soil_temperature.temperature_c}°`]] },
      { label: 'Вазон 2', pairs: [['Вологість', `${raw.pot[1].soil_moisture.value_pct}%`], ['Стан', raw.pot[1].soil_moisture.class], ['Темп.', `${raw.pot[1].soil_temperature.temperature_c}°`]] },
      { label: 'Параметри', value: 'Датчики вазонів та калібрування', action: settingsOpenAction(entity), wide: true },
    ];
  }

  if (entity === 'humidifier') {
    return [
      { label: 'Режим', value: raw.humidifier.settings.mode === 'auto' ? 'Автоматичний режим' : 'Ручний режим' },
      { label: 'Вода', value: raw.humidifier.water_status === 'present' ? 'Є вода' : 'Немає води' },
      { label: 'Пар', value: raw.humidifier.mist_output === 'on' ? 'Працює' : 'Вимкнено' },
      { label: 'Локальний вентилятор', value: raw.humidifier.fan_output === 'on' ? 'Працює' : 'Вимкнено' },
      { label: 'Параметри', value: 'Режим, пороги вологості, цикл зволоження', action: settingsOpenAction(entity), wide: true },
    ];
  }

  if (entity === 'light') {
    return [
      { label: 'Поточний стан', value: raw.light.output === 'on' ? 'Світло увімкнене' : 'Світло вимкнене' },
      { label: 'Режим', value: raw.light.settings.mode === 'auto' ? 'Автоматичний режим' : 'Ручний режим' },
      { label: 'Графік', value: `${raw.system.global_context.day_window.time_on} - ${raw.system.global_context.day_window.time_off}` },
      { label: 'Параметри', value: 'Режим і графік світла', action: settingsOpenAction(entity), wide: true },
    ];
  }

  if (entity === 'fan') {
    return [
      { label: 'Поточний стан', value: raw.fan.output === 'on' ? 'Працює' : 'Пауза за режимом' },
      { label: 'Режим', value: raw.fan.settings.mode === 'auto' ? 'Автоматичний режим' : 'Ручний режим' },
      { label: 'Стратегія', value: raw.fan.settings.auto_strategy },
      { label: 'Параметри', value: 'Режим, runtime, стратегія вентиляції', action: settingsOpenAction(entity), wide: true },
    ];
  }

  if (entity === 'connection') {
    return [
      { label: 'Звʼязок', value: raw.system.global_context.connection.wifi_state },
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
      { label: 'Допустима різниця', value: '3°C / 15% RH', action: getUiAction('climate.thresholds.edit') },
      { label: 'Рекомендація', value: 'Основні параметри клімату налаштовуються тут; технічні пороги лишаються у сервісі.', wide: true },
    ];
  }

  if (entity === 'pot') {
    return [
      {
        label: 'Датчики вазонів',
        controls: [
          getUiAction('pot[0].set_soil_moisture_enabled'),
          getUiAction('pot[0].set_soil_temperature_enabled'),
          getUiAction('pot[1].set_soil_moisture_enabled'),
          getUiAction('pot[1].set_soil_temperature_enabled'),
        ],
      },
      {
        label: 'Як калібрувати',
        instructions: [
          'Встановіть датчик у ємність із таким самим сухим грунтом. Зачекайте 1 хвилину, натисніть "Сухо".',
          'Долийте води до бажаного оптимального стану. Зачекайте 1 хвилину, натисніть "Норма".',
          'Перелийте води, зробіть грунт вологішим, ніж потрібно. Зачекайте 1 хвилину, натисніть "Мокро".',
        ],
        note: 'Калібрування закінчено. Переставте датчик у бажаний вазон обережно, не пошкоджуючи коріння.',
        wide: true,
      },
      {
        label: 'Калібрування',
        controls: [
          getUiAction('pot[0].calibrate_soil_moisture.dry'),
          getUiAction('pot[0].calibrate_soil_moisture.normal'),
          getUiAction('pot[0].calibrate_soil_moisture.wet'),
          getUiAction('pot[0].calibration.reset'),
        ],
        wide: true,
      },
      { label: 'Останнє калібрування', value: 'Не змінювалось' },
    ];
  }

  if (entity === 'humidifier') {
    return [
      {
        label: 'Керування режимом',
        controls: [
          getUiAction('humidifier.mode.auto'),
          getUiAction('humidifier.mode.manual'),
          getUiAction('humidifier.manual_mist'),
          getUiAction('humidifier.stop'),
        ],
        wide: true,
      },
      { label: 'Пороги вологості', value: `Старт ${raw.humidifier.settings.rh_start}%, стоп ${raw.humidifier.settings.rh_stop}%`, action: getUiAction('humidifier.settings.edit') },
      { label: 'Цикл', value: `${raw.humidifier.settings.manual_mist_duration_sec} сек робота / ${raw.humidifier.settings.post_fan_sec} сек продув`, action: getUiAction('humidifier.settings.edit') },
    ];
  }

  if (entity === 'light') {
    return [
      {
        label: 'Режим',
        controls: [
          getUiAction('light.mode.auto'),
          getUiAction('light.manual.on'),
          getUiAction('light.manual.off'),
        ],
      },
      { label: 'Графік', value: `${raw.system.global_context.day_window.time_on} - ${raw.system.global_context.day_window.time_off}`, action: getUiAction('light.schedule.edit') },
    ];
  }

  if (entity === 'fan') {
    return [
      {
        label: 'Режим',
        controls: [
          getUiAction('fan.mode.auto'),
          getUiAction('fan.mode.manual'),
          getUiAction('fan.runtime.day'),
        ],
      },
      { label: 'Стратегія', value: raw.fan.settings.auto_strategy, action: getUiAction('fan.settings.edit') },
      { label: 'Цикл', value: raw.fan.settings.auto_strategy === 'timer' ? `${raw.fan.settings.auto_timer_on_sec} / ${raw.fan.settings.auto_timer_off_sec} сек` : noData, action: getUiAction('fan.settings.edit') },
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
  return [
    { label: 'Клімат', value: '3°C / 15% RH', action: getUiAction('climate.thresholds.edit') },
    { label: 'Зволоження', value: `${uiState.raw.humidifier.settings.rh_start}% / ${uiState.raw.humidifier.settings.rh_stop}%`, action: getUiAction('humidifier.settings.edit') },
    { label: 'Цикл зволоження', value: `${uiState.raw.humidifier.settings.manual_mist_duration_sec} сек / ${uiState.raw.humidifier.settings.post_fan_sec} сек`, action: getUiAction('humidifier.settings.edit') },
    { label: 'Світло', value: `${uiState.raw.system.global_context.day_window.time_on} - ${uiState.raw.system.global_context.day_window.time_off}`, action: getUiAction('light.schedule.edit') },
    { label: 'Вентиляція', value: noData, action: getUiAction('fan.settings.edit') },
    { label: 'Оновлення', value: 'Vazon V3 draft', controls: [getUiAction('service.update.check'), getUiAction('service.update.install')] },
    { label: 'Діагностика', value: 'Сховано', action: getUiAction('service.diagnostics.open') },
    { label: 'Остання команда', value: 'Команд ще не було', wide: true },
    { label: 'Пристрій', value: 'Vazon V3 prototype' },
  ];
}
