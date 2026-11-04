const CACHE_NAME = 'civic-obd-v1.2';
const ASSETS = [
  './',
  './index.html',
  './manifest.json',
  './icon-192.png',
  './icon-512.png',
  './icon.png',
  './icon.svg',
  './fonts/BarlowCondensed-SemiBold.woff2',
  './vtec-rev.mp3'
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
          console.warn('[SW] Pre-cache error:', asset, e);
        }
      }
    })
  );
});

// Permite que la página pida activar de inmediato un SW nuevo en espera
self.addEventListener('message', (event) => {
  if (event.data && event.data.type === 'SKIP_WAITING') self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(
        keys.map((key) => {
          if (key !== CACHE_NAME) {
            console.log('[SW] Deleting old cache:', key);
            return caches.delete(key);
          }
        })
      )
    ).then(() => self.clients.claim())
  );
});

// Cache-First con Stale-While-Revalidate para navegación y assets
// Permite que la app abra instantáneamente al 100% sin datos/wifi
self.addEventListener('fetch', (event) => {
  const req = event.request;
  if (req.method !== 'GET') return;

  const isHtml = req.mode === 'navigate' ||
                 req.headers.get('accept')?.includes('text/html') ||
                 req.url.endsWith('/') ||
                 req.url.endsWith('index.html');

  if (isHtml) {
    event.respondWith(
      caches.match(req).then((cached) => {
        // En segundo plano intentamos actualizar la caché si hay conexión
        const fetchPromise = fetch(req, { cache: 'no-cache' })
          .then((netRes) => {
            if (netRes && netRes.status === 200) {
              const clone = netRes.clone();
              caches.open(CACHE_NAME).then((cache) => cache.put(req, clone));
            }
            return netRes;
          })
          .catch(() => null);

        // Si tenemos caché (modo sin datos/wifi), responder inmediatamente
        if (cached) return cached;

        // Si no está en caché directa, intentar network o fallback a index.html en caché
        return fetchPromise.then((netRes) => {
          if (netRes) return netRes;
          return caches.match('./index.html').then((fallback) => fallback || caches.match('./'));
        });
      })
    );
  } else {
    // Recursos estáticos (imágenes, manifest, iconos)
    event.respondWith(
      caches.match(req).then((cached) => {
        if (cached) return cached;
        return fetch(req).then((netRes) => {
          if (netRes && netRes.status === 200) {
            const clone = netRes.clone();
            caches.open(CACHE_NAME).then((cache) => cache.put(req, clone));
          }
          return netRes;
        }).catch(() => {
          // Si falla y es un icono, devolver icon-192.png si está en caché
          if (req.url.includes('icon')) {
            return caches.match('./icon-192.png');
          }
        });
      })
    );
  }
});
