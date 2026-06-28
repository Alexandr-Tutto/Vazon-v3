import { mockState } from './mock-state.js';
import { createUiState } from './ui-state.js';
import { renderCards, renderMetricCard, renderTile } from './ui-components.js';
import { MENU_LEVELS } from './ui-menu-levels.js';
import { getAdvancedServiceView, getFunctionSettingsView, getFunctionStatusView } from './ui-panels.js';

const uiState = createUiState(mockState);
let currentView = null;

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
  }
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

render();
registerServiceWorker();
