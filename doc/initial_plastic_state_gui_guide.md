# Guía GUI: importar `pl_strain` desde VTK y preparar el input

Esta guía describe la ruta disponible en WeldFormGUI para importar una malla Tet4 con `CELL_DATA pl_strain`, guardarla en un proyecto y generar un input MINI3D.

## 1. Preparar el modelo

1. Abre un proyecto `.wfmodel` existente o crea uno desde el árbol `Models`: clic derecho sobre `Models` → `New Model`.
2. En la ventana `Model`, selecciona `Solid 3D` y confirma con `Ok`. Si el modelo ya existía, abre su menú contextual con clic derecho sobre `Model (...)` y elige `Edit` (también puedes hacer doble clic); confirma `Solid 3D` con `Ok`.
3. Crea o revisa `Material 0` antes de importar historia. En el árbol, clic derecho en `Materials` → `New`. Completa `Density`, `Elastic Mod` y `Poisson Mod`; configura la ley plástica en `Plastic` → `Yield Criteria` y sus parámetros, si corresponde; termina con `Ok`. Si ya tienes `Material 0`, puedes abrirlo con doble clic o con clic derecho → `Edit`.

La decisión de conservar historia se valida contra `Material 0`, que es el material efectivo que usa actualmente esta ruta de exportación. Cambiar ese material después vuelve obsoleta la decisión y exige confirmarla otra vez en el árbol de piezas o descartar la historia.

4. Crea un paso desde el árbol `Steps`: clic derecho → `New`. En la ventana del paso elige `Implicit`; dentro de `Implicit Solver`, selecciona `Formulation` → `Rigid Viscoplastic`. Para la ruta de historia, el modelo debe seguir en `Solid 3D`. Confirma con `Ok`. La exportación de input usa el primer paso (`Step 0` en el orden interno); si ya existe un primer paso, edítalo desde `Steps` → menú contextual `Edit` en `Step ...` en vez de añadir un segundo.
5. Agrega las condiciones y piezas de herramienta que necesite tu operación usando los controles habituales del modelo. La historia importada se aplica a la pieza deformable seleccionada; las piezas rígidas de herramienta se conservan como partes separadas. Para exportar input con historia, debe quedar exactamente una parte deformable mallada.

Guarda el proyecto desde `File` → `Save`. Si todavía no tiene nombre, elige la ubicación y el nombre `.wfmodel` en el diálogo. Este paso fija también la carpeta desde la que se escribirán el input y sus archivos asociados.

## 2. Importar la malla VTK

1. Elige `File` → `Import Mesh` y selecciona el archivo `.vtk` legacy o `.vtu`.
2. En `Select VTK Mesh Parts`, revisa el nombre del archivo, la cantidad de Tet4 y el diagnóstico de historia.
3. Si aparece una lista `Part_ID ... — pieza deformable`, marca solo los grupos deformables que quieres importar. `Part_ID` no distingue automáticamente una pieza deformable de una herramienta volumétrica: excluye las herramientas. Las celdas superficiales acompañantes se ignoran.
4. Si el archivo no contiene `CELL_DATA Part_ID`, marca `Confirmo que todos los Tet4 son una sola pieza deformable` solamente cuando sea cierto para todos los tetraedros del archivo. No se separan piezas por conectividad.
5. Si el diagnóstico indica `CELL_DATA pl_strain` válido, elige `Usar CELL_DATA pl_strain` para asignarlo. Para cada pieza/grupo seleccionado elige explícitamente `Conservar con Material 0 actual` o `Descartar historia`. La ventana muestra origen, cantidad, mínimo y máximo del campo seleccionado. Conservarlo no garantiza equivalencia física con el material de la simulación anterior: el solver calcula la respuesta con la ley actual.
6. Si no hay campo, si solo existe en `POINT_DATA`, o si sus valores son inválidos, no puedes importarlo como historia. Elige `Importación convencional sin historia` para traer la geometría sin `pl_strain`; no se crean ceros como sustituto.
7. Pulsa `Importar piezas`. Pulsa `Cancelar` si no quieres crear las partes.

La importación con historia admite Tet4 lineales. Otros tipos volumétricos o arrays inconsistentes generan un diagnóstico y rechazan el archivo.

## 3. Revisar historia y editar la operación

En el árbol de cada parte importada aparece el estado de la historia inicial. Si la decisión de material dejó de ser vigente, usa `Conservar historia con Material actual` para confirmarla con la configuración actual o `Descartar historia` para eliminar expresamente el campo y su metadato.

La traslación y rotación rígida de la malla conservan los valores asociados a cada elemento y actualizan la geometría que se exportará. Renumerar nodos o elementos tampoco desplaza la historia a otra celda. Cambiar conectividad, agregar o quitar elementos invalida la historia. El árbol y la exportación mostrarán que hay que descartarla antes de reemplazar o volver a mallar esa parte. Escalar, cortar o remallar una pieza que aún tiene historia está bloqueado; descártala explícitamente antes de esas operaciones.

Guarda de nuevo con `File` → `Save` después de cambiar el modelo. El `.wfmodel` guarda la geometría actual, la historia y su procedencia; no necesita reabrir el VTK original. Para reabrir el proyecto usa `File` → `Open` y selecciona el `.wfmodel`; conserva junto a este archivo su carpeta oculta de generación `.wf_<nombre>_generation_*`.

## 4. Generar el input

1. Confirma que el modelo sea `Solid 3D`, que tenga un paso `Implicit` con `Rigid Viscoplastic`, y que haya exactamente una parte deformable mallada con historia. No combines esta ruta con `J2`, un paso explícito, análisis 2D o `restartFile`.
2. Guarda el proyecto con `File` → `Save` si hiciste cambios.
3. Elige `File` → `Write JSON Input`. El comando escribe `<nombre>.wfinput` en la carpeta base del proyecto; no abre un diálogo de ubicación. Si el modelo todavía no tiene nombre, primero usa `File` → `Save` y guárdalo.
4. El input apunta al BDF y al sidecar JSON de estado inicial mediante rutas relativas. Junto al input se crea una carpeta oculta de generación, por ejemplo `.wf_<nombre>_generation_1`, que contiene el BDF, el sidecar y los recursos asociados. Para reabrirlo usa `File` → `Open Input` y selecciona el `.wfinput`. Conserva el `.wfinput` y la carpeta de generación como un paquete al mover o compartir el input; no se requiere el VTK de origen.

## Límites actuales

La GUI restringe el input con historia a una sola pieza deformable 3D en el solver implícito rígido-viscoplástico MINI3D. La generación y lectura de los archivos están implementadas en GUI, pero el consumo y uso efectivo del sidecar por WeldForm-Engine todavía requiere validación runtime. No interpretes que el archivo generado ya fue comprobado con una ejecución de Engine. La función `File` → `Save As..` no está implementada; elige la carpeta del proyecto la primera vez que guardes con `File` → `Save`.
