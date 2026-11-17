<!-- coderabbit-ai-generated:v1 -->

# InputAlign Diagnostics — experimental

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:7abd1fc74b45dfa4cf1ccc4c378ffcbf569bef892fb57ceac11c5e50ac8b7fb8 -->

Complemento escrito para este repositorio. Desde v0.2.0 añade una ayuda opcional de tap al aterrizar, además del diagnóstico. No sustituye ni modifica la DLL original ni reconstruye su simulador de trayectorias. Su identificador sigue siendo `agdprogr.inputalign_diagnostics`.

<!-- /coderabbit-ai-generated:v1 -->

## Qué muestra

El botón **Diag InputAlign** del menú de pausa muestra los ajustes actuales y las últimas ocho llamadas observadas a `handleButton` (guarda hasta 32 por intento). Distingue buffer apagado, trayectoria desactivada, restricción a práctica, modo desactivado, ventana de cero ticks y CBF cargado sin sincronización. Comprueba la presencia y el tipo de los ajustes antes de leerlos. Los ajustes de cada entrada se capturan al observarla, por lo que cambiar opciones después no reescribe su historial.

**Esto no es un registro de decisiones internas de InputAlign.** Las llamadas observadas pueden incluir entradas generadas por otros mods, dependen del orden de hooks y no equivalen a clics físicos. Sin un jugador identificado no se registra la llamada. Los bloqueos describen la configuración en ese momento; no demuestran qué rama tomó la DLL. Si no hay bloqueos conocidos aparece **motivo interno no disponible**. No atribuye fallos a tiempo agotado, spam o falta de trayectoria segura sin datos del motor. El presupuesto, Spam Guard y rendimiento se muestran como contexto.

## Estabilidad del complemento

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:5d911f7e5c2759c91ad0cc34dfc4863a2002848f5dfb3aeaed910c832c634210 -->

El diagnóstico reenvía cada llamada original una vez con sus argumentos. La ayuda opcional de v0.2.0 puede añadir una pulsación y una liberación al aterrizar, como se explica abajo.

<!-- /coderabbit-ai-generated:v1 --> El historial usa almacenamiento fijo, no conserva punteros del juego y se limpia al reiniciar o salir. Ante un error de captura se suspende la captura hasta reiniciar y continúa el envío de entradas. No escribe archivos por clic. Se puede desactivar la captura en Geode.

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

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:3fda931bdc25d940eb87fa605109fc790724a02d903a4d89d7665cfed0f288ac -->

## Ayuda de tap al aterrizar (v0.2.0)

**Cambio de comportamiento autorizado:** el complemento antes solo observaba. Ahora permite activar una asistencia experimental, apagada por defecto. En Geode activa **Ayuda de tap al aterrizar** y ajusta su ventana (10–150 milisegundos reales, 80 por defecto; no son ticks). <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:f06d5040417484ccb6d4627de482f9bd975d05d3f5cb68a75bdc068139d4abb1 -->

Desde v0.3.0 basta con apagar **Buffer in cube and others** del original; puedes conservar **Buffer in wave** activo. También funciona con el original completamente apagado o ausente. CBF debe estar ausente.

<!-- /coderabbit-ai-generated:v1 -->

Solo se aplica en práctica, al jugador 1 y al cubo normal de un jugador, con gravedad normal, sin pendientes, dash, orbes, pads ni portales detectados. No actúa en plataforma, dual, Wave ni otros vehículos. Deja pasar todas las entradas originales. Cuando detecta una pulsación mientras caes, recuerda ese tap brevemente. Si ya soltaste antes de tocar el suelo y sigues dentro de la ventana, añade una pulsación y la libera tras la siguiente actualización de física. Una pulsación que sigues manteniendo queda a cargo del juego. Nunca libera una pulsación nueva del usuario.

El panel muestra el bloqueo actual de esta ayuda y su último resultado real: pendiente, aplicado, expirado, cancelado o pulsación mantenida. No convierte esos resultados en decisiones del motor original. Pausar cancela taps pendientes; reiniciar, morir y salir limpian el estado. La ventana usa reloj monotónico real, por lo que a cámara lenta representa menos tiempo de juego. Llamadas generadas por otros mods pueden confundirse con taps; no usar con macros/reproductores u otras ayudas de entrada.

No predice obstáculos ni garantiza que el salto sea seguro. Puede producir saltos no deseados, incluso en interacciones que sus filtros no detecten. **No se ha probado en Geometry Dash.** Empieza con un suelo sencillo en práctica: pulsa y suelta justo antes de caer y comprueba un salto; repite fuera de la ventana, manteniendo el botón, pausando, muriendo y reiniciando. Verifica que no se queda pulsado. Prueba también cada filtro: <!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:11619eeb5cc90e992457261ccd2ff5feaf0bbf27474982dd1f5fa0a70f5ee264 -->

con el buffer normal del original activo o CBF presente debe quedar suspendida.

<!-- /coderabbit-ai-generated:v1 --> Mantén esta función apagada fuera de esas pruebas.

<!-- coderabbit-ai-generated:v1 -->

<!-- coderabbit-ai-generated-section:v1:bcbf9c8990f2cca117e7b21d928a9688fa91fda64d6ca95fdbe74936ad81e517 -->

## Novedades v0.3.0

La compatibilidad ahora se decide mediante una política comprobada fuera del juego. El original puede conservar Wave activado si **Buffer in cube and others** está apagado. Si no se pueden leer sus ajustes con el tipo esperado, la ayuda se suspende. Se conservan las restricciones de práctica, jugador 1, cubo normal y ausencia de CBF. Esta convivencia se apoya en la separación Normal/Wave documentada por el mod original; todavía no se ha probado dentro del juego.

El panel muestra contadores por nivel: **solicitudes de pulsación**, taps expirados, taps cancelados y pulsaciones mantenidas a cargo del juego, junto al retraso de la última solicitud. No cuentan vidas salvadas ni demuestran que una pulsación solicitada haya superado todos los hooks. No cuentan cada observación de colisión como otra corrección. Los contadores usan espacio fijo y saturan al llegar al máximo. Sobreviven a muertes y reintentos; salir del nivel los borra. Las entradas pendientes sí se descartan inmediatamente al morir o reiniciar.

La cobertura añade pruebas de los filtros y sus combinaciones, convivencia con el original, conservación de estadísticas, reinicios y ausencia de doble conteo. El verificador de cobertura genérico no reconoce estos archivos C++; se inspeccionaron manualmente. Compilar y pasar estas pruebas no sustituye la comprobación de física y orden de hooks dentro de Geometry Dash.

## Trabajo pendiente del motor

<!-- /coderabbit-ai-generated:v1 -->

<!-- /coderabbit-ai-generated:v1 -->

Para conocer el motivo definitivo por clic, el motor debe emitir su decisión en cada salida: corrección aplicada, entrada ya segura, sin solución dentro de la ventana, presupuesto agotado o entrada omitida por un filtro. Ese dato no existe en la interfaz pública que hemos podido comprobar de la DLL actual. Este complemento no afirma haber añadido esa instrumentación interna ni haber optimizado la simulación.
