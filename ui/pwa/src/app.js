import { mqttConfig } from './mqtt-config.js';
import { createMqttConnection } from './mqtt/mqtt-client.js';
import { parseVazonTopic } from './mqtt/topic-registry.js';
import { createDeviceStore } from './state/device-store.js';
import { createUiState } from './ui-state.js';
import { renderCards, renderMetricCard, renderTile } from './ui-components.js';
import { MENU_LEVELS } from './ui-menu-levels.js';
import { getAdvancedServiceView, getFunctionSettingsView, getFunctionStatusView } from './ui-panels.js';

const deviceStore = createDeviceStore(mqttConfig);

function currentUiState() {
  return {
    ...createUiState(deviceStore.getState()),
    lastCommandResult: deviceStore.getLastCommandResult(),
  };
}

let uiState = currentUiState();
let currentView = null;
let settingsDraftDirty = false;
let pendingSettingsRequest = null;

const elements = {
  mainStatus: document.getElementById('mainStatus'),
  serverStatus: document.getElementById('serverStatus'),
  statusGrid: document.getElementById('statusGrid'),
  metricGrid: document.getElementById('metricGrid'),
  detailKicker: document.getElementById('detailKicker'),
  detailTitle: document.getElementById('detailTitle'),
  detailBody: document.getElementById('detailBody'),
  closeButton: document.getElementById('closeButton'),
  serviceButton: document.getElementById('serviceButton'),
};

function setServerStatus(status) {
  const labels = {
    connecting: 'Сервер: підключення…',
    connected: deviceStore.getDeviceId()
      ? `Сервер: підключено, контролер №${mqttConfig.serialNumber}`
      : `Сервер: підключено, контролер №${mqttConfig.serialNumber} не знайдено`,
    reconnecting: 'Сервер: відновлення звʼязку…',
    'device-offline': `Сервер: підключено, контролер №${mqttConfig.serialNumber} недоступний`,
    offline: 'Сервер: немає звʼязку',
    error: 'Сервер: помилка підключення',
  };

  elements.serverStatus.textContent = labels[status] || labels.offline;
  elements.serverStatus.className = `server-status is-${status}`;
}

function renderMainStatus(state) {
  if (state.deviceOffline) {
    elements.mainStatus.textContent = 'Пристрій не відповідає';
    return;
  }

  const maintenance = state.raw.system.global_context.maintenance;

  if (maintenance?.active === true) {
    if (maintenance.door_active === true) {
      elements.mainStatus.textContent = state.raw.door.state === 'open'
        ? 'Відкриті двері — обслуговування'
        : 'Стан дверей невідомий — обслуговування';
    } else {
      elements.mainStatus.textContent = 'Режим обслуговування';
    }
    return;
  }

  const status = state.system.status;
  const affected = state.system.affected_system_label;

  if (status === 'ok') {
    elements.mainStatus.textContent = 'Все добре';
    return;
  }

  if (status === 'warning') {
    elements.mainStatus.textContent = `Потрібна увага: ${affected.toLowerCase()}`;
    return;
  }

  if (status === 'error') {
    elements.mainStatus.textContent = `Потрібна дія: ${affected.toLowerCase()}`;
    return;
  }

  elements.mainStatus.textContent = 'Немає актуальних даних';
}

function renderTiles(state) {
  elements.statusGrid.textContent = '';

  state.tiles.forEach((tile) => {
    const button = renderTile(tile);
    button.addEventListener('click', () => openFunctionStatus(tile.entity));
    elements.statusGrid.appendChild(button);
  });
}

function renderMetrics(state) {
  const climate = state.climate;

  elements.metricGrid.textContent = '';
  elements.metricGrid.append(
    renderMetricCard('Температура', [
      { label: 'Вгорі', value: climate.top.temperature_c },
      { label: 'Внизу', value: climate.bottom.temperature_c },
    ]),
    renderMetricCard('Вологість', [
      { label: 'Вгорі', value: climate.top.humidity_pct },
      { label: 'Внизу', value: climate.bottom.humidity_pct },
    ]),
  );
}

function openView(view) {
  currentView = view;
  elements.detailKicker.textContent = view.kicker;
  elements.detailTitle.textContent = view.title;
  renderCards(view.cards, elements.detailBody);
  document.body.classList.add('has-panel');
}

function openFunctionStatus(entity) {
  openView(getFunctionStatusView(entity, uiState));
}

function openFunctionSettings(entity, resetDraft = true) {
  if (resetDraft) settingsDraftDirty = false;
  openView(getFunctionSettingsView(entity, uiState));
}

function openAdvancedService() {
  openView(getAdvancedServiceView(uiState));
}

