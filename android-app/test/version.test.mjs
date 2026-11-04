import { test } from 'node:test';
import assert from 'node:assert/strict';
import { isNewer, versionFromTag } from '../src/version.mjs';

test('detecta versión más nueva', () => {
  assert.equal(isNewer('1.4.1', '1.4.0'), true);
  assert.equal(isNewer('1.10.0', '1.9.3'), true);   // numérico, no texto
  assert.equal(isNewer('2.0.0', '1.99.99'), true);
});

test('igual o más vieja no es nueva', () => {
  assert.equal(isNewer('1.4.0', '1.4.0'), false);
  assert.equal(isNewer('1.3.9', '1.4.0'), false);
  assert.equal(isNewer('1.4', '1.4.0'), false);     // faltante = 0
});

test('extrae la versión del tag', () => {
  assert.equal(versionFromTag('android-v1.4.0'), '1.4.0');
  assert.equal(versionFromTag('v1.4.2'), '1.4.2');
});
