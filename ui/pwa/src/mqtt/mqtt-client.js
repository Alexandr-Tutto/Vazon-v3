import { commandTopic, subscriptionTopic } from './topic-registry.js';

function randomClientId() {
  const random = globalThis.crypto?.randomUUID?.().replaceAll('-', '').slice(0, 12) ||
    Math.random().toString(16).slice(2, 14).padEnd(12, '0');
  return `vazon-pwa-${random}`;
}

function commandId() {
  return globalThis.crypto?.randomUUID?.() ||
    `ui-${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

export function createMqttConnection(config, callbacks = {}) {
  let client = null;
  let connected = false;

  function notifyStatus(status, detail = null) {
    callbacks.onStatus?.(status, detail);
  }

  function connect() {
    if (!globalThis.mqtt?.connect) {
      notifyStatus('error', 'MQTT.js is unavailable');
      return;
    }

    notifyStatus('connecting');
    client = globalThis.mqtt.connect(config.endpoint, {
      username: config.username,
      password: config.password,
      clientId: randomClientId(),
      protocolVersion: 4,
      clean: true,
      keepalive: config.keepaliveSeconds,
      connectTimeout: config.connectTimeoutMs,
      reconnectPeriod: config.reconnectPeriodMs,
      resubscribe: true,
    });

    client.on('connect', () => {
      client.subscribe(subscriptionTopic(config.topicRoot), { qos: config.qos }, (error) => {
        if (error) {
          connected = false;
          notifyStatus('error', error.message);
          return;
        }

        connected = true;
        notifyStatus('connected');
      });
    });

    client.on('reconnect', () => {
      connected = false;
      notifyStatus('reconnecting');
    });

    client.on('offline', () => {
      connected = false;
      notifyStatus('offline');
    });

    client.on('close', () => {
      connected = false;
    });

    client.on('error', (error) => {
      connected = false;
      notifyStatus('error', error.message);
    });

    client.on('message', (topic, payload, packet) => {
      const text = payload.toString();
      let value = text;

      try {
        value = JSON.parse(text);
      } catch {
        // Availability and service messages may intentionally be plain text.
      }

      callbacks.onMessage?.({
        topic,
        payload: value,
        qos: packet.qos,
        retain: packet.retain,
      });
    });
  }

  function publishCommand(deviceId, command) {
    if (!client || !connected) {
      return Promise.reject(new Error('MQTT connection is offline'));
    }

    const topic = commandTopic(config.topicRoot, deviceId, command.target);
    const payload = JSON.stringify({
      cmd_id: commandId(),
      cmd: command.cmd,
      args: command.args || {},
      source: 'ui',
      ts: Date.now(),
    });

    return new Promise((resolve, reject) => {
      client.publish(topic, payload, { qos: config.qos, retain: false }, (error) => {
        if (error) {
          reject(error);
          return;
        }

        resolve();
      });
    });
  }

  function disconnect() {
    connected = false;
    client?.end(true);
    client = null;
  }

  return { connect, disconnect, publishCommand };
}