function setActionActiveState(actionId, active) {
  const button = Array.from(elements.detailBody.querySelectorAll('[data-action]'))
    .find((candidate) => candidate.dataset.action === actionId);
  if (!button) return;

  button.classList.toggle('is-active', active);
  if (active) {
    button.setAttribute('aria-pressed', 'true');
    button.style.borderColor = 'var(--ok)';
    button.style.color = 'var(--ok)';
    return;
  }

  button.removeAttribute('aria-pressed');
  button.style.removeProperty('border-color');
  button.style.removeProperty('color');
}

function setActionLabel(actionId, label) {
  const button = Array.from(elements.detailBody.querySelectorAll('[data-action]'))
    .find((candidate) => candidate.dataset.action === actionId);
  if (button) button.textContent = label;
}

function refreshDirtySettingsControls(entity) {
  if (entity !== 'humidifier') return;

  const humidifier = uiState.raw.humidifier;
  const manualRunActive = humidifier.settings.mode === 'manual'
    && humidifier.fan_output === 'on';
  const postRunActive = humidifier.fan_output === 'post_run';
  const states = {
    'humidifier.mode.auto': humidifier.settings.mode === 'auto',
    'humidifier.mode.manual': humidifier.settings.mode === 'manual',
    'humidifier.runtime.day': humidifier.settings.runtime === 'day',
    'humidifier.runtime.always': humidifier.settings.runtime === 'always',
    'humidifier.power.low': humidifier.settings.mist_power_level === 'low',
    'humidifier.power.medium': humidifier.settings.mist_power_level === 'medium',
    'humidifier.power.high': humidifier.settings.mist_power_level === 'high',
    'humidifier.manual_mist': manualRunActive,
    'humidifier.stop': humidifier.fan_output === 'off' || postRunActive,
  };

  Object.entries(states).forEach(([actionId, active]) => {
    setActionActiveState(actionId, active);
  });
  setActionLabel(
    'humidifier.stop',
    postRunActive ? 'Стоп — післяпродувка' : 'Стоп',
  );
}

function refreshOpenView() {
  if (!currentView) return;

  const entity = currentView.context?.entity;
  if (currentView.level === MENU_LEVELS.FUNCTION_STATUS && entity) {
    if (settingsDraftDirty || hasFocusedPanelInput()) return;
    openFunctionStatus(entity);
  } else if (currentView.level === MENU_LEVELS.FUNCTION_SETTINGS && entity) {
    if (settingsDraftDirty || hasFocusedPanelInput()) {
      refreshDirtySettingsControls(entity);
      return;
    }
    openFunctionSettings(entity, false);
  } else if (currentView.level === MENU_LEVELS.ADVANCED_SERVICE) {
    openAdvancedService();
  }
}

function closePanel() {
  currentView = null;
  settingsDraftDirty = false;
  pendingSettingsRequest = null;
  document.body.classList.remove('has-panel');
}

function openParentView() {
  if (!currentView) {
    return;
  }

  if (currentView.level === MENU_LEVELS.FUNCTION_SETTINGS && currentView.context.entity) {
    openFunctionStatus(currentView.context.entity);
    return;
  }

  closePanel();
}

function isTextInput(element) {
  return element instanceof HTMLInputElement || element instanceof HTMLTextAreaElement;
}

function hasFocusedPanelInput() {
  return isTextInput(document.activeElement) &&
    elements.detailBody.contains(document.activeElement);
}

function rememberInputStartValue(event) {
  if (!isTextInput(event.target)) {
    return;
  }

  event.target.dataset.editStartValue = event.target.value;
}

function cancelInputEdit(input) {
  input.value = input.dataset.editStartValue ?? input.value;
  delete input.dataset.editStartValue;
  input.blur();
}

function handleEscape(event) {
  if (event.key !== 'Escape') {
    return;
  }

  if (isTextInput(event.target)) {
    event.preventDefault();
    event.stopPropagation();
    cancelInputEdit(event.target);
    return;
  }

  if (!currentView) {
    return;
  }

  event.preventDefault();
  openParentView();
}

function toggleMaintenanceMode(actionButton) {
  emitCommand(actionButton);
}

function markSettingsDraftDirty(event) {
  if (currentView && isTextInput(event.target)) {
    event.target.setCustomValidity('');
    normalizeDayWindowInput(event.target);
    settingsDraftDirty = true;
  }
}

function actionReadsFieldArgs(actionButton) {
  const target = actionButton.dataset.commandTarget;
  const cmd = actionButton.dataset.commandName;
  return cmd === 'set_settings' ||
    (target === 'system' && cmd === 'set_day_window');
}

function readStaticCommandArgs(actionButton) {
  if (!actionButton.dataset.commandArgs) {
    return {};
  }

  try {
    return JSON.parse(actionButton.dataset.commandArgs);
  } catch {
    return {};
  }
}

