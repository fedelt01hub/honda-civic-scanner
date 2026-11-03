# APK Android (TWA) — Civic OBD

El APK es un contenedor **Trusted Web Activity**: abre `https://fedelt01hub.github.io/civic-obd/`
con el motor de Chrome del teléfono, así que Web Bluetooth y Wake Lock funcionan sin cambiar el JS.
Se instala una sola vez; el contenido se actualiza solo vía `sw.js` cada vez que se publica `civic-obd`.

| Dato | Valor |
| --- | --- |
| Package ID | `io.github.fedelt01hub.civicobd` |
| URL de inicio | `https://fedelt01hub.github.io/civic-obd/index.html` |
| Keystore | `android-signing/civic-obd.keystore` (alias `civic-key`, **no está en git**) |
| SHA-256 | `F9:40:8D:A3:0E:66:38:95:19:DC:0F:14:09:89:23:10:49:3E:7B:8E:E3:E3:40:E6:88:D6:52:E4:BA:79:A4:60` |

> ⚠️ Respalda `android-signing/` (keystore + `credenciales.txt`) fuera de la PC. Si se pierde,
> hay que generar otro keystore, agregar su huella en `assetlinks.json` y reinstalar el APK.

## Archivos

- `twa-manifest.json` — configuración de Bubblewrap (colores, URL, versión mínima Android 7.0 / SDK 24).
- `generate.mjs` — genera el proyecto Gradle en `twa/android/` (ignorado por git) sin preguntas interactivas.
- `build-local.sh` — genera, compila y firma en la PC → `android-signing/out/Civic-OBD-Scanner.apk`.
- `user-site/` — contenido para el repo `fedelt01hub.github.io` (ver paso 1).
- `../.github/workflows/build-apk.yml` — compila y publica el APK en GitHub Releases.

## Paso 1 — Digital Asset Links (quita la barra de URL)

Android solo busca el archivo en la **raíz del dominio**:
`https://fedelt01hub.github.io/.well-known/assetlinks.json`. Un repo de proyecto (`civic-obd`)
no puede servir esa ruta, así que hace falta el repo de usuario:

1. En GitHub crea un repo **público** llamado exactamente `fedelt01hub.github.io`.
2. Sube el contenido de `twa/user-site/` (incluye `.nojekyll`, necesario para que Pages publique
   la carpeta `.well-known`; y un `index.html` que redirige a `/civic-obd/`).
3. Verifica: `curl https://fedelt01hub.github.io/.well-known/assetlinks.json`.

Sin este paso el APK funciona igual, pero muestra una barra de URL arriba (modo Custom Tab).

## Paso 2 — Compilar en GitHub Actions (opcional)

En *Settings → Secrets and variables → Actions* del repo `honda-civic-scanner` crea:

- `KEYSTORE_BASE64` = contenido de `android-signing/keystore_base64.txt`
- `KEYSTORE_PASSWORD` = `STORE_PASSWORD` de `android-signing/credenciales.txt`

Luego *Actions → Build APK (TWA) → Run workflow*, o empuja un tag `apk-v1.0.1`.
El APK queda en *Releases*.

## Compilar en la PC

```bash
export JAVA_HOME="/ruta/a/jdk-17"   # el JBR de Android Studio es JDK 25 y Gradle 8.11 no lo soporta
bash twa/build-local.sh 2 1.0.1      # versionCode, versionName
```

## ¿Cuándo hace falta un APK nuevo?

Casi nunca: solo si cambian el nombre, ícono del lanzador, colores de splash o la URL.
Para cambios de la app basta con actualizar `civic-obd` (subir versión en `sw.js` y `index.html`).
Si se reinstala un APK nuevo, su `versionCode` debe ser mayor que el instalado.
