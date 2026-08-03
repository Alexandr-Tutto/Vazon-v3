const DEVICE_ID_PATTERN = /^Vazon_(\d{2})_([0-9a-f]{6})$/;
const TARGET_PATTERN = /^[A-Za-z0-9_-]+(?:\/[A-Za-z0-9_-]+)*$/;

export function subscriptionTopic(topicRoot) {
  return `${topicRoot}/#`;
}

export function parseVazonTopic(topic, topicRoot) {
  const rootParts = topicRoot.split('/');
  const parts = topic.split('/');

  if (parts.length < rootParts.length + 2 ||
      !rootParts.every((part, index) => parts[index] === part)) {
    return null;
  }

  const deviceId = parts[rootParts.length];
  const topicClass = parts[rootParts.length + 1];
  const entity = parts.slice(rootParts.length + 2).join('/');

  return { deviceId, topicClass, entity };
}

export function deviceMatchesSerial(deviceId, serialNumber) {
  const match = DEVICE_ID_PATTERN.exec(deviceId);
  return match !== null && Number(match[1]) === serialNumber;
}

export function commandTopic(topicRoot, deviceId, target) {
  if (!DEVICE_ID_PATTERN.test(deviceId) || !TARGET_PATTERN.test(target)) {
    throw new Error('Invalid MQTT command target');
  }

  return `${topicRoot}/${deviceId}/cmd/${target}`;
}
