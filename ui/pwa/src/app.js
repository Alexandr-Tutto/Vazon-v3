import { mockState } from './mock-state.js';
import { createUiState } from './ui-state.js';

const uiState = createUiState(mockState);

const elements = {
  mainStatus: document.getElementById('mainStatus'),
  statusGrid: document.getElementById('statusGrid'),
  metricGrid: document.getElementById('metricGrid'),
  detailPanel: document.getElementById('detailPanel'),
  detailKicker: document.getElementById('detailKicker'),
  detailTitle: document.getElementById('detailTitle'),
  detailBody: document.getElementById('detailBody'),
  closeButton: document.getElementById('closeButton'),
  serviceButton: document.getElementById('serviceButton'),
};

const icons = {
  climate: `
    <span class="status-icon icon-climate">
      <svg viewBox="0 0 64 64" aria-hidden="true">
        <path d="M28 10a8 8 0 0 1 16 0v25a16 16 0 1 1-16 0Z"></path>
        <path d="M36 14v30"></path>
        <circle cx="36" cy="46" r="6"></circle>
      </svg>
    </span>
  `,
  pot: `
    <span class="status-icon icon-soil">
      <svg viewBox="0 0 64 64" aria-hidden="true">
        <path d="M13 47c10-7 28-7 38 0"></path>
        <path d="M18 39c8-5 20-5 28 0"></path>
        <path d="M32 34V15"></path>
        <path d="M32 22c-9 0-14-5-15-13 8 0 14 4 15 13Z"></path>
        <path d="M32 28c9 0 14-5 15-13-8 0-14 4-15 13Z"></path>
      </svg>
    </span>
  `,
  humidifier: `
    <span class="status-icon icon-mist">
      <svg viewBox="0 0 64 64" aria-hidden="true">
        <path class="mist-drop" d="M20 10c-6 9-9 14-9 20a9 9 0 0 0 18 0c0-6-3-11-9-20Z"></path>
        <path class="mist-drop" d="M44 10c-6 9-9 14-9 20a9 9 0 0 0 18 0c0-6-3-11-9-20Z"></path>
        <path class="mist-drop" d="M32 34c-5 7-7 11-7 15a7 7 0 0 0 14 0c0-4-2-8-7-15Z"></path>
      </svg>
    </span>
  `,
  light: `
    <span class="status-icon icon-light">
      <svg viewBox="0 0 64 64" aria-hidden="true">
        <circle cx="32" cy="32" r="12"></circle>
        <path d="M32 4v10M32 50v10M4 32h10M50 32h10M12.2 12.2l7.1 7.1M44.7 44.7l7.1 7.1M51.8 12.2l-7.1 7.1M19.3 44.7l-7.1 7.1"></path>
      </svg>
    </span>
  `,
  fan: `
    <span class="status-icon icon-fan">
      <svg viewBox="0 0 64 64" aria-hidden="true">
        <g class="fan-blades">
          <circle cx="32" cy="32" r="5"></circle>
          <path d="M32 27c-2-9 3-18 12-20 6 8 3 18-7 24M37 35c9 2 15 10 13 19-10 2-18-4-19-15M27 35c-7 6-17 6-23 0 4-9 14-12 23-6"></path>
        </g>
        <path d="M45 22h12M45 32h15M45 42h10"></path>
      </svg>
    </span>
  `,
  connection: `
    <span class="wifi-bars level-4" aria-hidden="true"><span></span><span></span><span></span><span></span><span></span></span>
  `,
};

function statusClass(status) {
  if (status === 'warning') return 'is-warn';
  if (status === 'error') return 'is-error';
  if (status === 'inactive') return 'is-inactive';
  if (status === 'ok') return 'is-ok';
  return 'is-unknown';
}

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
  elements.statusGrid.innerHTML = '';

  state.tiles.forEach((tile) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = `status-tile ${statusClass(tile.status)} ${tile.active ? 'is-running' : ''}`;
    button.dataset.entity = tile.entity;
    button.setAttribute('aria-label', tile.title);

    button.innerHTML = `
      ${icons[tile.entity] || ''}
      <span class="tile-label">
        <span class="tile-title">${tile.title}</span>
        <span class="tile-state">${tile.summary}</span>
      </span>
    `;

    button.addEventListener('click', () => openDetail(tile.entity));
    elements.statusGrid.appendChild(button);
  });
}

function renderMetrics(state) {
  const climate = state.climate;

  elements.metricGrid.innerHTML = `
    <article class="metric-card">
      <h2 class="metric-title">Температура</h2>
      <div class="metric-row"><span>Вгорі</span><strong>${climate.top.temperature_c}</strong></div>
      <div class="metric-row"><span>Внизу</span><strong>${climate.bottom.temperature_c}</strong></div>
    </article>

    <article class="metric-card">
      <h2 class="metric-title">Вологість</h2>
      <div class="metric-row"><span>Вгорі</span><strong>${climate.top.humidity_pct}</strong></div>
      <div class="metric-row"><span>Внизу</span><strong>${climate.bottom.humidity_pct}</strong></div>
    </article>
  `;
}

function openDetail(entity) {
  const detail = uiState.details[entity];

  elements.detailKicker.textContent = 'Деталі';
  elements.detailTitle.textContent = detail.title;
  elements.detailBody.innerHTML = `
    <article class="panel-card is-wide">
      <strong>${detail.summary}</strong>
      <p>${detail.message}</p>
    </article>
    ${detail.rows.map((row) => `
      <article class="panel-card">
        <strong>${row.value}</strong>
        <span>${row.label}</span>
      </article>
    `).join('')}
  `;

  document.body.classList.add('has-panel');
}

function openService() {
  elements.detailKicker.textContent = 'Сервіс';
  elements.detailTitle.textContent = 'Стан UI';
  elements.detailBody.innerHTML = `
    <article class="panel-card is-wide">
      <strong>Mock state</strong>
      <span>Реального MQTT-зʼєднання ще немає</span>
    </article>
    <article class="panel-card is-wide">
      <pre class="debug-state">${JSON.stringify(uiState.raw, null, 2)}</pre>
    </article>
  `;

  document.body.classList.add('has-panel');
}

function closePanel() {
  document.body.classList.remove('has-panel');
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
elements.serviceButton.addEventListener('click', openService);

render();
registerServiceWorker();
