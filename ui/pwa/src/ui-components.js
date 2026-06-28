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

export function statusClass(status) {
  if (status === 'warning') return 'is-warn';
  if (status === 'error') return 'is-error';
  if (status === 'inactive') return 'is-inactive';
  if (status === 'ok') return 'is-ok';
  return 'is-unknown';
}

export function renderTile(tile) {
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

  return button;
}

export function renderMetricCard(title, rows) {
  const article = document.createElement('article');
  article.className = 'metric-card';

  const heading = document.createElement('h2');
  heading.className = 'metric-title';
  heading.textContent = title;
  article.append(heading);

  rows.forEach((row) => {
    const item = document.createElement('div');
    item.className = 'metric-row';

    const label = document.createElement('span');
    label.textContent = row.label;

    const value = document.createElement('strong');
    value.textContent = row.value;

    item.append(label, value);
    article.append(item);
  });

  return article;
}

export function renderActionButton(action) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'ghost-control';
  button.textContent = action.label;
  button.dataset.action = action.id;

  if (action.menuTarget) {
    button.dataset.menuLevel = String(action.menuTarget.level);
    button.dataset.menuEntity = action.menuTarget.entity || '';
  }

  if (action.command) {
    button.dataset.commandTarget = action.command.target;
    button.dataset.commandName = action.command.cmd;
  }

  return button;
}

function renderField(field) {
  const item = document.createElement('label');
  item.className = 'mini-value';

  const fieldLabel = document.createElement('span');
  fieldLabel.textContent = field.label;

  const input = document.createElement('input');
  input.type = 'text';
  input.inputMode = 'decimal';
  input.name = field.name;
  input.value = field.value ?? '';
  input.maxLength = field.maxLength || 5;
  input.size = field.size || 5;
  input.pattern = field.pattern || '[0-9.,-]*';
  input.style.width = field.width || '6ch';
  input.style.maxWidth = '100%';
  input.style.minWidth = '0';
  input.style.justifySelf = 'start';

  if (field.min !== undefined) input.dataset.min = String(field.min);
  if (field.max !== undefined) input.dataset.max = String(field.max);
  if (field.step !== undefined) input.dataset.step = String(field.step);
  if (field.unit) input.dataset.unit = field.unit;

  item.append(fieldLabel, input);
  return item;
}

export function renderCard(card) {
  const article = document.createElement('article');
  article.className = `panel-card${card.wide ? ' is-wide' : ''}`;

  const label = document.createElement('span');
  label.textContent = card.label;
  article.append(label);

  if (card.fields) {
    const fieldWrap = document.createElement('div');
    fieldWrap.className = 'value-pair';
    card.fields.forEach((field) => fieldWrap.append(renderField(field)));
    article.append(fieldWrap);
  } else if (card.pairs) {
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

export function renderCards(cards, targetElement) {
  targetElement.textContent = '';
  cards.map(renderCard).forEach((card) => targetElement.append(card));
}
