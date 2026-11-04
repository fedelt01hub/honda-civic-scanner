// Comparación de versiones "1.4.0" para el aviso de actualización del APK.
export function versionFromTag(tag) {
  return String(tag).replace(/^[^0-9]*/, '');
}

export function isNewer(candidate, current) {
  const a = String(candidate).split('.').map(n => parseInt(n, 10) || 0);
  const b = String(current).split('.').map(n => parseInt(n, 10) || 0);
  for (let i = 0; i < Math.max(a.length, b.length); i++) {
    const x = a[i] || 0, y = b[i] || 0;
    if (x !== y) return x > y;
  }
  return false;
}

// Decide qué hacer con el aviso de versión web publicado (apk-bundle.json):
//   'download' | 'none' | 'needsApk' (requiere APK más nuevo) | 'pending' (ya descargada)
export function decideWebUpdate({ currentWeb, native, info, pending }) {
  if (!info || !info.version || !info.url) return 'none';
  if (!isNewer(info.version, versionFromTag(currentWeb))) return 'none';
  if (info.minNative && isNewer(info.minNative, native)) return 'needsApk';
  if (pending === info.version) return 'pending';
  return 'download';
}
