import { mockState } from './mock-state.js';
import { createUiState } from './ui-state.js';
import { renderCards, renderMetricCard, renderTile } from './ui-components.js';
import { MENU_LEVELS } from './ui-menu-levels.js';
import { getAdvancedServiceView, getFunctionSettingsView, getFunctionStatusView } from './ui-panels.js';

const uiState = createUiState(mockState);
let currentView = null;

function applyFanPresentation(state) {
  const fanTile = state.tiles.find((tile) => tile.entity === 'fan');

  if (fanTile && state.raw.fan.status_reason) {
    fanTile.summary = 'Увага';
  }

  if (state.details.fan && state.raw.fan.status_reason === 'door_open') {
    state.details.fan.summary = '- заблоковано через відкриті двері';
  }
}

applyFanPresentation(uiState);

const elements = {
  mainStatus: document.getElementById('mainStatus'),
  statusGrid: document.getElementById('statusGrid'),
  metricGrid: document.getElementById('metricGrid'),
  detailKicker: document.getElementById('detailKicker'),
  detailTitle: document.getElementById('detailTitle'),
  detailBody: document.getElementById('detailBody'),
  closeButton: document.getElementById('closeButton'),
  serviceButton: document.getElementById('serviceButton'),
};

function renderMainStatus(state) {
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

function openFunctionSettings(entity) {
  openView(getFunctionSettingsView(entity, uiState));
}

function openAdvancedService() {
  openView(getAdvancedServiceView(uiState));
}

function closePanel() {
  currentView = null;
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

function toggleMaintenanceMode() {
  const maintenance = uiState.raw.system.global_context.maintenance;
  maintenance.active = !maintenance.active;
  maintenance.reason = maintenance.active ? 'manual' : null;
  openAdvancedService();
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

function buildCommandPayload(actionButton) {
  const target = actionButton.dataset.commandTarget;
  const cmd = actionButton.dataset.commandName;
  let args = {
    ...readStaticCommandArgs(actionButton),
    ...readFieldArgs(actionButton),
  };

  if (target === 'fan' && cmd === 'set_settings') {
    args = normalizeFanSettingsArgs(args);
  }

  return {
    target,
    cmd,
    args,
  };
}

function emitCommand(actionButton) {
  const payload = buildCommandPayload(actionButton);

  if (!payload.target || !payload.cmd) {
    return false;
  }

  elements.detailBody.dispatchEvent(new CustomEvent('vazon-command', {
    bubbles: true,
    detail: payload,
  }));

  return true;
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
    toggleMaintenanceMode();
    return;
  }

  emitCommand(actionButton);
}

function registerServiceWorker() {
  if (!('serviceWorker' in navigator)) {
    return;
  }

  navigator.serviceWorker.register('./sw.js').catch(() => {
    // Service worker registration failure is non-critical for UI skeleton.
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
elements.detailBody.addEventListener('focusin', rememberInputStartValue);
document.addEventListener('keydown', handleEscape);

render();
registerServiceWorker();
