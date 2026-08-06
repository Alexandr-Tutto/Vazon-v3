const CACHE_NAME = 'vazon-v3-pwa-mqtt-prototype-v7';

const APP_ASSETS = [
  './index.html',
  './manifest.webmanifest',
  './src/styles.css',
  './src/app.js',
  './src/mqtt-config.js',
  './src/mqtt/mqtt-client.js',
  './src/mqtt/topic-registry.js',
  './src/state/device-store.js',
  './src/ui-actions.js',
  './src/ui-components.js',
  './src/ui-menu-levels.js',
  './src/ui-panels.js',
  './src/ui-state.js',
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then((cache) => cache.addAll(APP_ASSETS))
      .then(() => self.skipWaiting()),
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys()
      .then((keys) => Promise.all(
        keys.filter((key) => key !== CACHE_NAME).map((key) => caches.delete(key)),
      ))
      .then(() => self.clients.claim()),
  );
});

self.addEventListener('fetch', (event) => {
  if (event.request.method !== 'GET') {
    return;
  }

  const requestUrl = new URL(event.request.url);
  if (requestUrl.origin !== self.location.origin) return;

  event.respondWith(fetch(event.request)
    .then((response) => {
      const cachedResponse = response.clone();
      caches.open(CACHE_NAME).then((cache) => cache.put(event.request, cachedResponse));
      return response;
    })
    .catch(() => caches.match(event.request)));
});
