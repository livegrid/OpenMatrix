const CACHE_NAME = 'livegrid-cache-v1';
const urlsToCache = [
  '/',
  '/index.html',
  '/src/main.js',
  '/public/manifest.json',
  '/public/logo_32.png',
  '/public/logo_192.png',
  '/public/logo_512.png'
  // Add other important assets here
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then((cache) => {
        return cache.addAll(urlsToCache);
      })
  );
});

self.addEventListener('fetch', (event) => {
  event.respondWith(
    caches.match(event.request)
      .then((response) => {
        // Cache hit - return response
        if (response) {
          return response;
        }
        return fetch(event.request);
      })
  );
});
