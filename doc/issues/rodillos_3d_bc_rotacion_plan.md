# GUI: BC de velocidad angular para herramientas MINI3D

## Estado y dependencia

Solo plan, basado en lectura de `src`; no hay implementación en esta revisión.
Contrato coordinado con
`../weldform-engine/doc/issues/rodillos_3d_eje_movil_plan.md`.
No agregar `RigidBodies[].motion`. Extender BC y amplitudes existentes.
La GUI se implementa separadamente de D1/D2/D3 del motor; no habilitar exportación
angular para ejecución hasta disponer de D2 del motor cerrada.

Tres entregas pequeñas secuenciales, cada una en sesión individual con rol
`luna`, modelo solicitado `gpt-6-luna`, esfuerzo `low`, sin subagentes. Informar
limitaciones del runtime y no afirmar modelo no confirmado. Preservar cambios
ajenos. Al cerrar cada entrega: requisitos explícitos, pruebas mínimas,
comandos/resultados, revisión separada del diff y pendientes.

## Hallazgos del directorio src

- `src/bc_dialog.cpp`: carga valores existentes cerca de línea 96, inicializa
  nuevos cerca de 122, dibuja selector Constant/Function (0/1), vector lineal,
  factor y editor de tabla desde 289; Create/Update desde 400. No hay controles
  angulares ni centro de giro. El diálogo también edita IC y desplazamientos:
  no mostrar ni aplicar giro en esos casos.
- `src/bc_dialog.h`: solo estado lineal `m_valueType`, `m_amplitudeFactor`,
  vectores de tabla y velocidad; necesita estado independiente angular y reset.
- `src/model/Condition.h`: `ConstantValue = 0`, `AmplitudeValue = 1`; almacena
  vector lineal, tabla, factor y máscara. `setAmplitudeTable` ordena y trunca
  longitudes desiguales: no usarlo para ocultar errores de tabla angular.
- `src/model/BoundaryCondition.h`: deriva de Condition y actualmente no tiene
  datos adicionales. Guardar aquí el bloque angular opcional, evitando añadir
  semántica de giro a InitialCondition. Identificar ausencia explícitamente,
  incluso si el usuario prescribe giro cero.
- `src/model/Part.h`: existe `Part_Type {Elastic=0, Rigid}`. Resolver la parte
  por identidad para habilitar controles, sin inferir rigidez por su nombre.
- `src/io/ModelWriter.cpp` y `ModelReader.cpp`: `.wfmodel` persiste tablas de
  amplitud dentro de cada BC, con `amplitudeTime`/`amplitudeValue`. Falta todo
  el bloque angular; actualizar ambas direcciones para no perderlo al reabrir.
- `src/io/InputWriter.cpp`: `appendAmplitudeForCondition` y exportador de BC
  generan tabla global e ID correlativo; solo usan amplitud lineal. Ante tabla
  inválida hay degradación a constante: no copiar ese comportamiento al giro.
- `src/io/InputReader.cpp`: lee BC lineales y resuelve tabla desde `Amplitudes`.
  Hoy un `amplitudeId` encontrado fuerza modo amplitud; el bloque angular debe
  respetar `angularValueType` y validar contradicciones según contrato del motor.

Los números de línea son orientativos. Releer los bloques al implementar.

## Contrato visible y JSON de solver

Mantener los campos lineales actuales intactos y agregar, solo cuando el bloque
angular esté habilitado:

```json
{
  "zoneId": 2,
  "valueType": 1,
  "value": [0.0, -1.2, 0.0],
  "amplitudeId": 1,
  "amplitudeFactor": 1.0,
  "valueAng": [0.0, 0.0, 3.0],
  "rotationCenter": [0.0, 0.0, 0.0],
  "angularValueType": 1,
  "angularAmplitudeId": 2,
  "angularAmplitudeFactor": 1.0
}
```

- BC Velocity aplicada a Part rígida, modelo 3D, solver MINI3D. No IC, NodeSet,
  Symmetry ni Displacement. Resolver las opciones reales de solver/dimensión
  del modelo antes de cablear la condición de visibilidad.
- `valueAng`: vector angular rad/s, X/Y/Z globales, mano derecha.
- `rotationCenter`: centro inicial global de la malla ya posicionada; acompaña
  la traslación. Entrada explícita, sin inferencia por origen/centroide.
- Modo angular Constant = 0; Function/table = 1. Independiente del lineal.
  Modo omitido en importación = 0, factor omitido = 1; no heredar tabla lineal.
  En modo 0 exportar vector, centro y modo; omitir referencia/factor de amplitud.
- Tabla angular en la colección existente `Amplitudes`; IDs sin colisión con
  tablas lineales. Compartir ID es admisible si los datos son iguales, pero no
  es necesario deduplicar para esta entrega.
- Omitir todos los nuevos campos cuando el bloque angular esté desactivado.
  Mantener JSON antiguo y defaults al importar archivos sin campos nuevos.
- Una tabla escala las tres componentes del vector; no prometer tablas
  independientes por eje. Traslación y giro sí tienen tablas independientes.

