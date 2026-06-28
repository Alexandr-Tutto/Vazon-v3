import { mockState } from './mock-state.js';
import { createUiState } from './ui-state.js';

const uiState = createUiState(mockState);
const noData = '—';

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

function renderActionButton(action) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'ghost-control';
  button.textContent = action.label;
  button.dataset.action = action.action;
  return button;
}

function renderCard(card) {
  const article = document.createElement('article');
  article.className = `panel-card${card.wide ? ' is-wide' : ''}`;

  const label = document.createElement('span');
  label.textContent = card.label;
  article.append(label);

  if (card.pairs) {
    const pairWrap = document.createElement('div');
    pairWrap.className = 'value-pair';
    card.pairs.forEach((pair) => {
      const mini = document.createElement('div');
      mini.className = 'mini-value';
      const miniLabel = document.createElement('span');
      miniLabel.textContent = pair[0];
      const value = document.createElement('b');
      value.textContent = pair[1];
      mini.append(miniLabel, value);
      pairWrap.append(mini);
    });
    article.append(pairWrap);
  } else if (card.controls) {
    if (card.value) {
      const value = document.createElement('strong');
      value.textContent = card.value;
      article.append(value);
    }
    const row = document.createElement('div');
    row.className = 'control-row';
    card.controls.forEach((control) => row.append(renderActionButton(control)));
    article.append(row);
  } else if (card.instructions) {
    const list = document.createElement('ol');
    list.className = 'instruction-list';
    card.instructions.forEach((item) => {
      const listItem = document.createElement('li');
      listItem.textContent = item;
      list.append(listItem);
    });
    article.append(list);

    if (card.note) {
      const note = document.createElement('p');
      note.className = 'instruction-note';
      note.textContent = card.note;
      article.append(note);
    }
  } else {
    const value = document.createElement('strong');
    value.textContent = card.value || '';
    article.append(value);
  }

  if (card.action) {
    article.append(renderActionButton(card.action));
  }

  return article;
}

function getPanelCards(entity) {
  const raw = uiState.raw;

  if (entity === 'climate') {
    return [
      { label: 'Температура', pairs: [['Вгорі', uiState.climate.top.temperature_c], ['Внизу', uiState.climate.bottom.temperature_c]] },
      { label: 'Вологість', pairs: [['Вгорі', uiState.climate.top.humidity_pct], ['Внизу', uiState.climate.bottom.humidity_pct]] },
      { label: 'Допустима різниця', value: '3°C / 15% RH', action: { label: 'Змінити', action: 'climate.thresholds.edit' } },
    ];
  }

  if (entity === 'pot') {
    return [
      { label: 'Вазон 1', pairs: [['Вологість', `${raw.pot[0].soil_moisture.value_pct}%`], ['Стан', raw.pot[0].soil_moisture.class], ['Темп.', `${raw.pot[0].soil_temperature.temperature_c}°`]] },
      { label: 'Вазон 2', pairs: [['Вологість', `${raw.pot[1].soil_moisture.value_pct}%`], ['Стан', raw.pot[1].soil_moisture.class], ['Темп.', `${raw.pot[1].soil_temperature.temperature_c}°`]] },
      {
        label: 'Датчики вазонів',
        controls: [
          { label: 'Вазон 1 волога', action: 'pot[0].set_soil_moisture_enabled' },
          { label: 'Вазон 1 температура', action: 'pot[0].set_soil_temperature_enabled' },
          { label: 'Вазон 2 волога', action: 'pot[1].set_soil_moisture_enabled' },
          { label: 'Вазон 2 температура', action: 'pot[1].set_soil_temperature_enabled' },
        ],
      },
      {
        label: 'Як калібрувати',
        instructions: [
          'Встановіть датчик у ємність із таким самим сухим грунтом. Зачекайте 1 хвилину, натисніть "Сухо".',
          'Долийте води до бажаного оптимального стану. Зачекайте 1 хвилину, натисніть "Норма".',
          'Перелийте води, зробіть грунт вологішим, ніж потрібно. Зачекайте 1 хвилину, натисніть "Мокро".',
        ],
        note: 'Калібрування закінчено. Переставте датчик у бажаний вазон обережно, не пошкоджуючи коріння.',
        wide: true,
      },
      {
        label: 'Калібрування',
        controls: [
          { label: 'Сухо', action: 'pot[0].calibrate_soil_moisture' },
          { label: 'Норма', action: 'pot[0].calibrate_soil_moisture' },
          { label: 'Мокро', action: 'pot[0].calibrate_soil_moisture' },
          { label: 'Скинути', action: 'pot[0].set_settings' },
        ],
        wide: true,
      },
      { label: 'Останнє калібрування', value: 'Не змінювалось' },
    ];
  }

  if (entity === 'humidifier') {
    return [
      { label: 'Режим', value: raw.humidifier.settings.mode === 'auto' ? 'Автоматичний режим' : 'Ручний режим' },
      { label: 'Вода', value: raw.humidifier.water_status === 'present' ? 'Є вода' : 'Немає води' },
      {
        label: 'Керування',
        controls: [
          { label: 'Авто', action: 'humidifier.set_mode' },
          { label: 'Ручний', action: 'humidifier.set_mode' },
          { label: 'Пуск', action: 'humidifier.manual_mist' },
          { label: 'Стоп', action: 'humidifier.stop' },
        ],
        wide: true,
      },
      { label: 'Пороги вологості', value: `Старт ${raw.humidifier.settings.rh_start}%, стоп ${raw.humidifier.settings.rh_stop}%`, action: { label: 'Змінити', action: 'humidifier.set_settings' } },
      { label: 'Цикл', value: `${raw.humidifier.settings.manual_mist_duration_sec} сек робота / ${raw.humidifier.settings.post_fan_sec} сек продув`, action: { label: 'Змінити', action: 'humidifier.set_settings' } },
    ];
  }

  if (entity === 'light') {
    return [
      { label: 'Поточний стан', value: raw.light.output === 'on' ? 'Світло увімкнене' : 'Світло вимкнене' },
      { label: 'Графік', value: `${raw.system.global_context.day_window.time_on} - ${raw.system.global_context.day_window.time_off}`, action: { label: 'Змінити', action: 'light.schedule.edit' } },
      {
        label: 'Режим',
        controls: [
          { label: 'Авто', action: 'light.set_mode' },
          { label: 'Увімкнути', action: 'light.set_manual_state' },
          { label: 'Вимкнути', action: 'light.set_manual_state' },
        ],
      },
    ];
  }

  if (entity === 'fan') {
    return [
      { label: 'Поточний стан', value: raw.fan.output === 'on' ? 'Працює' : 'Пауза за режимом' },
      { label: 'Цикл', value: raw.fan.settings.auto_strategy === 'timer' ? `${raw.fan.settings.auto_timer_on_sec} / ${raw.fan.settings.auto_timer_off_sec} сек` : noData, action: { label: 'Змінити', action: 'fan.set_settings' } },
      {
        label: 'Режим',
        controls: [
          { label: 'Авто', action: 'fan.set_mode' },
          { label: 'Ручний', action: 'fan.set_mode' },
          { label: 'За світлом', action: 'fan.set_runtime' },
        ],
      },
    ];
  }

  if (entity === 'connection') {
    return [
      { label: 'Звʼязок', value: raw.system.global_context.connection.wifi_state },
      { label: 'Останній сигнал', value: 'Добрий, щойно' },
      { label: 'Сервіс', value: raw.system.global_context.connection.mqtt_state === 'connected' ? 'Підключено' : 'Недоступний' },
    ];
  }

  return [];
}

