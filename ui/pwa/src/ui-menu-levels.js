export const MENU_LEVELS = {
  OVERVIEW: 1,
  FUNCTION_STATUS: 2,
  FUNCTION_SETTINGS: 3,
  ADVANCED_SERVICE: 4,
};

const levelLabels = {
  [MENU_LEVELS.OVERVIEW]: 'Огляд',
  [MENU_LEVELS.FUNCTION_STATUS]: 'Стан підсистеми',
  [MENU_LEVELS.FUNCTION_SETTINGS]: 'Налаштування підсистеми',
  [MENU_LEVELS.ADVANCED_SERVICE]: 'Сервіс / розширено',
};

export function getMenuLevelLabel(level) {
  return levelLabels[level] || 'Меню';
}

export function createMenuView(level, title, cards, context = {}) {
  return {
    level,
    kicker: getMenuLevelLabel(level),
    title,
    cards,
    context,
  };
}
