const actionRegistry = {
  'navigation.close': {
    label: 'Відмінити',
    command: null,
  },

  'climate.thresholds.edit': {
    label: 'Змінити пороги',
    command: {
      target: 'climate',
      cmd: 'set_settings',
      args: {},
    },
  },

  'pot[0].soil_moisture.enable': {
    label: 'Активувати вологу',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: true },
    },
  },
  'pot[0].soil_moisture.disable': {
    label: 'Вимкнути вологу',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: false },
    },
  },
  'pot[0].soil_temperature.enable': {
    label: 'Активувати температуру',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: true },
    },
  },
  'pot[0].soil_temperature.disable': {
    label: 'Вимкнути температуру',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: false },
    },
  },
  'pot[1].soil_moisture.enable': {
    label: 'Активувати вологу',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: true },
    },
  },
  'pot[1].soil_moisture.disable': {
    label: 'Вимкнути вологу',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: false },
    },
  },
  'pot[1].soil_temperature.enable': {
    label: 'Активувати температуру',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: true },
    },
  },
  'pot[1].soil_temperature.disable': {
    label: 'Вимкнути температуру',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: false },
    },
  },
  'pot[0].calibrate_soil_moisture.dry': {
    label: 'Сухо',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'dry' },
    },
  },
  'pot[0].calibrate_soil_moisture.normal': {
    label: 'Норма',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'normal' },
    },
  },
  'pot[0].calibrate_soil_moisture.wet': {
    label: 'Мокро',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'wet' },
    },
  },
  'pot[0].calibrate_soil_moisture.reset': {
    label: 'Скинути',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'reset' },
    },
  },
  'pot[1].calibrate_soil_moisture.dry': {
    label: 'Сухо',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'dry' },
    },
  },
  'pot[1].calibrate_soil_moisture.normal': {
    label: 'Норма',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'normal' },
    },
  },
  'pot[1].calibrate_soil_moisture.wet': {
    label: 'Мокро',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'wet' },
    },
  },
  'pot[1].calibrate_soil_moisture.reset': {
    label: 'Скинути',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'reset' },
    },
  },
  'pot[0].settings.edit': {
    label: 'Змінити параметри',
    command: {
      target: 'pot/0',
      cmd: 'set_settings',
      args: {},
    },
  },
  'pot[1].settings.edit': {
    label: 'Змінити параметри',
    command: {
      target: 'pot/1',
      cmd: 'set_settings',
      args: {},
    },
  },

  'humidifier.mode.auto': {
    label: 'Авто',
    command: {
      target: 'humidifier',
      cmd: 'set_mode',
      args: { mode: 'auto' },
    },
  },
  'humidifier.mode.manual': {
    label: 'Ручний',
    command: {
      target: 'humidifier',
      cmd: 'set_mode',
      args: { mode: 'manual' },
    },
  },
  'humidifier.runtime.day': {
    label: 'День',
    command: {
      target: 'humidifier',
      cmd: 'set_runtime',
      args: { runtime: 'day' },
    },
  },
  'humidifier.runtime.always': {
    label: 'Завжди',
    command: {
      target: 'humidifier',
      cmd: 'set_runtime',
      args: { runtime: 'always' },
    },
  },
  'humidifier.manual_mist': {
    label: 'Пуск пару',
    command: {
      target: 'humidifier',
      cmd: 'manual_mist',
      args: {},
    },
  },
  'humidifier.stop': {
    label: 'Стоп',
    command: {
      target: 'humidifier',
      cmd: 'stop',
      args: {},
    },
  },
  'humidifier.power.low': {
    label: 'Низька',
    command: {
      target: 'humidifier',
      cmd: 'set_mist_power_level',
      args: { mist_power_level: 'low' },
    },
  },
  'humidifier.power.medium': {
    label: 'Помірна',
    command: {
      target: 'humidifier',
      cmd: 'set_mist_power_level',
      args: { mist_power_level: 'medium' },
    },
  },
  'humidifier.power.high': {
    label: 'Висока',
    command: {
      target: 'humidifier',
      cmd: 'set_mist_power_level',
      args: { mist_power_level: 'high' },
    },
  },
  'humidifier.settings.edit': {
    label: 'Змінити параметри',
    command: {
      target: 'humidifier',
      cmd: 'set_settings',
      args: {},
    },
  },

  'light.mode.auto': {
    label: 'Авто',
    command: { target: 'light', cmd: 'set_mode', args: { mode: 'auto' } },
  },
  'light.mode.manual': {
    label: 'Ручний',
    command: { target: 'light', cmd: 'set_mode', args: { mode: 'manual' } },
  },
  'light.manual.on': {
    label: 'Увімкнути',
    command: { target: 'light', cmd: 'set_manual_state', args: { manual_state: 'on' } },
  },
  'light.manual.off': {
    label: 'Вимкнути',
    command: { target: 'light', cmd: 'set_manual_state', args: { manual_state: 'off' } },
  },
  'light.settings.edit': {
    label: 'Змінити графік',
    command: { target: 'light', cmd: 'set_settings', args: {} },
  },

  'fan.mode.auto': {
    label: 'Авто',
    command: { target: 'fan', cmd: 'set_mode', args: { mode: 'auto' } },
  },
  'fan.mode.manual': {
    label: 'Ручний',
    command: { target: 'fan', cmd: 'set_mode', args: { mode: 'manual' } },
  },
  'fan.runtime.day': {
    label: 'День',
    command: { target: 'fan', cmd: 'set_runtime', args: { runtime: 'day' } },
  },
  'fan.runtime.always': {
    label: 'Завжди',
    command: { target: 'fan', cmd: 'set_runtime', args: { runtime: 'always' } },
  },
  'fan.strategy.delta': {
    label: 'За різницею',
    command: { target: 'fan', cmd: 'set_auto_strategy', args: { auto_strategy: 'delta' } },
  },
  'fan.strategy.timer': {
    label: 'За таймером',
    command: { target: 'fan', cmd: 'set_auto_strategy', args: { auto_strategy: 'timer' } },
  },
  'fan.manual_run': {
    label: 'Пуск',
    command: { target: 'fan', cmd: 'manual_run', args: {} },
  },
  'fan.stop': {
    label: 'Стоп',
    command: { target: 'fan', cmd: 'stop', args: {} },
  },
  'fan.settings.edit': {
    label: 'Змінити параметри',
    command: { target: 'fan', cmd: 'set_settings', args: {} },
  },

  'service.update.check': {
    label: 'Перевірити',
    command: null,
  },
  'service.update.install': {
    label: 'Встановити',
    command: null,
  },
  'service.diagnostics.open': {
    label: 'Відкрити',
    command: null,
  },
};

export function getUiAction(id) {
  return {
    id,
    ...actionRegistry[id],
  };
}
