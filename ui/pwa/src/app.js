import { mockState } from './mock-state.js';
import { createUiState } from './ui-state.js';

const uiState = createUiState(mockState);

const screens = {
  overview: document.getElementById('overviewScreen'),
  detail: document.getElementById('detailScreen'),
  service: document.getElementById('serviceScreen'),
};

const elements = {
  connectionText: document.getElementById('connectionText'),
  mainMessage: document.getElementById('mainMessage'),
  tileGrid: document.getElementById('tileGrid'),
  climateNumbers: document.getElementById('climateNumbers'),
  detailTitle: document.getElementById('detailTitle'),
  detailBody: document.getElementById('detailBody'),
  debugState: document.getElementById('debugState'),
  backButton: document.getElementById('backButton'),
  serviceButton: document.getElementById('serviceButton'),
  serviceBackButton: document.getElementById('serviceBackButton'),
};

function showScreen(name) {
  Object.values(screens).forEach((screen) => screen.classList.remove('active-screen'));
  screens[name].classList.add('active-screen');
}

function statusClass(status) {
  return `status-${status || 'unknown'}`;
}

function renderMainMessage(state) {
  const status = state.system.status;
  const affected = state.system.affected_system_label;

  elements.mainMessage.className = `main-message ${statusClass(status)}`;

  if (status === 'ok') {
    elements.mainMessage.textContent = 'Все добре';
    return;
  }

  if (status === 'warning') {
    elements.mainMessage.textContent = `Потрібна увага: ${affected}`;
    return;
  }

  if (status === 'error') {
    elements.mainMessage.textContent = `Потрібна дія: ${affected}`;
    return;
  }

  elements.mainMessage.textContent = 'Немає актуальних даних';
}

function renderTiles(state) {
  elements.tileGrid.innerHTML = '';

  state.tiles.forEach((tile) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = `tile ${statusClass(tile.status)} ${tile.active ? 'tile-active' : ''}`;
    button.dataset.entity = tile.entity;

    button.innerHTML = `
      <span class="tile-title">${tile.title}</span>
      <span class="tile-state">${tile.summary}</span>
    `;

    button.addEventListener('click', () => openDetail(tile.entity));
    elements.tileGrid.appendChild(button);
  });
}

function renderClimate(state) {
  const climate = state.climate;

  elements.climateNumbers.innerHTML = `
    <div class="number-card">
      <span class="number-label">Температура вгорі</span>
      <span class="number-value">${climate.top.temperature_c}</span>
    </div>
    <div class="number-card">
      <span class="number-label">Температура внизу</span>
      <span class="number-value">${climate.bottom.temperature_c}</span>
    </div>
    <div class="number-card">
      <span class="number-label">Вологість вгорі</span>
      <span class="number-value">${climate.top.humidity_pct}</span>
    </div>
    <div class="number-card">
      <span class="number-label">Вологість внизу</span>
      <span class="number-value">${climate.bottom.humidity_pct}</span>
    </div>
  `;
}

function renderOverview() {
  elements.connectionText.textContent = uiState.connectionLabel;
  renderMainMessage(uiState);
  renderTiles(uiState);
  renderClimate(uiState);
  elements.debugState.textContent = JSON.stringify(uiState.raw, null, 2);
}

function openDetail(entity) {
  const detail = uiState.details[entity];

  elements.detailTitle.textContent = detail.title;
  elements.detailBody.innerHTML = `
    <section class="detail-card ${statusClass(detail.status)}">
      <h2>${detail.summary}</h2>
      <p>${detail.message}</p>
      <dl>
        ${detail.rows.map((row) => `<dt>${row.label}</dt><dd>${row.value}</dd>`).join('')}
      </dl>
    </section>
  `;

  showScreen('detail');
}

function registerServiceWorker() {
  if (!('serviceWorker' in navigator)) {
    return;
  }

  navigator.serviceWorker.register('./sw.js').catch(() => {
    // Service worker registration failure is non-critical for UI skeleton.
  });
}

elements.backButton.addEventListener('click', () => showScreen('overview'));
elements.serviceBackButton.addEventListener('click', () => showScreen('overview'));
elements.serviceButton.addEventListener('click', () => showScreen('service'));

renderOverview();
registerServiceWorker();
