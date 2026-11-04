// Genera el paquete de actualización de la app web para el APK (sin reinstalar):
//   dist/apk-bundle-<versión>.zip  (contenido de www/)
//   dist/apk-bundle.json           ({ version, url, minNative })
// Se publican copiándolos a la raíz del repo civic-obd (GitHub Pages).
// Correr después de `npm run build:web`.
import { readFile, readdir, writeFile, mkdir, rm } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { zipSync } from 'fflate';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const www = path.join(root, 'www');
const dist = path.join(root, 'dist');
const BASE_URL = 'https://fedelt01hub.github.io/civic-obd/';

const html = await readFile(path.join(www, 'index.html'), 'utf8');
const m = html.match(/const APP_CURRENT_VERSION = 'v([0-9.]+)';/);
if (!m) throw new Error('No encontré APP_CURRENT_VERSION en www/index.html');
const version = m[1];
const pkg = JSON.parse(await readFile(path.join(root, 'package.json'), 'utf8'));
const minNative = pkg.klino && pkg.klino.minNative;
if (!minNative) throw new Error('Falta "klino.minNative" en package.json');

async function collect(dir, prefix = '') {
  const files = {};
  for (const entry of await readdir(dir, { withFileTypes: true })) {
    const rel = prefix + entry.name;
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) Object.assign(files, await collect(full, rel + '/'));
    else files[rel] = new Uint8Array(await readFile(full));
  }
  return files;
}

const files = await collect(www);
if (!files['index.html'] || !files['native-bridge.js']) throw new Error('www/ incompleto: corre npm run build:web');

await rm(dist, { recursive: true, force: true });
await mkdir(dist, { recursive: true });
const zipName = `apk-bundle-${version}.zip`;
await writeFile(path.join(dist, zipName), zipSync(files, { level: 9 }));
const info = { version, url: BASE_URL + zipName, minNative };
await writeFile(path.join(dist, 'apk-bundle.json'), JSON.stringify(info, null, 2) + '\n');
console.log(`dist listo: ${zipName} (${Object.keys(files).length} archivos), minNative ${minNative}`);
