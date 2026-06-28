import { getUiAction } from './ui-actions.js';

const noData = '—';

export function getPanelCards(entity, uiState) {
  const raw = uiState.raw;

  if (entity === 'climate') {
    return [
      { label: 'Температура', pairs: [['Вгорі', uiState.climate.top.temperature_c], ['Внизу', uiState.climate.bottom.temperature_c]] },
      { label: 'Вологість', pairs: [['Вгорі', uiState.climate.top.humidity_pct], ['Внизу', uiState.climate.bottom.humidity_pct]] },
      { label: 'Допустима різниця', value: '3°C / 15% RH', action: getUiAction('climate.thresholds.edit') },
    ];
  }

  if (entity === 'pot') {
    return [
      { label: 'Вазон 1', pairs: [['Вологість', `${raw.pot[0].soil_moisture.value_pct}%`], ['Стан', raw.pot[0].soil_moisture.class], ['Темп.', `${raw.pot[0].soil_temperature.temperature_c}°`]] },
      { label: 'Вазон 2', pairs: [['Вологість', `${raw.pot[1].soil_moisture.value_pct}%`], ['Стан', raw.pot[1].soil_moisture.class], ['Темп.', `${raw.pot[1].soil_temperature.temperature_c}°`]] },
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
      { label: 'Режим', value: raw.humidifier.settings.mode === 'auto' ? 'Автоматичний режим' : 'Ручний режим' },
      { label: 'Вода', value: raw.humidifier.water_status === 'present' ? 'Є вода' : 'Немає води' },
      {
        label: 'Керування',
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
      { label: 'Поточний стан', value: raw.light.output === 'on' ? 'Світло увімкнене' : 'Світло вимкнене' },
      { label: 'Графік', value: `${raw.system.global_context.day_window.time_on} - ${raw.system.global_context.day_window.time_off}`, action: getUiAction('light.schedule.edit') },
      {
        label: 'Режим',
        controls: [
          getUiAction('light.mode.auto'),
          getUiAction('light.manual.on'),
          getUiAction('light.manual.off'),
        ],
      },
    ];
  }

  if (entity === 'fan') {
    return [
      { label: 'Поточний стан', value: raw.fan.output === 'on' ? 'Працює' : 'Пауза за режимом' },
      { label: 'Цикл', value: raw.fan.settings.auto_strategy === 'timer' ? `${raw.fan.settings.auto_timer_on_sec} / ${raw.fan.settings.auto_timer_off_sec} сек` : noData, action: getUiAction('fan.settings.edit') },
      {
        label: 'Режим',
        controls: [
          getUiAction('fan.mode.auto'),
          getUiAction('fan.mode.manual'),
          getUiAction('fan.runtime.day'),
        ],
      },
    ];
  }

  if (entity === 'connection') {
    return [
      { label: 'Звʼязок', value: raw.system.global_context.connection.wifi_state },
      { label: 'Останній сигнал', value: 'Добрий, щойно' },
      { label: 'Сервіс', value: raw.system.global_context.connection.mqtt_state === 'connected' ? 'Підключено' : 'Недоступний' },
    ];
  }

  return [];
}

export function getServiceCards(uiState) {
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
