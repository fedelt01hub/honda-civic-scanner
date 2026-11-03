#!/usr/bin/env bash
# Compila y firma el APK en la PC (Git Bash). Requiere:
#   JAVA_HOME -> JDK 17 (Gradle 8.11 no corre con JDK 25)
#   Android SDK en %LOCALAPPDATA%/Android/Sdk (o ANDROID_HOME)
#   android-signing/civic-obd.keystore + android-signing/credenciales.txt
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${ANDROID_HOME:-$LOCALAPPDATA/Android/Sdk}"
export ANDROID_HOME="$SDK"
BT="$SDK/build-tools/$(ls "$SDK/build-tools" | sort -V | tail -1)"
APKSIGNER="$BT/apksigner"; [ -f "$APKSIGNER.bat" ] && APKSIGNER="cmd //c $(cygpath -w "$BT/apksigner.bat")"
PW=$(grep '^STORE_PASSWORD=' "$ROOT/android-signing/credenciales.txt" | cut -d= -f2)

cd "$ROOT/twa"
[ -d node_modules ] || npm ci
node generate.mjs "${1:-1}" "${2:-1.0.0}"
(cd android && ./gradlew assembleRelease --no-daemon -q)

OUT="$ROOT/android-signing/out"; mkdir -p "$OUT"
"$BT/zipalign" -f -p 4 android/app/build/outputs/apk/release/app-release-unsigned.apk "$OUT/aligned.apk"
$APKSIGNER sign --ks "$ROOT/android-signing/civic-obd.keystore" --ks-key-alias civic-key \
  --ks-pass "pass:$PW" --key-pass "pass:$PW" --out "$OUT/Civic-OBD-Scanner.apk" "$OUT/aligned.apk"
echo "APK listo: $OUT/Civic-OBD-Scanner.apk"
