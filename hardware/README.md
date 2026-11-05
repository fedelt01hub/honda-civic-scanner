# PCB del escáner K-Line (Honda Civic, DLC de 3 pines)

Placa de 66 × 42 mm y 2 capas que sustituye el montaje en perfboard (ESP32-C3 + L9637D + LM2596).
Todo el diseño se genera con scripts, así que es reproducible.

| Bloque | Pieza | Notas |
|---|---|---|
| Entrada DLC | J1: 3 pads para soldar el cable adaptador Honda→OBD2 cortado | K = OBD2 p7, 12V = p16, GND = p4/5 |
| Protección | F1 PTC 500 mA 60 V, D2 SS34 (inversión), D1 SMBJ18A (TVS) | |
| 12 V → 5 V | U1 AP63205WU + L1 4,7 µH | buck síncrono de 2 A |
| USB-C | J2 HRO TYPE-C-31-M-12, U4 USBLC6-2SC6, CC 5,1 k | programación por el USB nativo (IO18/IO19) |
| OR 5 V | D3/D4 B5819W | se puede alimentar por el coche y por USB a la vez |
| 3,3 V | U2 AP2112K-3.3 (600 mA) | |
| MCU | U3 ESP32-C3-WROOM-02-N4 | la antena sobresale del borde superior, con keep-out |
| K-Line | U5 L9637D, R6 1 k 1 W pull-up | RX = IO4, TX = IO5 (igual que el firmware) |
| Interfaz | SW1 RESET, SW2 BOOT (IO9), 3 LEDs junto al USB-C: PWR (blanco, +5V), STATUS (amarillo, IO8), K-LINE (rojo, actividad del bus); TPs | |

## Regenerar

```bash
KP="/c/Program Files/KiCad/10.0/bin/python.exe"
cd hardware/scripts
"$KP" gen_sch.py        # esquemático
"$KP" gen_3d.py         # modelos 3D simplificados (USB-C y JST)
"$KP" gen_pcb.py        # colocación + rutas a mano (routes.py) + autorouter A* (autoroute.py) + zonas GND
"$KP" export_fab.py     # ERC/DRC (se detiene si hay errores) + Gerbers/taladros/BOM/CPL/PDF/renders
```

## Fabricación (JLCPCB)

En `fab/` están estos archivos:
- `civic_scanner-gerbers.zip`: va en el pedido de PCB (2 capas, 1,6 mm, HASL sin plomo o ENIG).
- `civic_scanner-bom-jlcpcb.csv` y `civic_scanner-cpl-jlcpcb.csv`: se usan para el ensamble PCBA, solo por la cara superior.
- `civic_scanner-erc.rpt` y `civic_scanner-drc.rpt`: 0 errores, 0 violaciones y 0 pads sin conectar.

Antes de pedir conviene revisar dos cosas:
- **Rotaciones del CPL:** compruébalas en la vista previa de JLCPCB, sobre todo U1/U2/U4 (SOT), U5 (SOIC-8), J2 y los LEDs. KiCad y JLC no siempre comparten el cero de rotación.
- **J1:** no va en el BOM. Son pads donde se suelda a mano el cable.
- **Disponibilidad:** confirma el stock de cada LCSC el día del pedido.

## Carcasa

La carcasa imprimible 3D (base + tapa en STL) está en `enclosure/`; ver `enclosure/README.md`.
