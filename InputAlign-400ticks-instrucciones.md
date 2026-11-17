<!-- coderabbit-ai-generated:v1 -->

# InputAlign: variante experimental de 400 ticks

Generado por CodeRabbit Coding Agent a partir del paquete original de este repositorio. Desarrollador original: lilWC; identificador conservado: ai.predictivebuffer.

## Alcance

- Controles y máximos de ajustes Normal, Wave e intervalo de spam: 400 ticks.
- Normal y Wave predeterminados: 400. Los ajustes ya guardados pueden prevalecer; selecciona el valor manualmente.
- Spam Guard permanece apagado por defecto. Su intervalo no aumenta las correcciones mientras está apagado.
- **Las recomendaciones y la comparación de macros conservan sus límites de 80 ticks.** Aplicar una recomendación puede bajar los valores seleccionados. No se han reescrito esas instrucciones ni el motor de simulación.
- El presupuesto Max correction time no cambia. Poder seleccionar 400 no demuestra que el simulador alcance esa ventana en todos los niveles.

400 ticks / 240 ticks por segundo = **1.666,7 ms = 1,67 segundos** de retraso máximo permitido. No es el tiempo de cálculo. Una ventana mayor puede generar entradas muy tardías y no garantiza más saltos salvados.

## Instalación y relación con el complemento

Haz copia del paquete anterior. Sustituye el InputAlign original por este archivo del mismo identificador; no instales varias versiones del original simultáneamente. Abre los ajustes y selecciona Normal/Wave manualmente si se conservaron valores antiguos.

El complemento InputAlign Diagnostics v0.3.0 tiene otro identificador y una ayuda de aterrizaje distinta, en milisegundos. Para usarla en práctica, desactiva Buffer in cube and others del original; puedes dejar Wave del original activo. Tener 400 ticks no amplía el límite de 150 ms de esa ayuda. No se ha probado la convivencia dentro del juego.

## Verificación y limitaciones

Las pruebas comprueban integridad del ZIP, metadatos, procedencia, recursos preservados, salida reproducible y diferencias binarias limitadas a tres instrucciones respecto al paquete de 80 ticks. Emulan los controles y límites de ajuste x64, incluidos valores próximos a 400. Esas comprobaciones no ejecutan la física completa, la carga de Geode ni CBF.

**No probado en Geometry Dash.** No se puede prometer más correcciones útiles sin repetir las mismas entradas en los mismos niveles y comparar muertes, retraso y FPS.

## Reproducción

```sh
python tools/patch_inputalign_400.py InputAlign-v2.7.1-alpha.1-windows.geode InputAlign-v2.7.1-alpha.1-400ticks-windows.geode
PYTHONPATH=/tmp/inputalign-testdeps python -m unittest discover -s tools -p test_inputalign_400.py
```

Las pruebas necesitan Unicorn 2.1.4. La herramienta requiere el paquete original exacto y comprueba su hash mediante el parche base de 80 ticks. La salida tiene SHA-256 `26c731cd2e243c80ae85298fdfe73b9dd852a191ac32cb98764b64c8c79971b7`.
