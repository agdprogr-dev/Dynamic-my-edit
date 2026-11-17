<!-- coderabbit-ai-generated:v1 -->

# InputAlign: parche experimental de 80 ticks

Generado por CodeRabbit Coding Agent a partir del paquete incluido en el repositorio. Mantiene el desarrollador original lilWC y el identificador ai.predictivebuffer.

## Cambios

- Máximo 80 ticks en Normal, Wave e intervalo de spam.
- Normal y Wave predeterminados: 80. Spam Guard desactivado por defecto.
- Topes de recomendaciones y ventana de comparación de macros ampliados a 80.
- Conserva el parche anterior de opacidad del botón y su procedencia.

## Instalación

Sustituye la versión anterior del mismo mod en la carpeta de mods de Geode por este archivo y reinicia el juego. No mantengas ambos paquetes del mismo identificador instalados. Haz primero una copia del paquete anterior.

Los ajustes guardados de tu instalación pueden mantenerse: abre InputAlign y selecciona Normal = 80 y Wave = 80, y desactiva Spam Guard. El intervalo de spam solo influye si Spam Guard está activo; subirlo no aumenta la corrección.

80 ticks permiten hasta aproximadamente 333 ms de retraso, según la equivalencia declarada de 4 ticks por frame a 60 FPS. El presupuesto de simulación sigue limitado por Max correction time; no se garantiza que encuentre correcciones hasta ese horizonte en todos los niveles.

## Verificación y límites

Comprobados: integridad del ZIP, metadatos, recursos preservados y diferencias binarias limitadas a las instrucciones identificadas y dos textos. Pasaron 108 casos de emulación x86-64 de controles y recomendaciones.

**No probado dentro de Geometry Dash.** El entorno de trabajo es Linux y no dispone del juego Windows ni del runtime Geode. La emulación cubre fragmentos aislados; no demuestra estabilidad de la simulación completa, compatibilidad con CBF ni mejoras en partidas reales. Esta entrega es experimental.

## Prueba dentro del juego

1. Comprueba que los controles permiten llegar a 80 y conservan el valor tras reiniciar.
2. Prueba primero en práctica, tanto Normal como Wave, con Spam Guard apagado.
3. Compara el mismo tramo a 40 y 80; revisa retrasos, FPS, pulsaciones y liberaciones, también con CBF si lo usas.
4. Si hay cierres o entradas incorrectas, vuelve al paquete anterior.
