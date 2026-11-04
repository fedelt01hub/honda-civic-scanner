import { test } from 'node:test';
import assert from 'node:assert/strict';
import { decideWebUpdate } from '../src/version.mjs';

const info = { version: '1.5', url: 'https://x/apk-bundle-1.5.zip', minNative: '1.4.0' };

test('descarga cuando hay versión web más nueva y el APK es compatible', () => {
  assert.equal(decideWebUpdate({ currentWeb: 'v1.4', native: '1.4.0', info }), 'download');
});

test('nada si ya está al día o la publicada es más vieja', () => {
  assert.equal(decideWebUpdate({ currentWeb: 'v1.5', native: '1.4.0', info }), 'none');
  assert.equal(decideWebUpdate({ currentWeb: 'v1.6', native: '1.4.0', info }), 'none');
});

test('pide APK nuevo si la versión web necesita un APK más reciente', () => {
  assert.equal(decideWebUpdate({ currentWeb: 'v1.4', native: '1.4.0', info: { ...info, minNative: '1.5.0' } }), 'needsApk');
});

test('no vuelve a descargar lo que ya está esperando para aplicarse', () => {
  assert.equal(decideWebUpdate({ currentWeb: 'v1.4', native: '1.4.0', info, pending: '1.5' }), 'pending');
});

test('ignora un aviso de versión incompleto', () => {
  assert.equal(decideWebUpdate({ currentWeb: 'v1.4', native: '1.4.0', info: { version: '1.5' } }), 'none');
  assert.equal(decideWebUpdate({ currentWeb: 'v1.4', native: '1.4.0', info: null }), 'none');
});