function readFieldArgs(actionButton) {
  const scope = actionButton.closest('.card-section') || actionButton.closest('.panel-card');

  if (!scope) {
    return {};
  }

  return Array.from(scope.querySelectorAll('input[name]')).reduce((args, input) => {
    args[input.name] = input.value;
    return args;
  }, {});
}

function normalizeFanSettingsArgs(args) {
  const normalized = { ...args };
  const minuteFields = {
    auto_timer_on_min: 'auto_timer_on_sec',
    auto_timer_off_min: 'auto_timer_off_sec',
    manual_duration_min: 'manual_duration_sec',
  };

  Object.entries(minuteFields).forEach(([source, target]) => {
    if (!(source in normalized)) return;

    const minutes = Number(String(normalized[source]).replace(',', '.'));
    normalized[target] = Number.isFinite(minutes) ? Math.round(minutes * 60) : normalized[source];
    delete normalized[source];
  });

  ['auto_delta_on_pct', 'auto_delta_off_pct'].forEach((field) => {
    if (!(field in normalized)) return;

    const value = Number(String(normalized[field]).replace(',', '.'));
    if (Number.isFinite(value)) normalized[field] = value;
  });

  return normalized;
}

function normalizeHumidifierSettingsArgs(args) {
  const normalized = { ...args };

  ['rh_start', 'rh_stop', 'rh_delta'].forEach((field) => {
    if (!(field in normalized)) return;

    const value = Number(String(normalized[field]).replace(',', '.'));
    if (Number.isFinite(value)) normalized[field] = value;
  });

  ['manual_mist_duration_sec', 'post_fan_sec'].forEach((field) => {
    if (!(field in normalized)) return;

    const value = Number(String(normalized[field]).replace(',', '.'));
    if (Number.isFinite(value)) normalized[field] = Math.round(value);
  });

  return normalized;
}

function normalizeTimeText(value) {
  const match = /^(\d{2})\D(\d{2})$/.exec(value);
  if (!match) return null;

  if (Number(match[1]) > 23 || Number(match[2]) > 59) return null;
  return `${match[1]}:${match[2]}`;
}

function normalizeDayWindowInput(input) {
  if (!['time_on', 'time_off'].includes(input.name)) return;

  const normalized = normalizeTimeText(input.value);
  if (normalized !== null) input.value = normalized;
}

function validateActionFields(actionButton) {
  const scope = actionButton.closest('.card-section') || actionButton.closest('.panel-card');
  if (!scope) return true;

  const target = actionButton.dataset.commandTarget;
  const cmd = actionButton.dataset.commandName;
  const inputs = Array.from(scope.querySelectorAll('input[name]'));

  for (const input of inputs) {
    input.setCustomValidity('');
    if (target === 'system' && cmd === 'set_day_window' &&
        ['time_on', 'time_off'].includes(input.name)) {
      const normalized = normalizeTimeText(input.value);
      if (normalized === null) {
        input.setCustomValidity('Вкажіть коректний час у форматі ГГ:ХХ; розділювач може бути будь-яким');
      } else {
        input.value = normalized;
      }
    }
    if (!input.checkValidity()) {
      input.reportValidity();
      return false;
    }
  }

  return true;
}

function normalizeClimateSettingsArgs(args) {
  const normalized = { ...args };

  [
    'temperature_low_warn',
    'temperature_high_warn',
    'humidity_low_warn',
    'humidity_high_warn',
    'temperature_delta_warn',
    'temperature_delta_error',
    'rh_offset_0x44_pct',
    'rh_offset_0x45_pct',
  ].forEach((field) => {
    if (!(field in normalized)) return;

    const value = Number(String(normalized[field]).replace(',', '.'));
    if (Number.isFinite(value)) normalized[field] = value;
  });

  return normalized;
}

function buildCommandPayload(actionButton) {
  const target = actionButton.dataset.commandTarget;
  const cmd = actionButton.dataset.commandName;
  let args = readStaticCommandArgs(actionButton);

  if (actionReadsFieldArgs(actionButton)) {
    args = {
      ...args,
      ...readFieldArgs(actionButton),
    };
  }

  if (target === 'fan' && cmd === 'set_settings') {
    args = normalizeFanSettingsArgs(args);
  }
  if (target === 'climate' && cmd === 'set_settings') {
    args = normalizeClimateSettingsArgs(args);
  }
  if (target === 'humidifier' && cmd === 'set_settings') {
    args = normalizeHumidifierSettingsArgs(args);
  }

  return {
    target,
    cmd,
    args,
  };
}

function isUnchangedDayWindowCommand(payload) {
  if (payload.target !== 'system' || payload.cmd !== 'set_day_window') {
    return false;
  }

  const confirmed = deviceStore.getState().system.global_context.day_window;
  return Object.entries(payload.args)
    .every(([key, value]) => key in confirmed && String(confirmed[key]) === String(value));
}

