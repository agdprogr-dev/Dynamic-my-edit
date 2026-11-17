<!-- coderabbit-ai-generated:v1 -->

# InputAlign Diagnostics — experimental

Complemento nuevo, escrito para este repositorio. No sustituye ni modifica la DLL de InputAlign, no añade correcciones y no reconstruye el motor de simulación original. Su identificador es `agdprogr.inputalign_diagnostics`; el mod original conserva `ai.predictivebuffer`.

## Qué muestra

El botón **Diag InputAlign** del menú de pausa muestra los ajustes actuales y las últimas ocho llamadas observadas a `handleButton` (guarda hasta 32 por intento). Distingue buffer apagado, trayectoria desactivada, restricción a práctica, modo desactivado, ventana de cero ticks y CBF cargado sin sincronización. Comprueba la presencia y el tipo de los ajustes antes de leerlos. Los ajustes de cada entrada se capturan al observarla, por lo que cambiar opciones después no reescribe su historial.

**Esto no es un registro de decisiones internas de InputAlign.** Las llamadas observadas pueden incluir entradas generadas por otros mods, dependen del orden de hooks y no equivalen a clics físicos. Sin un jugador identificado no se registra la llamada. Los bloqueos describen la configuración en ese momento; no demuestran qué rama tomó la DLL. Si no hay bloqueos conocidos aparece **motivo interno no disponible**. No atribuye fallos a tiempo agotado, spam o falta de trayectoria segura sin datos del motor. El presupuesto, Spam Guard y rendimiento se muestran como contexto.

## Estabilidad del complemento

No retrasa, reproduce, consume ni cambia argumentos de entrada; reenvía cada llamada una vez. El historial usa almacenamiento fijo, no conserva punteros del juego y se limpia al reiniciar o salir. Ante un error de captura se suspende la captura hasta reiniciar y continúa el envío de entradas. No escribe archivos por clic. Se puede desactivar la captura en Geode.

Estas medidas no prueban estabilidad dentro del juego. Es una integración experimental para **Windows, Geometry Dash 2.2081 y Geode 5.11.2**, pendiente de probar con InputAlign y CBF reales.

## Compilación y pruebas

Pruebas de la lógica sin juego ni SDK:

```sh
cmake -S diagnostics -B /tmp/inputalign-diagnostics-build -DINPUTALIGN_BUILD_MOD=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/inputalign-diagnostics-build
ctest --test-dir /tmp/inputalign-diagnostics-build --output-on-failure
```

Para el mod, en Windows con herramientas C++ y Geode CLI, instala el SDK 5.11.2, define `GEODE_SDK` y compila este directorio mediante CMake. El workflow `InputAlign diagnostics` compila y publica un artefacto experimental cuando cambian estos archivos; un workflow definido no implica que haya pasado.

Instala el `.geode` resultante junto a InputAlign, abre práctica, pulsa y suelta, pausa y abre el botón de diagnóstico. Comprueba que las entradas siguen comportándose igual, que el historial cambia y se borra al reiniciar. Cambia cada ajuste y comprueba el mensaje correspondiente. Repite con CBF activado, desactivado y sin sincronización, y con dos jugadores. Desactiva el complemento si interfiere. No confundir ausencia de observaciones con ausencia de clics o de correcciones.

## Trabajo pendiente del motor

Para conocer el motivo definitivo por clic, el motor debe emitir su decisión en cada salida: corrección aplicada, entrada ya segura, sin solución dentro de la ventana, presupuesto agotado o entrada omitida por un filtro. Ese dato no existe en la interfaz pública que hemos podido comprobar de la DLL actual. Este complemento no afirma haber añadido esa instrumentación interna ni haber optimizado la simulación.
