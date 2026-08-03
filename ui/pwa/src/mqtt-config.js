export const mqttConfig = Object.freeze({
  endpoint: 'wss://fc1e785f4f774c3cae2ea6cce1d5a607.s1.eu.hivemq.cloud:8884/mqtt',
  username: 'vazon-ui-dev',
  password: 'vazon-ui-dev',
  topicRoot: 'vazon/v3',
  serialNumber: 2,
  qos: 1,
  keepaliveSeconds: 30,
  connectTimeoutMs: 10000,
  reconnectPeriodMs: 3000,
});
