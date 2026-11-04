// Arma www/ para el APK: copia ../app y empaqueta el puente nativo.
import { cp, rm } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { build } from 'esbuild';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const www = path.join(root, 'www');

await rm(www, { recursive: true, force: true });
await cp(path.join(root, '..', 'app'), www, { recursive: true });
await rm(path.join(www, 'sw.js'), { force: true }); // el APK no usa service worker
await build({
  entryPoints: [path.join(root, 'src', 'native-entry.mjs')],
  bundle: true,
  format: 'iife',
  target: 'es2020',
  outfile: path.join(www, 'native-bridge.js'),
  logLevel: 'warning'
});
console.log('www listo:', www);
