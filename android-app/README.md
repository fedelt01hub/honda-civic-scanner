# APK Android de K-Lino (Capacitor)

El APK usa **la misma app** que la web (`../app/index.html`). `npm run build:web` la copia a `www/`
(sin `sw.js`) y empaqueta `src/native-entry.mjs` → `www/native-bridge.js`, que expone
`window.KLinoNative` (Bluetooth nativo, pantalla encendida, compartir CSV y aviso de actualizaciones).
`index.html` detecta el APK con `IS_NATIVE` y usa ese puente; en el navegador todo sigue igual.

| Dato | Valor |
| --- | --- |
| ID del paquete | `com.klino.escaner` (permanente) |
| Versión | `android/app/build.gradle` → `versionCode` / `versionName` |
| Llave de firma | `~/klino-signing/klino.jks`, alias `klino` (**no está en git**) |

## Comandos

```bash
npm ci
npm test              # pruebas de versiones (node --test)
npm run build:web     # arma www/
npm run apk:debug     # APK de prueba: android/app/build/outputs/apk/debug/app-debug.apk
```

Requiere JDK 21 y Android SDK 36 (`JAVA_HOME`, `ANDROID_HOME`; `android/local.properties` con `sdk.dir=...`).

## Llave de firma (una sola vez)

```bash
mkdir -p ~/klino-signing && cd ~/klino-signing
keytool -genkeypair -v -keystore klino.jks -alias klino -keyalg RSA -keysize 2048 -validity 10000 \
  -dname "CN=K-Lino, O=K-Lino, C=MX"          # misma contraseña para store y key
base64 -w0 klino.jks > klino.jks.b64
gh secret set KLINO_KEYSTORE_BASE64 -R fedelt01hub/honda-civic-scanner < klino.jks.b64
gh secret set KLINO_KEYSTORE_PASSWORD -R fedelt01hub/honda-civic-scanner
rm klino.jks.b64
```

> ⚠️ Respalda `klino.jks` y su contraseña fuera de la PC. Sin ellos no se puede publicar una
> actualización que se instale encima: los clientes tendrían que desinstalar.

## Actualizar la app sin reinstalar (lo normal)

Pantallas, códigos de falla, animaciones, etc. viven en `../app/` y llegan al APK solos:

1. Subir la versión web como siempre (`v1.X` en `app/index.html` y `app/sw.js`).
2. `npm run bundle` → `dist/apk-bundle-<versión>.zip` y `dist/apk-bundle.json`.
3. Copiar **ambos** a la raíz del repo `civic-obd` (junto con `index.html` y `sw.js`), borrar los
   `apk-bundle-*.zip` viejos y hacer push.
4. Cada APK revisa `apk-bundle.json` al abrir (con internet), descarga el zip en segundo plano y lo
   aplica la próxima vez que se abre. En Avanzado → Buscar actualización se aplica al momento.
   Si la versión nueva no arranca bien, el plugin regresa sola a la anterior a los 10 s.

`klino.minNative` en `package.json` = versión mínima de APK que necesita la app web. Súbelo solo
si el cambio usa algo nativo nuevo (otro plugin, permisos); esos APK viejos verán el aviso de APK nuevo.

## Publicar un APK nuevo (solo cambios nativos)

1. Subir `versionCode` (+1) y `versionName` en `android/app/build.gradle`.
2. Commit, `git tag android-v<versionName>` y `git push origin android-v<versionName>`.
3. GitHub Actions (`.github/workflows/build-android.yml`) compila, firma y sube `K-Lino-<versión>.apk` a Releases.
4. En la app: Avanzado → Buscar actualización detecta el Release y abre la descarga.
