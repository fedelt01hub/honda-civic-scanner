const CACHE_NAME = 'civic-obd-v22';
const ASSETS = [
  './',
  './index.html',
  './manifest.json',
  './icon.svg'
];

self.addEventListener('install', (event) => {
  self.skipWaiting();
  event.waitUntil(
    caches.open(CACHE_NAME).then(async (cache) => {
      for (const asset of ASSETS) {
        try {
          const res = await fetch(asset, { cache: 'reload' });
          if (res.ok) await cache.put(asset, res);
        } catch (e) {
          console.warn('Pre-cache error:', asset, e);
        }
      }
    })
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) return caches.delete(key);
        })
      )
    ).then(() => self.clients.claim())
  );
});

// Network-First para navegación y HTML; Cache-First para recursos estáticos
self.addEventListener('fetch', (event) => {
  const req = event.request;
  const isHtml = req.mode === 'navigate' ||
                 req.headers.get('accept')?.includes('text/html') ||
                 req.url.endsWith('/') ||
                 req.url.endsWith('index.html');

  if (isHtml) {
    event.respondWith(
      fetch(req, { cache: 'no-cache' })
        .then((netRes) => {
          if (netRes && netRes.status === 200) {
            const clone = netRes.clone();
            caches.open(CACHE_NAME).then((cache) => cache.put(req, clone));
          }
          return netRes;
        })
        .catch(() => caches.match(req).then((cached) => cached || caches.match('./index.html') || caches.match('./')))
    );
  } else {
    event.respondWith(
      caches.match(req).then((cached) => {
        if (cached) return cached;
        return fetch(req).then((netRes) => {
          if (netRes && netRes.status === 200) {
            const clone = netRes.clone();
            caches.open(CACHE_NAME).then((cache) => cache.put(req, clone));
          }
          return netRes;
        });
      })
    );
  }
});
