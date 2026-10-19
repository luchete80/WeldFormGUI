# Auditoría G3: IO de velocidad angular

## Estado

G3 de GUI implementada. El contrato del motor D2 está cerrado según
`../weldform-engine/doc/issues/rodillos_3d_eje_movil_D2_auditoria.md`; el caso
combinado del motor se ejecuta desde la exportación GUI durante esta auditoría.
El runtime combinado D2 pasó después de corregir la lectura de BDF de solo
lectura. La repetición final se ejecutó además sin escalación contra BDF del
checkout de motor con el checker D2 combinado, confirmando lectura de los
archivos fuente de solo lectura.

## Cambios

- `InputWriter` asigna IDs correlativos globales a tablas lineales y angulares.
  Antes de generar archivos valida solver 3D MINI3D implícito, destino rígido
  mallado, modo angular, tablas angulares y lineales válidas, y BC de movimiento
  ambiguas en cada herramienta cuando hay una ruta angular. Bloquea BC angular
  de tipo/destino incompatible, incluyendo Symmetry, NodeSet y BC no Velocity.
  El proyecto conserva el bloque angular aunque se seleccione otro solver; la
  exportación lo rechaza con diagnóstico.
- `InputReader` interpreta `angularValueType` ausente como constante y factor
  ausente como 1. Rechaza bloques parciales, tablas/IDs inválidos o duplicados,
  centros/vectores incorrectos, giro sobre NodeSet, target que no sea una Part
  rígida única y duplicados de Velocity en herramientas. El chequeo estricto de
  amplitudes solo se activa con campos angulares; los inputs antiguos sin giro
  conservan el comportamiento previo de lectura de amplitudes.
- La validación angular estricta inspecciona IDs raw de RigidBodies antes de que
  Model reasigne duplicados; valida ids de rango entero, tool vectors de tres
  componentes, tablas lineales de todas las herramientas, Set IDs que aliasen
  zonas rígidas (también sin BC NodeSet) y campos angulares fuera de BC directas.
  Respeta `valueType=0` lineal aunque aparezca una referencia `amplitudeId`, de
  acuerdo con MINI3D.
- `JobDialog` deshabilita restart/checkpoint para inputs o modelos con giro.
  `Job` bloquea también settings persistidos que soliciten restart/checkpoint en
  la cinemática angular.
- `NastranReader` abre BDF solo con `ios::in`; antes pedía lectura/escritura y
  podía devolver una malla vacía sin marcar error en archivos de solo lectura.
  Un fallo de apertura o reapertura ahora marca `readValid=false` y se informa.
- La prueba IO tiene un modo `--roundtrip-input <entrada> <salida>` que importa
  `.wfinput`, serializa y reabre `.wfmodel`, y reexporta `.wfinput`. La fixture
  fuente prueba tabla angular/lineal independiente, IDs no colisionados, valores
  por defecto, reject cases de amplitudes duplicadas (incluida primera tabla
  inválida), duplicados de movimiento por herramienta, alias Set/zone, IDs
  grandes, vectores parciales/no finitos, target omitido, tipo BC incorrecto,
  solver incompatible con preservación del proyecto y restart/checkpoint.
  También reexporta un input legacy sin giro y comprueba su semántica lineal.

## Verificación

Evidencia ejecutada:

```sh
cmake --build /home/weldform-pc/Numerico/WeldFormGUI_bin \
  --target angular_motion_project_io_tests imgui_vtk -j2
ctest --test-dir /home/weldform-pc/Numerico/WeldFormGUI_bin \
  -R '^angular_motion_project_io$' --output-on-failure
python3 -B src/io/tests/check_angular_solver_runtime.py \
  --output /tmp/wfg_g3_audit_runtime_escalated
git diff --check
```

`angular_motion_project_io` pasó 1/1. El runtime reportó: “GUI project/input
roundtrip accepted by MINI3D; two tools, knots, geometry, shear and two rollbacks
verified”. La primera corrida falló al leer BDF externos y produjo mallas
vacías porque el lector abría con permisos de escritura; tras cambiar a
`ios::in`, el checker D2 combinado pasó, tanto escalado como sin escalación.
`git diff --check` pasó.

`initial_state_vtk_reader` no pasó: su aserción “element removal invalidates
history rather than reassigning values” contradice el contrato actual de
`removeElements`/`validateInitialStateTopology` (una historia inválida provoca
que la validación posterior no intente reasignarla). El test del fallo de
apertura Nastran y los casos de lectura/exportación anteriores sí se ejecutaron
antes de esa aserción. Este fallo no involucra el movimiento angular.

## Revisión y límites

- No se modifica el motor ni se afirma compatibilidad de restart/checkpoint; la
  GUI deshabilita esa ruta y el motor D2 la rechaza.
- La ruta angular continúa limitada a Velocity de Part rígida en 3D MINI3D.
- La validación física se delega al caso combinado de D2; no se agrega otra
  matriz de ensayos.
- Revisión separada del diff realizada por el agente principal contra el plan
  y `RigidMotion3DInput.h` del motor. Se repitieron de forma independiente los
  casos de alias Set/zone, duplicado de herramienta, vector lineal parcial,
  ID fuera de rango y clave angular en la raíz: todos fallaron en importación
  (código 10 del harness). El caso de modo lineal constante con `amplitudeId`
  conservó modo 0 al reexportar.
- La corrida final con todas las correcciones pasó:
  `python3 -B src/io/tests/check_angular_solver_runtime.py --output /tmp/wfg_g3_runtime_final`.
  Los logs y archivos exportados están en ese directorio. Se verificaron los
  dos cuerpos, quiebres, geometría, shear friction y dos rollbacks usando el
  checker del motor; no se modificó la configuración exportada para lograrlo.
- Las correcciones se delegaron con modelo solicitado `gpt-6-luna` y esfuerzo
  `high`, según lo pedido por el usuario. El runtime aceptó esos parámetros;
  no confirmó la identidad efectiva del modelo. La revisión separada y las
  ejecuciones integradas las realizó el agente principal.
- Los requisitos IO de G3 quedan auditados con la limitación de la suite de
  historia inicial indicada arriba. El checklist visual interactivo del editor
  continúa siendo el pendiente de G2 registrado en su propia auditoría.
