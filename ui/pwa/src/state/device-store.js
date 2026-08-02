import { deviceMatchesNumber, parseVazonTopic } from '../mqtt/topic-registry.js';

function unknownOutputState() {
  return {
    output: 'unknown',
    status: 'unknown',
    status_reason: null,
    last_command_result: 'unknown',
    last_output_confirmed: 'unknown',
    settings: {},
  };
}

function unknownPotState() {
  return {
    soil_moisture: { value: null, class: 'unknown', status: 'unknown', status_reason: null },
    soil_temperature: { temperature_c: null, status: 'unknown', status_reason: null },
    status: 'unknown',
    status_reason: null,
    settings: {},
  };
}

function createEmptyState() {
  return {
    system: {
      status: 'unknown',
      status_reason: null,
      primary_fault: null,
      affected_system: 'system',
      global_context: {
        maintenance: {
          active: false,
          manual_active: false,
          external_active: false,
          door_active: false,
          reason: null,
        },
        day_window: {
          active: 'unknown',
          schedule_enabled: false,
          time_on: '',
          time_off: '',
        },
        connection: { wifi_state: 'unknown', mqtt_state: 'disconnected' },
        global_interlocks: {
          maintenance_active: false,
          sensor_missing: false,
          actuator_failed: false,
        },
      },
    },
    climate: {
      sensor_0x44: { temperature_c: null, humidity_pct: null, status: 'unknown', status_reason: null },
      sensor_0x45: { temperature_c: null, humidity_pct: null, status: 'unknown', status_reason: null },
      rh_delta_pct: null,
      temperature_delta_c: null,
      status: 'unknown',
      status_reason: null,
      settings: {},
    },
    pot: [unknownPotState(), unknownPotState()],
    door: { state: 'unknown', status: 'unknown', status_reason: null },
    light: unknownOutputState(),
    fan: {
      ...unknownOutputState(),
      applied_power_pct: null,
      auto_state: 'unknown',
    },
    humidifier: {
      water_status: 'unknown',
      mist_output: 'unknown',
      fan_output: 'unknown',
      status: 'unknown',
      status_reason: null,
      settings: {},
    },
    status_led: { red_output: 'unknown', green_output: 'unknown', pattern: 'unknown' },
  };
}

function mergeEntity(target, payload) {
  if (payload && typeof payload === 'object' && !Array.isArray(payload)) {
    Object.assign(target, payload);
    return true;
  }

  return false;
}

export function createDeviceStore({ topicRoot, deviceNumber }) {
  const state = createEmptyState();
  let deviceId = null;
  let lastCommandResult = null;

  function acceptDevice(candidate) {
    if (!deviceMatchesNumber(candidate, deviceNumber)) return false;
    if (deviceId === null) deviceId = candidate;
    return deviceId === candidate;
  }

  function applyMessage(topic, payload) {
    const parsed = parseVazonTopic(topic, topicRoot);
    if (!parsed || !acceptDevice(parsed.deviceId)) return false;

    if (parsed.topicClass === 'availability') {
      state.system.global_context.connection.mqtt_state =
        payload === 'online' ? 'connected' : 'disconnected';
      return true;
    }

    if (['ack', 'reject', 'fail'].includes(parsed.topicClass)) {
      if (!payload || typeof payload !== 'object' || Array.isArray(payload)) {
        return false;
      }

      lastCommandResult = {
        ...payload,
        topic_class: parsed.topicClass,
      };
      return true;
    }

    const { topicClass, entity } = parsed;
    if (!['state', 'status', 'settings'].includes(topicClass)) return false;

    if (entity === 'system') {
      if (topicClass === 'state' && payload && typeof payload === 'object') {
        Object.assign(state.system.global_context, payload);
        return true;
      }
      if (topicClass === 'status') return mergeEntity(state.system, payload);
      if (topicClass === 'settings') {
        return mergeEntity(state.system.global_context.day_window, payload);
      }
    }

    const potMatch = /^pot\/([01])$/.exec(entity);
    const target = potMatch ? state.pot[Number(potMatch[1])] : state[entity];
    if (!target) return false;

    if (topicClass === 'settings') return mergeEntity(target.settings, payload);
    return mergeEntity(target, payload);
  }

  return {
    applyMessage,
    getDeviceId: () => deviceId,
    getLastCommandResult: () => lastCommandResult,
    getState: () => state,
  };
}
