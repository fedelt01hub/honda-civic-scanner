// Puente nativo de K-Lino. Se empaqueta a www/native-bridge.js (esbuild) y solo se
// carga dentro del APK. Expone window.KLinoNative para app/index.html.
import { BleClient } from '@capacitor-community/bluetooth-le';
import { KeepAwake } from '@capacitor-community/keep-awake';
import { Filesystem, Directory, Encoding } from '@capacitor/filesystem';
import { Share } from '@capacitor/share';
import { App } from '@capacitor/app';
import { CapacitorUpdater } from '@capgo/capacitor-updater';
import { isNewer, versionFromTag, decideWebUpdate } from './version.mjs';

const KEY_ID = 'klinoDeviceId';
const KEY_NAME = 'klinoDeviceName';
const RELEASES_URL = 'https://api.github.com/repos/fedelt01hub/honda-civic-scanner/releases/latest';
const WEB_BUNDLE_URL = 'https://fedelt01hub.github.io/civic-obd/apk-bundle.json';
const KEY_PENDING = 'klinoPendingWeb';

// Avisa que esta versión de la app arrancó bien. Si una versión descargada no llega
// a este punto en 10 s, el plugin regresa sola a la anterior.
CapacitorUpdater.notifyAppReady().catch(() => {});

let bleReady = false;
async function ensureBle() {
  if (!bleReady) {
    await BleClient.initialize({ androidNeverForLocation: true });
    bleReady = true;
  }
  if (!(await BleClient.isEnabled())) await BleClient.requestEnable(); // diálogo de Android para encender Bluetooth
}

// Escaneo para el selector propio de index.html. Devuelve una función para detenerlo.
async function startScan(namePrefix, onDevice) {
  await ensureBle();
  await BleClient.requestLEScan({ namePrefix }, (r) => {
    onDevice({ deviceId: r.device.deviceId, name: r.device.name || r.localName || namePrefix, rssi: r.rssi });
  });
  return () => { BleClient.stopLEScan().catch(() => {}); };
}

// Misma interfaz que WebBleTransport en index.html:
//   connect(onNotify(DataView), onDisconnect()) -> nombre; write(Uint8Array); disconnect(); reset(); forget()
// pick(): selector propio (index.html); sin él se usa el diálogo genérico del plugin.
function createBle({ service, tx, rx, namePrefix, pick }) {
  let deviceId = null;
  return {
    async connect(onNotify, onDisconnect) {
      await ensureBle();
      deviceId = localStorage.getItem(KEY_ID);
      let name = localStorage.getItem(KEY_NAME) || 'K-Lino';
      if (!deviceId) {
        const dev = pick ? await pick() : await BleClient.requestDevice({ namePrefix, services: [service] });
        deviceId = dev.deviceId;
        name = dev.name || name;
        localStorage.setItem(KEY_ID, deviceId);
        localStorage.setItem(KEY_NAME, name);
      }
      // El K-Lino con llave en OFF solo anuncia 3 s de cada 18 s: timeout amplio
      await BleClient.connect(deviceId, () => onDisconnect(), { timeout: 20000 });
      await BleClient.startNotifications(deviceId, service, tx, (value) => onNotify(value));
      return name;
    },
    async write(bytes) {
      await BleClient.write(deviceId, service, rx, new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength));
    },
    disconnect() {
      if (deviceId) BleClient.disconnect(deviceId).catch(() => {});
    },
    reset() {},
    forget() {
      localStorage.removeItem(KEY_ID);
      localStorage.removeItem(KEY_NAME);
    }
  };
}

async function keepAwake(on) {
  if (on) await KeepAwake.keepAwake();
  else await KeepAwake.allowSleep();
}

async function shareCsv(fileName, text) {
  const { uri } = await Filesystem.writeFile({
    path: fileName, data: text, directory: Directory.Cache, encoding: Encoding.UTF8
  });
  await Share.share({ title: fileName, files: [uri] });
}

async function checkApkUpdate() {
  const { version } = await App.getInfo();
  const res = await fetch(RELEASES_URL, { headers: { Accept: 'application/vnd.github+json' } });
  if (!res.ok) throw new Error('GitHub respondió ' + res.status);
  const rel = await res.json();
  const latest = versionFromTag(rel.tag_name);
  const apk = (rel.assets || []).find(a => a.name.endsWith('.apk'));
  return { current: version, latest, newer: isNewer(latest, version), url: apk ? apk.browser_download_url : rel.html_url };
}

// Actualización de la app web dentro del APK (sin reinstalar). applyNow: recarga ya;
// si no, se aplica la próxima vez que se abre la app.
async function updateWeb(currentWeb, { applyNow = false } = {}) {
  const { version: native } = await App.getInfo();
  const res = await fetch(WEB_BUNDLE_URL + '?t=' + Date.now(), { cache: 'no-store' });
  if (!res.ok) throw new Error('apk-bundle.json respondió ' + res.status);
  const info = await res.json();
  const action = decideWebUpdate({ currentWeb, native, info, pending: localStorage.getItem(KEY_PENDING) });
  if (action !== 'download' && !(action === 'pending' && applyNow)) return { action, latest: info.version };
  const bundle = await CapacitorUpdater.download({ url: info.url, version: info.version });
  if (applyNow) {
    localStorage.removeItem(KEY_PENDING);
    await CapacitorUpdater.set({ id: bundle.id }); // recarga la app con la versión nueva
  } else {
    await CapacitorUpdater.next({ id: bundle.id });
    localStorage.setItem(KEY_PENDING, info.version);
  }
  return { action: 'downloaded', latest: info.version };
}

window.KLinoNative = { createBle, startScan, keepAwake, shareCsv, checkApkUpdate, updateWeb };
