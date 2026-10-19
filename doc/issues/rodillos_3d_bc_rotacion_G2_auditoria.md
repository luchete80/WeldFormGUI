# Auditoría de implementación G2: editor de velocidad angular

## Resultado

Se corrigió la implementación de G2 en `src/bc_dialog.cpp` y
`src/bc_dialog.h`, conforme a `rodillos_3d_bc_rotacion_plan.md`. La GUI ofrece
el bloque angular solo para BC Velocity de una Part rígida en modelo Solid3D,
con Step0 implícito y formulación rigid-viscoplastic (MINI3D). El editor conserva
la semántica lineal y de IC, y el bloque angular continúa siendo opcional.

## Requisitos cubiertos

- La compatibilidad consulta `Model::getAnalysisType()`, dimensión y Step0 real;
  no infiere el solver a partir de la geometría o del tipo de Part.
- Velocidad angular y centro se editan con controles `double` de X/Y/Z. La UI
  indica rad/s, regla de la mano derecha, coordenadas globales y que el centro
  inicial explícito acompaña la traslación de la pieza. La tabla escala las tres
  componentes.
- La tabla angular inválida se mantiene intacta al dibujar. No se ordena para
  graficar ni se reemplaza con valores por defecto. El editor limita el acceso a
  las filas presentes y ofrece un botón explícito para restablecerla. La
  validación comprueba números finitos, modo 0/1, al menos dos filas, longitudes
  iguales, tiempos no negativos y estrictamente crecientes; no exige que el
  primer tiempo sea cero. Reutiliza `BoundaryCondition::isAngularMotionValid()`
  para mantener un único contrato de datos angulares.
- La selección de Part se resuelve por ID en cada frame y usa nombres e IDs
  actuales. El estado ya no comparte un índice `static` entre diálogos. Cerrar,
  cancelar o cerrar una ventana colapsada reinicia el estado lineal, angular,
  de selección y de filas tanto para BC como para IC.
- La BC angular existente conserva su checkbox cuando cambia a un destino
  incompatible, y el guardado informa que se debe corregir el destino o
  desactivar explícitamente el giro.
- Si se usa la ruta angular, se comprueba una sola BC Velocity por cada Part
  rígida para todas las herramientas: detecta duplicados lineal-lineal y
  lineal-angular. La verificación sigue la ruta de movimiento rígido del motor,
  que cuenta BC Velocity sobre zonas rígidas; no añade restricciones a
  Displacement o Symmetry. Si no hay giro propuesto ni existente, se conserva el
  comportamiento para proyectos legados con BC lineales duplicadas.
- Al guardar una ruta angular se exige finitud también en el vector, factor y
  muestras de amplitud lineales que la acompañan; si se combina con tabla lineal,
  también se valida su forma y el orden estricto de tiempos. Cuando el giro
  está activado en la condición editada, la tabla lineal no se normaliza
  automáticamente durante la edición. El resto de las BC lineales mantiene su comportamiento actual. Las
  escrituras exitosas marcan el modelo como modificado.

## Evidencia y límites

- `git diff --check`: pasó.
- `ctest --test-dir /home/weldform-pc/Numerico/WeldFormGUI_bin -R '^angular_motion_project_io$' --output-on-failure`:
  pasó (1/1), incluyendo roundtrip de proyecto con tablas independientes,
  centro no nulo y lectura de proyectos antiguos. Se autorizó escritura del
  log de CTest en el directorio externo de build.
- `cmake --build /home/weldform-pc/Numerico/WeldFormGUI_bin --target imgui_vtk -j2`:
  pasó tras autorizar acceso al directorio externo de build, y compiló
  `bc_dialog.cpp`; el compilador informó una advertencia previa en `editor.cpp`
  sobre el retorno ignorado de `chdir`.
- Un harness ImGui headless temporal en `/tmp/g2_audit_check.cpp` ejercitó el
  solver ausente, explícito, J2, 2D y MINI3D; validación de finitos y tablas;
  precisión double inactiva; preservación de tabla malformada; guardado de
  tablas independientes; rechazo de duplicados en ambas direcciones; rechazo
  por solver incompatible y desactivación explícita; cancelación y reinicio. El
  resultado reportado fue: `G2: solver, validation, idle precision, malformed
  table preservation, independent saves, duplicate rejection both directions,
  incompatible solver rejection/disable, cancel and reset passed`.
- La comprobación manual interactiva completa del checklist de G2 queda
  pendiente. En particular: crear/editar combinación y modificar tablas por
  separado, cancelar y reabrir, persistir/cargar proyecto y confirmar rechazo
  visual de destino incompatible. Este cambio no implementa G3 ni modifica el
  lector/exportador del solver.

El modelo solicitado para esta entrega fue `gpt-6-luna`, esfuerzo `high`; la
identidad efectiva del modelo de ejecución no está confirmada.