function emitCommand(actionButton) {
  if (!validateActionFields(actionButton)) {
    return null;
  }

  const payload = buildCommandPayload(actionButton);

  if (!payload.target || !payload.cmd) {
    return null;
  }

  if (isUnchangedDayWindowCommand(payload)) {
    settingsDraftDirty = false;
    pendingSettingsRequest = null;
    refreshFromDeviceStore();
    return null;
  }

  elements.detailBody.dispatchEvent(new CustomEvent('vazon-command', {
    bubbles: true,
    detail: payload,
  }));

  return payload;
}

function handlePanelClick(event) {
  const actionButton = event.target.closest('[data-action]');

  if (!actionButton) {
    return;
  }

  const action = actionButton.dataset.action;
  const targetLevel = Number(actionButton.dataset.menuLevel || 0);
  const targetEntity = actionButton.dataset.menuEntity || '';

  if (targetLevel === MENU_LEVELS.FUNCTION_SETTINGS && targetEntity) {
    openFunctionSettings(targetEntity);
    return;
  }

  if (targetLevel === MENU_LEVELS.FUNCTION_STATUS && targetEntity) {
    openFunctionStatus(targetEntity);
    return;
  }

  if (action === 'navigation.close') {
    closePanel();
    return;
  }

  if (action === 'service.maintenance.toggle') {
    toggleMaintenanceMode(actionButton);
    return;
  }

  const commandPayload = emitCommand(actionButton);
  if (commandPayload && actionReadsFieldArgs(actionButton)) {
    settingsDraftDirty = true;
    pendingSettingsRequest = {
      target: commandPayload.target,
      cmd: commandPayload.cmd,
      expectedArgs: commandPayload.args,
    };
  }
}

function settingsPayloadMatchesPending(payload) {
  if (!pendingSettingsRequest || !payload || typeof payload !== 'object') {
    return false;
  }

  return Object.entries(pendingSettingsRequest.expectedArgs)
    .every(([key, value]) => key in payload && String(payload[key]) === String(value));
}

function registerServiceWorker() {
  if (!('serviceWorker' in navigator)) {
    return;
  }

  navigator.serviceWorker.register('./sw.js').catch(() => {
    // Service worker registration failure is non-critical for UI skeleton.
  });
}

function refreshFromDeviceStore() {
  uiState = currentUiState();
  render();
  refreshOpenView();
}

const mqttConnection = createMqttConnection(mqttConfig, {
  onStatus: (status) => setServerStatus(status),
  onMessage: ({ topic, payload }) => {
    if (!deviceStore.applyMessage(topic, payload)) return;
    const parsed = parseVazonTopic(topic, mqttConfig.topicRoot);
    if (settingsDraftDirty && pendingSettingsRequest &&
        parsed?.topicClass === 'settings' &&
        parsed.entity === pendingSettingsRequest.target &&
        settingsPayloadMatchesPending(payload)) {
      settingsDraftDirty = false;
      pendingSettingsRequest = null;
    } else if (settingsDraftDirty && pendingSettingsRequest &&
               ['reject', 'fail'].includes(parsed?.topicClass) &&
               payload?.target === pendingSettingsRequest.target &&
               payload?.cmd === pendingSettingsRequest.cmd) {
      settingsDraftDirty = false;
      pendingSettingsRequest = null;
    }
    refreshFromDeviceStore();
    setServerStatus(
      deviceStore.getState().system.global_context.connection.mqtt_state === 'connected'
        ? 'connected'
        : 'device-offline',
    );
  },
});

function sendMqttCommand(event) {
  const deviceId = deviceStore.getDeviceId();
  if (deviceStore.getState().system.global_context.connection.mqtt_state !==
      'connected') {
    setServerStatus('device-offline');
    return;
  }
  if (!deviceId) {
    setServerStatus('connected');
    return;
  }

  mqttConnection.publishCommand(deviceId, event.detail).catch(() => {
    setServerStatus('error');
  });
}

function render() {
  renderMainStatus(uiState);
  renderTiles(uiState);
  renderMetrics(uiState);
}

elements.closeButton.addEventListener('click', closePanel);
elements.serviceButton.addEventListener('click', openAdvancedService);
elements.detailBody.addEventListener('click', handlePanelClick);
elements.detailBody.addEventListener('vazon-command', sendMqttCommand);
elements.detailBody.addEventListener('focusin', rememberInputStartValue);
elements.detailBody.addEventListener('input', markSettingsDraftDirty);
document.addEventListener('keydown', handleEscape);

render();
registerServiceWorker();
mqttConnection.connect();
