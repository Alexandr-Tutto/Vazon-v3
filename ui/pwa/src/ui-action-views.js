import { getUiAction } from './ui-actions.js';

function formatCommand(action) {
  if (!action.command) {
    return 'Команда не формується на цьому етапі';
  }

  return `${action.command.target} / ${action.command.cmd}`;
}

function formatArgs(action) {
  if (!action.command || !action.command.args || Object.keys(action.command.args).length === 0) {
    return 'Без аргументів';
  }

  return Object.entries(action.command.args)
    .map(([key, value]) => `${key}: ${value}`)
    .join(', ');
}

export function getActionView(actionId) {
  const action = getUiAction(actionId);

  return {
    kicker: 'Дія',
    title: action.label,
    cards: [
      {
        label: 'UI action',
        value: action.id,
      },
      {
        label: 'Майбутня команда',
        value: formatCommand(action),
      },
      {
        label: 'Аргументи',
        value: formatArgs(action),
      },
      {
        label: 'Стан виконання',
        value: 'Команда ще не відправляється. MQTT transport не підключений.',
        wide: true,
      },
      {
        label: 'Навігація',
        controls: [getUiAction('navigation.back')],
      },
    ],
  };
}