## G1 — Datos de BC y persistencia del proyecto

Propiedad: `src/model/BoundaryCondition.h`,
`src/io/ModelReader.cpp`, `src/io/ModelWriter.cpp`.

Agregar datos opcionales: vector angular, centro inicial, modo angular,
factor y tabla angular. Inicializar de manera determinista. No modificar el
significado de los datos lineales ni de IC. Mantener las tablas angulares sin
normalizaciones silenciosas; validación explícita al editar/importar/exportar.

En `.wfmodel` guardar `valueAng`, `rotationCenter`, `angularValueType`,
`angularAmplitudeFactor`, `angularAmplitudeTime`, `angularAmplitudeValue` dentro
de la BC cuando corresponda. No guardar IDs globales de solver como identidad
permanente de una tabla del proyecto. Leer proyectos antiguos como bloque ausente.
Rechazar bloque parcial/incorrecto con diagnóstico; no perder giro silenciosamente.

Prueba mínima: un roundtrip de proyecto con dos BC rígidas, tablas lineal/angular
diferentes y centro no nulo; un proyecto antiguo sin giro carga sin cambios.
Aceptación: compila, valores y presencia se conservan, diff revisado. No UI aún.

## G2 — Editor en bc_dialog.cpp

Propiedad: `src/bc_dialog.cpp` y `src/bc_dialog.h`.

1. Mostrar opción de velocidad angular solo para destino compatible. Presentar
   controles X/Y/Z en rad/s, centro global X/Y/Z y nota de centro trasladado.
2. Editor Constant/Function angular con factor/tabla propios, reutilizando
   presentación del editor lineal sin compartir sus variables ni IDs ImGui.
   No duplicar un sistema de amplitudes global nuevo.
3. Cablear creación, edición, cancelación y reinicio de diálogo. Cargar datos
   de G1 y guardar mediante BoundaryCondition, sin convertir IC a BC.
4. Validar números finitos, tabla con al menos dos puntos, tiempos no negativos
   estrictamente crecientes y longitudes iguales. No exigir primer tiempo cero;
   indicar interpolación lineal y valor extremo mantenido fuera del intervalo.
5. Si una BC angular existente cambia a un destino/tipo incompatible, bloquear
   guardado con explicación hasta desactivar explícitamente el giro o corregir
   destino. No descartar datos por un simple cambio de selección.
6. Validar una sola BC de movimiento por herramienta para la ruta angular y
   centro explícito. Mantener `free` y máscara lineal con su semántica previa;
   no reutilizar la máscara para activar componentes angulares.

Prueba mínima manual: crear/editar BC combinada, cambiar ambas tablas por
separado, cancelar y reabrir sin cambios; guardar/reabrir proyecto; rechazo de
tabla inválida y destino incompatible. Verificar que IC/BC lineales conservan
sus controles. Aceptación: compila, checklist y diff documentados.

## G3 — Entrada/salida solver y cierre de compatibilidad

Propiedad: `src/io/InputReader.cpp`, `src/io/InputWriter.cpp` y pruebas IO
existentes o una fixture acotada. Reusar datos y validación G1/G2.

- Exportar vector/centro/modo angular y tabla/factor cuando corresponda,
  asignando IDs sin colisión a amplitudes lineales y angulares de todas las BC.
- Importar desde `.wfinput` resolviendo ID a datos de tabla independientes.
  Exigir modo 0/1 y asociaciones válidas; campos parciales, IDs inexistentes o
  tablas inválidas producen error claro, nunca giro ignorado o degradado.
- Antes de exportar ruta angular comprobar destino MINI3D 3D rígido y ausencia
  de BC duplicadas ambiguas en todas las herramientas. No habilitar restart ni
  checkpoints para esta cinemática; usar controles/configuración reales ya
  existentes, sin inventar soporte del motor.
- Preservar proyecto angular aunque se abra con otra selección de solver;
  impedir exportación incompatible con diagnóstico útil.

Pruebas mínimas: un roundtrip `.wfinput` con dos herramientas y amplitudes
independientes, incluyendo una solo lineal; comprobar valores, referencias de
IDs, factores y centros, sin exigir los mismos IDs al reexportar. Un fixture
antiguo sin giro debe exportar el mismo contenido semántico sin nuevos campos.
Cuando D2 del motor esté cerrada, ejecutar el caso combinado ya dirigido por el
motor desde el archivo exportado; no inventar otra matriz física para GUI.

Aceptación: compila, pruebas IO pasan, ruta completa editar → guardar → reabrir
→ exportar → importar conserva el bloque; revisión separada contra contrato del
motor y pendientes explícitos. No declarar soporte gráfico cerrado antes de
verificar exportación aceptada por MINI3D.

## Fuera de alcance

Implementación cinemática/contacto/cutback del motor, restart de la cinemática
nueva, dinámica libre, cambios constitutivos, ejes locales, expresiones analíticas
y validación física del giro libre de la preforma. No modificar código durante
esta tarea de planificación.
