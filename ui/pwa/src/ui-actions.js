const actionRegistry = {
  'climate.thresholds.edit': {
    label: 'Змінити',
    command: {
      target: 'climate',
      cmd: 'set_settings',
      args: {},
    },
  },

  'pot[0].set_soil_moisture_enabled': {
    label: 'Вазон 1 волога',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_moisture_enabled',
      args: {},
    },
  },
  'pot[0].set_soil_temperature_enabled': {
    label: 'Вазон 1 температура',
    command: {
      target: 'pot/0',
      cmd: 'set_soil_temperature_enabled',
      args: {},
    },
  },
  'pot[1].set_soil_moisture_enabled': {
    label: 'Вазон 2 волога',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_moisture_enabled',
      args: {},
    },
  },
  'pot[1].set_soil_temperature_enabled': {
    label: 'Вазон 2 температура',
    command: {
      target: 'pot/1',
      cmd: 'set_soil_temperature_enabled',
      args: {},
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
  'pot[0].calibration.reset': {
    label: 'Скинути',
    command: {
      target: 'pot/0',
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
  'light.schedule.edit': {
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
    label: 'За світлом',
    command: {
      target: 'fan',
      cmd: 'set_runtime',
      args: { runtime: 'day' },
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
