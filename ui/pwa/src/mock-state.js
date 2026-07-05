export const mockState = {
  system: {
    status: 'warning',
    status_reason: 'door_open',
    primary_fault: null,
    affected_system: 'door',
    global_context: {
      maintenance: {
        active: false,
        reason: null,
      },
      day_window: {
        active: true,
        schedule_enabled: true,
        time_on: '08:00',
        time_off: '22:00',
      },
      connection: {
        wifi_state: 'connected',
        mqtt_state: 'connected',
      },
    },
  },

  climate: {
    sensor_0x44: {
      temperature_c: 28.1,
      humidity_pct: 61,
      status: 'ok',
      status_reason: null,
    },
    sensor_0x45: {
      temperature_c: 23.4,
      humidity_pct: 78,
      status: 'ok',
      status_reason: null,
    },
    rh_delta_pct: 17,
    temperature_delta_c: 4.7,
    status: 'warning',
    status_reason: 'rh_delta_high',
    settings: {},
  },

  pot: [
    {
      soil_moisture: {
        raw_adc_value: 1900,
        raw_mv: 1510,
        value: 62,
        class: 'normal',
        status: 'ok',
        status_reason: null,
      },
      soil_temperature: {
        temperature_c: 22.8,
        status: 'ok',
        status_reason: null,
      },
      status: 'ok',
      status_reason: null,
      settings: {
        soil_moisture_enabled: true,
        soil_temperature_enabled: true,
      },
    },
    {
      soil_moisture: {
        raw_adc_value: 2020,
        raw_mv: 1580,
        value: 55,
        class: 'normal',
        status: 'ok',
        status_reason: null,
      },
      soil_temperature: {
        temperature_c: 23.1,
        status: 'ok',
        status_reason: null,
      },
      status: 'ok',
      status_reason: null,
      settings: {
        soil_moisture_enabled: true,
        soil_temperature_enabled: true,
      },
    },
  ],

  door: {
    state: 'open',
    status: 'ok',
    status_reason: null,
  },

  light: {
    output: 'on',
    status: 'ok',
    status_reason: null,
    last_command_result: 'accepted',
    last_output_confirmed: 'unknown',
    settings: {
      mode: 'auto',
      manual_state: 'off',
    },
  },

  fan: {
    output: 'off',
    auto_state: 'blocked',
    status: 'warning',
    status_reason: 'door_open',
    last_command_result: 'accepted',
    last_output_confirmed: 'unknown',
    settings: {
      mode: 'auto',
      runtime: 'day',
      auto_strategy: 'delta',
      manual_duration_sec: 600,
      auto_delta_on_pct: 10,
      auto_delta_off_pct: 5,
      auto_timer_on_sec: 60,
      auto_timer_off_sec: 300,
    },
  },

  humidifier: {
    water_status: 'present',
    mist_output: 'off',
    fan_output: 'off',
    status: 'warning',
    status_reason: 'door_open',
    settings: {
      mode: 'auto',
      runtime: 'day',
      rh_start: 55,
      rh_stop: 75,
      rh_delta: 10,
      mist_power_level: 'medium',
      manual_mist_duration_sec: 180,
      post_fan_sec: 20,
    },
  },

  status_led: {
    red_output: 'off',
    green_output: 'on',
    pattern: 'local_warning',
  },
};
