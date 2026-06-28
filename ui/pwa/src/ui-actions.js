const actionRegistry = {
  'climate.thresholds.edit': {
    label: 'Змінити пороги',
    command: {
      target: 'climate',
      cmd: 'set_settings',
      args: {},
    },
  },

  'pot[0].soil_moisture.enable': {
    label: 'Вазон 1 волога ON',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: true },
    },
  },
  'pot[0].soil_moisture.disable': {
    label: 'Вазон 1 волога OFF',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: false },
    },
  },
  'pot[0].soil_temperature.enable': {
    label: 'Вазон 1 темп. ON',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: true },
    },
  },
  'pot[0].soil_temperature.disable': {
    label: 'Вазон 1 темп. OFF',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: false },
    },
  },
  'pot[1].soil_moisture.enable': {
    label: 'Вазон 2 волога ON',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: true },
    },
  },
  'pot[1].soil_moisture.disable': {
    label: 'Вазон 2 волога OFF',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_moisture_enabled',
      args: { enabled: false },
    },
  },
  'pot[1].soil_temperature.enable': {
    label: 'Вазон 2 темп. ON',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: true },
    },
  },
  'pot[1].soil_temperature.disable': {
    label: 'Вазон 2 темп. OFF',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_temperature_enabled',
      args: { enabled: false },
    },
  },
  'pot[0].calibrate_soil_moisture.dry': {
    label: 'P1 сухо',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'dry' },
    },
  },
  'pot[0].calibrate_soil_moisture.normal': {
    label: 'P1 норма',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'normal' },
    },
  },
  'pot[0].calibrate_soil_moisture.wet': {
    label: 'P1 мокро',
    command: {
      target: 'pot/0',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'wet' },
    },
  },
  'pot[1].calibrate_soil_moisture.dry': {
    label: 'P2 сухо',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'dry' },
    },
  },
  'pot[1].calibrate_soil_moisture.normal': {
    label: 'P2 норма',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'normal' },
    },
  },
  'pot[1].calibrate_soil_moisture.wet': {
    label: 'P2 мокро',
    command: {
      target: 'pot/1',
      cmd: 'calibrate_soil_moisture',
      args: { point: 'wet' },
    },
  },
  'pot[0].settings.edit': {
    label: 'P1 settings',
    command: {
      target: 'pot/0',
      cmd: 'set_settings',
      args: {},
    },
  },
  'pot[1].settings.edit': {
    label: 'P2 settings',
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
    label: 'Пуск',
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
    label: 'Low',
    command: {
      target: 'humidifier',
      cmd: 'set_mist_power_level',
      args: { mist_power_level: 'low' },
    },
  },
  'humidifier.power.medium': {
    label: 'Medium',
    command: {
      target: 'humidifier',
      cmd: 'set_mist_power_level',
      args: { mist_power_level: 'medium' },
    },
  },
  'humidifier.power.high': {
    label: 'High',
    command: {
      target: 'humidifier',
      cmd: 'set_mist_power_level',
      args: { mist_power_level: 'high' },
    },
  },
  'humidifier.settings.edit': {
    label: 'Змінити',
    command: {
      target: 'humidifier',
      cmd: 'set_settings',
      args: {},
    },
  },

  'light.mode.auto': {
    label: 'Авто',
    command: {
      target: 'light',
      cmd: 'set_mode',
      args: { mode: 'auto' },
    },
  },
  'light.mode.manual': {
    label: 'Ручний',
    command: {
      target: 'light',
      cmd: 'set_mode',
      args: { mode: 'manual' },
    },
  },
  'light.manual.on': {
    label: 'Увімкнути',
    command: {
      target: 'light',
      cmd: 'set_manual_state',
      args: { state: 'on' },
    },
  },
  'light.manual.off': {
    label: 'Вимкнути',
    command: {
      target: 'light',
      cmd: 'set_manual_state',
      args: { state: 'off' },
    },
  },
  'light.settings.edit': {
    label: 'Змінити',
    command: {
      target: 'light',
      cmd: 'set_settings',
      args: {},
    },
  },

  'fan.mode.auto': {
    label: 'Авто',
    command: {
      target: 'fan',
      cmd: 'set_mode',
      args: { mode: 'auto' },
    },
  },
  'fan.mode.manual': {
    label: 'Ручний',
    command: {
      target: 'fan',
      cmd: 'set_mode',
      args: { mode: 'manual' },
    },
  },
  'fan.runtime.day': {
    label: 'День',
    command: {
      target: 'fan',
      cmd: 'set_runtime',
      args: { runtime: 'day' },
    },
  },
  'fan.runtime.always': {
    label: 'Завжди',
    command: {
      target: 'fan',
      cmd: 'set_runtime',
      args: { runtime: 'always' },
    },
  },
  'fan.strategy.delta': {
    label: 'Delta',
    command: {
      target: 'fan',
      cmd: 'set_auto_strategy',
      args: { auto_strategy: 'delta' },
    },
  },
  'fan.strategy.timer': {
    label: 'Timer',
    command: {
      target: 'fan',
      cmd: 'set_auto_strategy',
      args: { auto_strategy: 'timer' },
    },
  },
  'fan.manual_run': {
    label: 'Пуск',
    command: {
      target: 'fan',
      cmd: 'manual_run',
      args: {},
    },
  },
  'fan.stop': {
    label: 'Стоп',
    command: {
      target: 'fan',
      cmd: 'stop',
      args: {},
    },
  },
  'fan.settings.edit': {
    label: 'Змінити',
    command: {
      target: 'fan',
      cmd: 'set_settings',
      args: {},
    },
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
  const action = actionRegistry[id];

  if (!action) {
    return {
      id,
      label: id,
      command: null,
    };
  }

  return {
    id,
    ...action,
  };
}