function openDetail(entity) {
  const detail = uiState.details[entity];

  elements.detailKicker.textContent = 'Деталі';
  elements.detailTitle.textContent = `${detail.title} ${detail.summary || ''}`;
  elements.detailBody.textContent = '';
  getPanelCards(entity).map(renderCard).forEach((card) => elements.detailBody.append(card));

  document.body.classList.add('has-panel');
}

function openService() {
  elements.detailKicker.textContent = 'Сервіс';
  elements.detailTitle.textContent = 'Налаштування';
  elements.detailBody.textContent = '';

  const cards = [
    { label: 'Клімат', value: '3°C / 15% RH', action: { label: 'Змінити', action: 'climate.thresholds.edit' } },
    { label: 'Зволоження', value: `${uiState.raw.humidifier.settings.rh_start}% / ${uiState.raw.humidifier.settings.rh_stop}%`, action: { label: 'Змінити', action: 'humidifier.set_settings' } },
    { label: 'Цикл зволоження', value: `${uiState.raw.humidifier.settings.manual_mist_duration_sec} сек / ${uiState.raw.humidifier.settings.post_fan_sec} сек`, action: { label: 'Змінити', action: 'humidifier.set_settings' } },
    { label: 'Світло', value: `${uiState.raw.system.global_context.day_window.time_on} - ${uiState.raw.system.global_context.day_window.time_off}`, action: { label: 'Змінити', action: 'light.schedule.edit' } },
    { label: 'Вентиляція', value: noData, action: { label: 'Змінити', action: 'fan.set_settings' } },
    { label: 'Оновлення', value: 'Vazon V3 draft', controls: [{ label: 'Перевірити', action: 'service.update.check' }, { label: 'Встановити', action: 'service.update.install' }] },
    { label: 'Діагностика', value: 'Сховано', action: { label: 'Відкрити', action: 'service.diagnostics.open' } },
    { label: 'Остання команда', value: 'Команд ще не було', wide: true },
    { label: 'Пристрій', value: 'Vazon V3 prototype' },
  ];

  cards.map(renderCard).forEach((card) => elements.detailBody.append(card));
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
