# Estado plástico inicial: compresión tetraédrica

Dos proyectos GUI basados en `compression_tetra_qt`: ambos conservan las mismas piezas, malla, material Hollomon, paso implícito, simetrías X/Y, contacto y velocidad de la herramienta superior. Solo cambia el valor elemental uniforme de `pl_strain`.

- `pl_strain_0/demo.wfmodel`: 2633 Tet4 con `pl_strain = 0.0`.
- `pl_strain_0p5/demo.wfmodel`: 2633 Tet4 con `pl_strain = 0.5`.

## Abrir y revisar

Abre el `demo.wfmodel` de la variante en WeldFormGUI. La pieza deformable (ID 3) conserva la historia en `mesh.initialState`; no necesita el VTK para reabrir el proyecto. En el árbol expande/edita la pieza y revisa el estado de historia inicial. Para permitir la exportación, elige **Conservar historia con Material actual** (Material 0) y guarda el proyecto; esa confirmación queda pendiente intencionalmente en ambos archivos para no inventar una decisión del usuario. Luego usa **File → Write JSON Input**.

Los `initial_plastic_state_*.vtk` son fixtures de importación sintéticos: se generaron directamente de `demo_part_3.bdf`, contienen solo los 2633 Tet4 deformables, `Part_ID=3` y `CELL_DATA pl_strain` constante. Los `sourceCellId` serializados en el proyecto son los índices VTK zero-based de esas mismas celdas, en el orden del BDF. No son resultados de simulación. Sirven para repetir **File → Import Mesh**; selecciona el grupo deformable `Part_ID 3`, importa `pl_strain` y conserva la historia con Material 0.

El paso `implicit.formulation` se omite tal como en el proyecto de origen; el lector GUI aplica el default `rigid_viscoplastic`. La variante 0.5 es un estado inicial sintético, no un campo espacial derivado de una corrida de compresión.

## Reconocer y visualizar el campo

En el diálogo de importación, comprobar el diagnóstico `CELL_DATA pl_strain`, elegir **Usar CELL_DATA pl_strain** y revisar cantidad/mínimo/máximo: 2633 valores con rango 0–0 o 0.5–0.5. Después de abrir el proyecto, el árbol de la pieza muestra **Historia inicial pl_strain: requiere re-confirmación**; tras confirmar Material 0 muestra **decisión vigente**.

La casilla **Confirmo que todos los Tet4 son una sola pieza deformable** corresponde a VTK sin `CELL_DATA Part_ID`, no confirma presencia de historia. Los VTK de estos ejemplos sí tienen `Part_ID=3`, por lo que permiten seleccionar ese grupo directamente. No importarlos sobre el proyecto completo sin eliminar antes la pieza deformable existente: se duplicaría la pieza y la exportación exige exactamente una deformable.

La GUI actual no colorea la malla por `pl_strain` ni permite consultar su valor por elemento. Para ver la distribución espacial, abrir el VTK incluido en un visor externo compatible con `CELL_DATA` y seleccionar `pl_strain`. En estos ejemplos el valor es uniforme.

Validación de esta entrega: revisión estática de JSON, referencias, IDs y campos VTK. No se abrió la GUI ni se ejecutaron simulaciones; abrir/exportar los proyectos aún requiere comprobación en la aplicación.
