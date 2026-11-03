# K-Lino — Guía rápida

Escáner Bluetooth compatible con **Honda Civic 1996–2000** (conector de diagnóstico de **3 pines**).
Muestra RPM, velocidad, temperatura, sensores y VTEC en vivo; graba tus pasadas y lee/borra códigos de falla (Check Engine).

> **[POR LLENAR antes de imprimir]** WhatsApp de soporte: `__________` · Foto del conector: `docs/img/dlc_conector.jpg`

---

## 1. Conecta el K-Lino al auto

1. Ubícate en el asiento del **copiloto** y mira **debajo de la guantera**, hacia el panel lateral derecho.
2. Ahí hay una funda de goma con dos conectores sueltos:
   - uno de **2 pines** (no se usa),
   - uno de **3 pines** ← **aquí va el K-Lino**.
3. Conéctalo en la posición que indica la foto. **No lo fuerces.**
4. Acomoda el K-Lino para que no cuelgue ni lo pises con los pies.

Puedes dejarlo conectado: con la llave apagada se pone en reposo solo.
Si el auto va a estar parado **más de 2 semanas**, desconéctalo para cuidar la batería.

---

## 2. Instala la app

La app se abre desde el navegador y se instala como un ícono más. No ocupa tienda de apps.

**Dirección de la app:** `https://fedelt01hub.github.io/civic-obd/` (o escanea el QR de esta tarjeta)

### Android
1. Abre la dirección en **Google Chrome** (también sirve Brave).
2. Toca **Instalar** en el aviso naranja de arriba, o en el menú **⋮ → Agregar a pantalla principal / Instalar app**.
3. Aparece el ícono **K-Lino** en tu pantalla. Ábrela siempre desde ahí.

Requisitos: Android 7 o más nuevo, Bluetooth encendido. En Android 11 o anterior también enciende la **Ubicación** (Android la pide para buscar equipos Bluetooth).

### iPhone
Safari y Chrome de iPhone **no** permiten conectar equipos Bluetooth desde una página web. Usa el navegador gratuito **Bluefy**:
1. Instala **Bluefy – Web BLE Browser** desde la App Store.
2. Abre Bluefy y entra a la dirección de la app.
3. Guárdala en favoritos de Bluefy (⭐) para abrirla rápido la próxima vez.
4. Cuando Bluefy lo pida, permite el acceso a Bluetooth.

---

## 3. Conecta el celular al K-Lino

1. Gira la llave a **ON** (tablero encendido). Puedes arrancar el motor o no.
2. **Espera hasta 20 segundos**: el K-Lino despierta solo y busca la computadora del auto.
3. En la app toca **Conectar Bluetooth**.
4. Elige **`K-Lino-XXXX`** en la lista. Los 4 caracteres finales son los de **la etiqueta de tu equipo**.
5. Listo: el tablero empieza a moverse.

**No** lo vincules desde *Ajustes → Bluetooth* del celular. La conexión se hace solo desde la app.

---

## 4. Qué puedes hacer

| Pestaña | Para qué |
|---|---|
| **Dashboard** | Relojes en vivo: RPM, velocidad, temperatura, batería, sensores, VTEC, marcha. |
| **Datalogger** | Botón **Rec/Stop** para grabar una pasada, tiempos 0–60, 0–100 y 1/4 de milla, y descarga en **CSV**. |
| **Diagnóstico** | **Escanear Códigos de Falla** para ver qué prende el Check Engine; **Borrar** para apagarlo. |

Para borrar códigos: motor **apagado** y llave en **ON**.
**Importante:** borrar el código apaga la luz pero **no repara la falla**. Si la falla sigue, la luz se vuelve a prender.

En **Diagnóstico → Avanzado** se ve el nombre y la versión de tu equipo (`K-Lino-XXXX · FW …`). Si pides soporte, mándanos captura de esa línea.

---

## 5. Si algo no funciona

| Problema | Qué hacer |
|---|---|
| No aparece `K-Lino-XXXX` en la lista | Llave en **ON**, espera 20 s y vuelve a tocar Conectar. Revisa que el Bluetooth (y en Android viejo, la Ubicación) esté encendido. Si lo vinculaste en Ajustes del celular, entra ahí y toca **Olvidar**. |
| En iPhone no sale nada | Asegúrate de estar dentro de **Bluefy**, no en Safari ni en Chrome. |
| Conecta, pero los relojes no se mueven | Revisa que el conector de 3 pines esté bien metido. Apaga la llave, espera 10 s y vuelve a ponerla en ON. |
| Se desconecta al bloquear la pantalla | Es normal en algunos celulares. Deja la app abierta en primer plano mientras la usas. |
| La app se ve vieja o rara | Con internet, entra a **Diagnóstico → Avanzado → Buscar Actualización**. |

Si nada de esto lo resuelve, mándanos por WhatsApp una captura de **Diagnóstico → Avanzado**.

---

## Seguridad

- **No uses el celular mientras manejas.** Que lo opere el copiloto, o pon el teléfono en un soporte antes de arrancar.
- El K-Lino solo **lee** datos y borra códigos de falla. No modifica la computadora del auto.
- *Honda* y *Civic* son marcas de Honda Motor Co. K-Lino es un producto independiente y no está afiliado a Honda.
