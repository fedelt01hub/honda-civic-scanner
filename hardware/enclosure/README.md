# Carcasa imprimible 3D

Archivos listos para imprimir:

| Archivo | Qué es | Orientación |
|---|---|---|
| `case_base.stl` | base con soportes de PCB, entrada de cable y abertura USB-C | tal cual (fondo sobre la cama) |
| `case_lid.stl` | tapa con botones flexibles RESET/BOOT, ventanas de LEDs y textos grabados | tal cual (cara superior sobre la cama) |
| `case_assembly.stl`, `case_preview_exploded.stl` | solo para ver el ensamble | no imprimir |

Medidas exteriores: **82,3 × 47,6 × 15 mm**.

## Impresión

- **Material:** PETG o ASA, porque dentro del coche hace calor. El PLA se deforma al sol.
- **Parámetros:** capas de 0,2 mm, 3 perímetros, 20 % de relleno. **Sin soportes.**
- **Tornillería:** 4 tornillos autorroscantes **M3 × 12** para plástico y 1 cincho (brida) de 2,5 mm.

## Montaje

1. **Cable:** corta el adaptador Honda 3p → OBD2 dejando unos 10 cm después del conector Honda. Antes de cortar, identifica los hilos con el multímetro desde el lado OBD2:
   - pin 7 = K-Line
   - pin 16 = +12 V
   - pin 4/5 = GND
2. Pasa el extremo cortado por el agujero de la pared izquierda (lado "OBD").
3. Pela unos 3 cm de la funda y suelda los hilos en J1 (pads marcados K / 12V / GND en la serigrafía).
4. Fija la funda con el cincho: pásalo por el túnel del puente que está en el piso y ciérralo sobre el cable. Al cerrar, la tapa además prensa el cable con una costilla.
5. Coloca la PCB sobre los soportes (el USB-C queda en la abertura derecha) y cierra la tapa con los 4 tornillos. Dos de ellos atraviesan los agujeros H1/H2 de la PCB y la sujetan.

## Ajustes

Todo es paramétrico en `gen_case.py`. Lo más probable que tengas que ajustar:
- **`CABLE_D` (5,0 mm):** mide el diámetro de tu cable con un calibrador. Si no coincide, el agujero de entrada y la sujeción quedarán holgados o apretados.
- **`FIT` (0,25 mm):** holgura del faldón de la tapa. Súbela si la tapa entra muy justa en tu impresora.

Para regenerar los STL:

```bash
pip install manifold3d trimesh numpy matplotlib
"$KP" ../scripts/export_case_data.py     # solo si cambia la PCB (escribe board.json)
python gen_case.py                       # STL + chequeo de interferencias
python sections.py                       # cortes 2D en ../build/sec_*.png para revisar
```
