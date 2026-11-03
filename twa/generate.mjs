// Genera el proyecto Android (TWA) en twa/android a partir de twa/twa-manifest.json.
// Uso: node twa/generate.mjs [versionCode] [versionName]
// No es interactivo, así que sirve igual en la PC y en GitHub Actions.
import path from 'node:path';
import fs from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { TwaManifest, TwaGenerator, ConsoleLog } from '@bubblewrap/core';

const here = path.dirname(fileURLToPath(import.meta.url));
const outDir = path.join(here, 'android');

const manifest = await TwaManifest.fromFile(path.join(here, 'twa-manifest.json'));
const [versionCode, versionName] = process.argv.slice(2);
if (versionCode) manifest.appVersionCode = Number(versionCode);
if (versionName) manifest.appVersionName = versionName;

// Vaciar el contenido (no la carpeta en sí, por si alguna terminal está parada dentro)
await fs.mkdir(outDir, { recursive: true });
for (const entry of await fs.readdir(outDir)) {
  await fs.rm(path.join(outDir, entry), { recursive: true, force: true });
}
await new TwaGenerator().createTwaProject(outDir, manifest, new ConsoleLog('twa'));
console.log(`Proyecto TWA generado en ${outDir} (versionCode ${manifest.appVersionCode}, ${manifest.appVersionName})`);
