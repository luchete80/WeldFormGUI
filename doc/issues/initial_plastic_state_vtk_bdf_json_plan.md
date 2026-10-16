# Plan GUI: preparar una operación desde VTK y exportar BDF + JSON

Estado: puntos 1–12 implementados y revisados por inspección de código; sin builds ni ejecución de pruebas. La integración runtime con WeldForm-Engine queda pendiente de validación. Fecha: 2026-10-16.

## Estado final de esta entrega

La GUI implementa lectura legacy `.vtk`/`.vtu` de Tet4, selección explícita de piezas y modo de historia, almacenamiento propio de `pl_strain`, persistencia portable en `.wfmodel`, y exportación de BDF más sidecar v1 para la ruta de input MINI3D implícita con validación estricta. Las pruebas fuente añadidas cubren lectores, serialización, sidecar, hash, escenarios de fallo y la ruta de IO; no se compilaron ni ejecutaron en esta entrega. El checklist inferior permanece como guía de aceptación y no se marca como runtime completado.

La ruta de historia de input se restringe en GUI a un único deformable 3D con solver implícito rígido-viscoplástico MINI3D, sin restart. Esto es una restricción preventiva de esta integración: la carga y el uso del sidecar por Engine no se verificaron aquí y no se afirma compatibilidad runtime. Sigue pendiente compilar y ejecutar la suite de IO, y validar con Engine que la historia se aplica por EID antes de la primera evaluación constitutiva.

Después de configurar y compilar, ejecutar la suite registrada con `ctest --test-dir <build-dir> -R '^initial_state_vtk_reader$' --output-on-failure`.

## Objetivo y dependencia

Después de validar la entrega de WeldForm-Engine, permitir importar resultados volumétricos VTK, seleccionar piezas deformables y preparar una operación nueva. Se pueden eliminar o reemplazar herramientas y redefinir condiciones. El Engine recibe exclusivamente BDF + JSON, sin dependencia VTK.

Plan Engine y contrato de referencia: `../../../weldform-engine/doc/issues/mini3d_initial_plastic_state_bdf_json_plan.md`.

## Puntos de integración a revisar

- `src/editor.cpp`: entrada de importación de mallas y preparación del modelo.
- `src/model/Part.*`, `Mesh.*`, `Element.*`, `Node.*`: almacenamiento persistente del estado asociado a entidades, separado de actores de visualización.
- `src/model/Mesh.cpp`: exportación BDF existente, incluidos EID en CTETRA.
- `src/io/ModelWriter.cpp`, `ModelReader.cpp`: persistencia del proyecto y referencias por pieza.
- `src/io/InputWriter.cpp`, `InputReader.cpp`: exportación e importación de la referencia al estado inicial.
- No confundir el dataset de resultados usado por el visor con una malla de modelo editable que conserva campos. Auditar su propiedad y duración antes de programar.

## Contrato compartido v1

Un BDF y un `pieza_1.initial_state.json` por pieza deformable seleccionada. El JSON se genera después de cerrar el BDF, usando sus IDs definitivos; nunca índices VTK ni posiciones de filas.

```json
{
  "format": "weldform_initial_state",
  "version": 1,
  "mesh": "pieza_1.bdf",
  "mesh_sha256": "<SHA-256 hexadecimal de los bytes del BDF final>",
  "element_fields": {
    "pl_strain": {"101": 0.25, "205": 0.40}
  }
}
```

- `pl_strain`: deformación plástica equivalente acumulada, adimensional, escalar por tetraedro; no tensor plástico ni incremento.
- IDs: cadenas decimales canónicas positivas, sin ceros iniciales, dentro del rango admitido por el lector BDF. Rechazar claves duplicadas durante el parseo: un DOM JSON convencional podría sobrescribirlas antes de validarlas.
- Valores numéricos finitos y no negativos, sin convertir cadenas ni recortar valores inválidos.
- Cobertura exacta de todos los elementos deformables del BDF asociado. Rechazar IDs desconocidos, faltantes y duplicados. Las piezas sin archivo conservan la inicialización convencional en cero.
- `mesh` se resuelve respecto del JSON; debe identificar el mismo BDF referenciado por la pieza en el input. Verificar SHA-256 de sus bytes antes de cargar el estado. Una modificación de formato del BDF también invalida el hash y requiere regenerar el JSON.
- Un archivo por pieza evita colisiones entre IDs locales de BDF diferentes. Mantener la asociación pieza → BDF → JSON durante el ensamblado global.
- Versión 1 admite exclusivamente `pl_strain`. Rechazar versiones y campos de estado no soportados con diagnóstico; no ignorarlos silenciosamente.
- El futuro `node_fields.temperature` queda reservado conceptualmente, sin incluirlo ni consumirlo en v1. Requerirá unidades explícitas, IDs nodales, cobertura y precedencia térmica definidas en otra entrega.
- JSON UTF-8, números con precisión de ida y vuelta de `double` y escritura independiente de la configuración regional. Sin dependencia nueva de HDF5.

## Alcance físico y exclusiones

Se inicia una operación nueva sobre la geometría deformada anterior, conservando endurecimiento isotrópico mediante `pl_strain`. Materiales y condiciones de la nueva operación provienen del input habitual. No importar `sigma_y`: recalcularlo con la ley material y las condiciones actuales. No importar presión, velocidades, tensiones, tiempo, contacto ni energía acumulada. No equivale a restart ni a transferencia completa de un estado elastoplástico con tensiones residuales.

Temperatura se difiere explícitamente: MINI 3D obtiene temperatura elemental desde `T` nodal; su incorporación exige resolver inicialización térmica y acoplamiento. Daño, recristalización y endurecimiento cinemático quedan fuera.

## Entrega GUI: tareas y comportamiento

1. Importar `.vtk` legacy y `.vtu` como malla volumétrica, usando lectores VTK ya disponibles si están enlazados. Primera versión solo tetraedros lineales deformables. Rechazar entradas exclusivamente superficiales, tipos volumétricos no admitidos y arrays inconsistentes.
2. Mostrar piezas y permitir selección antes de crear entidades del modelo. Usar un campo de identificación de pieza verificado, por ejemplo `Part_ID` si existe. No inferir materiales o piezas solamente por conectividad: una pieza puede tener componentes desconectados. Si falta identificación, pedir asignación explícita o importar como una sola pieza confirmada.
3. Conservar geometría final sin sumar desplazamiento nuevamente. Comprobar la convención real del escritor de resultados; evitar doble aplicación de deformación o transformaciones del proyecto.
4. Leer exclusivamente `CELL_DATA pl_strain` escalar. Si falta, bloquear la importación como pieza con historia y ofrecer una importación convencional explícita sin historia. No sustituir campos ausentes por cero silenciosamente. No convertir `POINT_DATA` a elemental automáticamente.
5. Filtrar herramientas y celdas de contacto sin tratar sus campos de relleno como historia de material. Exigir valores finitos y no negativos para cada tetraedro seleccionado. Mostrar conteo, rango y origen antes de confirmar.
6. Guardar una copia propia del campo junto con las entidades de malla y preservar la asociación durante selección, eliminación, copia y renumeración. Usar un mapa explícito celda VTK → elemento de modelo → EID final BDF. No depender del orden del dataset ni del identificador del actor.
7. Permitir preparar nuevas herramientas, materiales y condiciones. Un cambio de material conserva el escalar solo por decisión explícita del usuario y recalcula la respuesta en Engine; no prometer equivalencia física con el material anterior.
8. Persistir historia y procedencia en el proyecto para que guardar/reabrir no dependa de la existencia del VTK original. Referencias externas empaquetadas o copia interna según contrato actual del proyecto, con prueba de portabilidad.
9. Exportar un BDF convencional por pieza usando la ruta existente. Capturar los IDs realmente escritos y comprobar cobertura. Después generar JSON con esos IDs y el hash del BDF cerrado; no volver a renumerar entre ambos archivos.
10. Generar archivos temporales y validar todo antes de publicar. Como dos renombrados no constituyen una transacción multiarchivo, usar un directorio de generación nuevo y actualizar la referencia del input/proyecto al final; conservar la generación anterior ante fallo. Evitar prometer atomicidad de dos archivos sueltos.
11. Exportar la referencia `initialStateFile` relativa a la ubicación del input, en la asociación por pieza acordada con Engine. Evitar rutas absolutas de la máquina de origen.
12. Transformaciones rígidas de una pieza mantienen el escalar y cambian el BDF/hash; un remallado o cambio topológico invalida la correspondencia. En v1 bloquear exportación con historia invalidada o exigir descartarla expresamente. Transferencia espacial queda diferida.

## Verificación y aceptación

- [ ] Fixture VTK multipieza con historia heterogénea y herramienta: seleccionar una pieza conserva solo sus tetraedros y valores.
- [ ] IDs dispersos, renumeración, eliminación de piezas y reordenación de celdas: JSON coincide con EID finales, nunca con posiciones anteriores.
- [ ] Dos BDF con IDs locales repetidos no mezclan historias.
- [ ] Falta de `pl_strain`, array nodal, datos inválidos y tipos no admitidos producen diagnósticos claros.
- [ ] Guardar, cerrar y reabrir proyecto mantiene historia; mover el paquete exportado conserva referencias.
- [ ] Transformar y exportar no duplica desplazamientos y regenera hash; cambio topológico invalida estado.
- [ ] Un fallo de exportación conserva el paquete anterior y no publica referencias incompletas.
- [ ] Integración con Engine: carga exacta por ID y uso desde primera evaluación constitutiva.
- [ ] Importación y exportación convencional sin historia mantienen su comportamiento.
- [ ] Revisión independiente de requisitos y diff, con pruebas y diferidos documentados.

## Restricciones y diferidos

### Progreso: punto 1

`src/io/InitialStateVtkReader.h` añade `ReadInitialStateVtkMesh`: admite `.vtk` legacy y `.vtu`, valida coordenadas y cardinalidad de arrays, acepta Tet4 y registra por separado celdas superficiales acompañantes. Rechaza entradas sin Tet4 y cualquier celda no admitida. Devuelve un `vtkUnstructuredGrid` con `DeepCopy`, por lo que la geometría y sus arrays ya no dependen de la vida del lector ni del frame temporal del visor. `tetCellIds` y `surfaceCellIds` conservan los índices originales de celda para selección y transferencia; aún no se crean entidades de modelo.

La API establece propiedad local de esta importación, mientras el punto 6 deberá copiar `pl_strain` a almacenamiento asociado a las entidades. Los frames en `src/results/results.h` poseen datasets por frame para visualización, así que no sirven como dueño persistente de la historia de una malla editable. La auditoría añadió lectura de todos los arrays legacy compatibles, chequeo explícito de error de ambos lectores y validación de IDs de conectividad/repetición nodal. Las pruebas registradas cubren `.vtk` con varios escalares, `.vtu` mixto y rechazo superficial. No se ejecutó build; tampoco existe un ejecutable de prueba disponible y no se compiló según la restricción de esta entrega.

Auditoría del punto 1 aceptada por inspección; la validación runtime sigue pendiente porque no se permite compilar CMake en esta entrega.

### Progreso: punto 2

El diálogo `Import Mesh` acepta `.vtk` y `.vtu` y presenta selección de grupos antes de crear entidades. Si existe `CELL_DATA Part_ID`, se exige un array numérico escalar, con una tupla por celda, valores finitos enteros dentro de rango; las filas se agrupan por ese ID y solo se incluyen índices de Tet4. No se infieren piezas por conectividad. Si falta el campo, se solicita confirmación explícita de tratar todos los Tet4 como una pieza. La selección confirmada queda en una estructura temporal propiedad del editor para el commit posterior; cancelar descarta dataset y selección. Por ahora el diálogo informa que confirmar solo prepara la selección y no crea partes, ya que geometría deformada, historia y persistencia se completan en puntos posteriores.

Auditoría del punto 2 aceptada por lectura; el diálogo es solo selección temporal y el commit de entidades queda pendiente de los puntos posteriores.

### Progreso: punto 3

`src/io/InitialStateVtkMeshBuilder.h` añade `ConvertInitialStateVtkTet4CellsToMesh`. Recibe explícitamente los índices VTK seleccionados y retorna una malla `Mesh` desacoplada del `Model`, propietarios RAII de nodos/elementos y el mapa source cell ID → `Element*`. Compacta solo nodos usados, genera IDs consecutivos positivos y establece dimensión 3. Copia las coordenadas de `dataset->GetPoint` directamente; no consulta ni suma arrays `DISP` ni aplica transforms. No se conecta todavía al botón confirmar, a la espera de integrar validación y transporte de historia en puntos 4–6.

La inspección del modelo mostró que `Mesh::addNode(Node*)` no actualiza el contador y `Mesh::addElement(Element*)` clona el elemento sin actualizar el contador. El builder usa `assignValues` para fijar listas y contadores, y conserva propietarios `unique_ptr` porque Mesh guarda punteros crudos y no tiene destructor propietario. El mapa de celdas referencia los mismos elementos usados por la malla.

En `../weldform-engine/src/common/VTKWriter.C:95-103` y `../weldform-engine/src/implicit/VTKWriter.C:70-78`, `Points` se escribe desde `getPosVec3`/`getPosVec3_h`; el escritor common también emite `DISP` por separado en líneas 401–407. En `../weldform-engine/include/common/Domain_d.h:725-728`, `getPosVec3` devuelve directamente el vector de coordenadas del dominio. La fixture de prueba contiene puntos finales y `DISP` no nulo; la prueba fuente verifica que la conversión conserva los puntos, crea un Tet4 y mantiene el mapa de celda. No se ejecutó build ni prueba porque esta entrega no incluye ejecutable de prueba y no se compila CMake.

Auditoría del punto 3 aceptada por lectura; la conversión aislada no publica cambios en `Model`.

### Progreso: punto 4

`InspectInitialStateVtkPlStrain` lee exclusivamente `CELL_DATA pl_strain`, exige array numérico escalar con una tupla por celda y devuelve los valores por índice VTK solo para Tet4. Rechaza valores no finitos o negativos. Informa estados separados para array ausente, presente solo en `POINT_DATA` e inválido; nunca convierte una asociación nodal ni genera ceros por defecto. El diálogo muestra el diagnóstico y exige seleccionar `CELL_DATA pl_strain` o `Importación convencional sin historia`. Si falta el campo, solo permite la opción convencional; los arrays presentes inválidos también bloquean el modo con historia, con opción explícita para descartar historia e importar convencionalmente. El commit de entidades sigue pendiente.

Las pruebas fuente cubren campo válido, ausente, nodal, vectorial, textual, negativo y no finito. No se ejecutaron porque no hay binario de prueba disponible y no se compila CMake.

Auditoría del punto 4 aceptada para el contrato de lectura y selección explícita.

### Progreso: punto 5

`InspectInitialStateVtkPlStrain(mesh, selectedSourceCellIds)` valida forma del campo y solo examina valores de las celdas Tet4 seleccionadas. Los Tet4 no seleccionados con datos inválidos no bloquean la pieza elegida; las celdas superficiales acompañantes se excluyen del mapa, aunque tengan valores de relleno inválidos. El diálogo exige selección explícita de grupos deformables y advierte que `Part_ID` no diferencia herramientas volumétricas; no infiere herramientas desde `pl_strain == 0`. Al cambiar selección recalcula diagnóstico, cantidad, mínimo y máximo para los IDs seleccionados; si el nuevo grupo invalida la historia, limpia el modo de historia previamente escogido y obliga a elegir otra vez.

La prueba fuente multipieza incluye un Tet4 válido, otro con `pl_strain` negativo y una superficie con valor de relleno negativo: seleccionar la primera pieza acepta su rango; seleccionar la segunda bloquea historia con índice de celda de origen. No se ejecutaron pruebas ni build CMake.

Auditoría del punto 5 aceptada. Se eliminó el escaneo lineal del vector `tetCellIds` por cada celda seleccionada: la validación usa rango del dataset, `GetCellType` y conjunto para duplicados. El diálogo conserva el último vector inspeccionado y solo recalcula al cambiar la selección.

### Progreso: punto 6

La confirmación ahora convierte todas las piezas seleccionadas a `Mesh` temporales, valida cobertura de la historia elegida y solo después las agrega al `Model`. Los objetos importados transfieren sus `unique_ptr` a almacenamiento explícito del `Mesh`; el `Mesh` conserva metadatos de origen (`sourcePath`, `Part_ID`, existencia/validez de historia) y el mapa índice de celda VTK → `Element*`. Cada `Element` guarda su propio `optional<double> pl_strain` y `optional<int64_t>` de celda VTK, por lo que copias preservan la asociación y renumeración/eliminación no desplazan valores; al eliminar un elemento importado también se borra su entrada del mapa y se libera con su dueño correcto. Ningún dataset VTK hace falta para leer estos valores después del commit.

Mientras el formato BDF y el guardado `.wfmodel` no serialicen la historia, `Mesh::exportToNASTRAN`, `InputWriter` y `ModelWriter` rechazan la operación antes de escribir; así no generan mallas/JSON que omitan silenciosamente `pl_strain`. La prueba fuente añadida cubre valor/provenance al copiar `Element`, la propiedad y metadatos del `Mesh`, y la eliminación coherente de elemento y mapa. No se ejecutó build ni runtime; la integración UI sigue pendiente de verificación compilada. La preparación completa ocurre antes del primer `Model::addPart`; como la API de modelo no ofrece inserción por lote, una excepción de asignación de memoria dentro de una llamada `addPart` podría dejar una incorporación parcial.

Para los puntos pendientes, revisar instrucciones locales y respetar `prompt.txt` del Engine en cambios compartidos: sin build CMake y con DEVLOG por cambio de código. No modificar librerías de remallado 2D sin autorización.

Diferir temperatura y sus unidades/precedencias, campos nodales, presión, velocidades, tensiones, formatos binarios y transferencia entre mallas distintas. No agregar infraestructura genérica de variables internas. Antes de comenzar tareas acotadas, fijar propiedad del estado en el modelo y contrato de exportación; separar implementación y revisión conforme a la política de agentes aplicable.

Auditoría del punto 6 aceptada por lectura; queda runtime pendiente.

### Progreso: punto 7

La importación con historia exige una decisión por pieza: conservar `CELL_DATA pl_strain` con el material efectivo actual o descartarla. La auditoría de los escritores muestra que el BDF asigna `MAT1` ID 1 a cada malla y los `DomainBlocks` no conectan `Section.materialIndex` con un material de pieza; por eso el material efectivo actual es `Material 0`. La sección asignada se incluye además en el snapshot para invalidar una decisión si cambia, aunque el exportador actual no use esa asociación. Si se edita `Material 0`, cambia la sección/material asignado o la pieza se convierte a rígida, la decisión queda obsoleta; el árbol permite re-confirmar para una pieza elástica o descartar y limpiar sus valores. El guard informa si la decisión falta/está obsoleta y, aun confirmada, mantiene export bloqueado hasta que BDF/JSON serialicen historia en los puntos 8-9.

El fingerprint determinista cubre parámetros elásticos, plásticos, térmicos y tabulados, además del tipo y parámetros que los serializadores consumen. Usa locale clásico; las tablas CSV externas se fingerprintan por contenido y si no están disponibles se bloquea la conservación. No consulta campos mutables de `Plastic_` mediante `const_cast`. La re-confirmación material no cambia `historyValid`, para que una invalidación posterior por topología no pueda rehabilitarse desde este control. Se muestra el aviso de que el Engine recalcula con la ley vigente y no hay equivalencia física garantizada. Prueba fuente cubre edición del material, cambio de `Section.materialIndex`, tipo rígido y descarte de valores/metadatos. La verificación runtime sigue pendiente; no se ejecutó build.

### Auditoría del punto 7

Aceptada. El fingerprint incluye `Plastic_::Et` cuando la ley BILINEAR lo exporta; su snapshot usa locale clásico y no rehabilita historias invalidadas por topología.

### Progreso: punto 8

`ModelWriter` guarda historia y procedencia inline bajo `part.mesh.initialState`, junto con el BDF de geometría actual. Los registros se asocian por EID actual y conservan `sourceCellId`; el archivo VTK de origen no se vuelve a abrir. El snapshot de proyecto usa un propósito explícito de exportación BDF (`ProjectSnapshot`), mientras las rutas de export runtime siguen bloqueadas hasta completar el contrato del punto 9. El escritor comprueba que el BDF se haya generado antes de publicar el JSON; la transacción completa y la protección de la generación previa quedan para el punto 10.

`ModelReader` carga primero el BDF y luego valida y aplica el JSON: EIDs positivos, únicos y con cobertura exacta para historia válida; source-cell IDs únicos no negativos; valores finitos y no negativos; metadatos y decisión material tipados. El parser del proyecto también rechaza claves JSON duplicadas antes de construir el DOM. Versiones o enteros fuera de rango se rechazan sin narrowing. Una historia inválida puede guardarse con diagnóstico y solo registros válidos/asociables; nunca se inventan ceros y permanece bloqueada para runtime. Historia marcada válida con malla vacía se rechaza.

La auditoría detectó que el lector GUI de BDF reasignaba EIDs secuenciales. `NastranReader` y `Mesh::genFromNastranFile` ahora conservan los EIDs del archivo y rechazan EIDs inválidos/duplicados y conectividad referida a nodos ausentes; esto mantiene la clave inline tras reabrir una malla con IDs dispersos o reordenados. Las pruebas fuente de helpers cubren persistencia por EID, geometría BDF actual, mover un paquete de prueba sin VTK, claves duplicadas, enteros unsigned fuera de rango y EIDs BDF dispersos/reordenados. No son todavía un test end-to-end de `ModelWriter`/`ModelReader`; no se ejecutaron builds ni tests runtime.

Auditoría del punto 8 aceptada por lectura. La corrección adicional envuelve errores de parseo BDF con diagnósticos en editor/lectores y construye la malla temporalmente antes de reemplazar la de `Part`, por lo que un EID inválido no escapa a la UI ni deja un mesh parcialmente publicado.

### Progreso: punto 9

`Mesh::exportToNASTRAN` puede devolver el mapa explícito `Element* → EID escrito`; el helper `ExportInitialStatePiece` valida malla 3D no vacía Tet4, conectividad de cuatro nodos distintos, coordenadas y IDs nodales válidos, historia completa finita/no negativa, decisión material explícita y EIDs actuales/de salida positivos, únicos y dentro del campo BDF de ocho columnas. La comprobación de que la decisión material siga vigente corresponde al guard del llamador, por eso InputWriter continúa bloqueado. El sidecar v1 contiene solo `pl_strain`, su ruta de BDF relativa a la carpeta JSON y SHA-256 de los bytes BDF después de cerrar el escritor. La rutina SHA-256 incluye pruebas fuente para los vectores conocidos vacío y `abc`; también hay pruebas de IDs 27/5 en orden disperso, dos piezas con IDs locales repetidos y hash del BDF generado. Sin builds ni runtime.

La escritura todavía va a las rutas solicitadas y el helper no se conecta a InputWriter: publicación en staging/transacción queda para el punto 10 y referencia relativa del input para el punto 11. El BDF de proyecto del punto 8 sigue usando su inline `initialState` independiente.

Auditoría del punto 9 aceptada por lectura. Se añadió preflight de EIDs actuales positivos, únicos y dentro del límite de ocho columnas antes de crear el BDF; los EIDs que devuelve el escritor se vuelven a validar sin asumir que una futura implementación de exportación conservará los IDs de entrada.

### Progreso: punto 10

`ExportGenerationDirectory` reserva con `create_directory` exclusivo un directorio nuevo por exportación y elimina solo ese directorio si la generación falla. `InputWriter` (explícito e implícito) y `ModelWriter` producen sus BDF, STEP, sidecars futuros y recursos auxiliares dentro de esa generación. Antes de publicar el `.wfinput`/`.wfmodel`, comprueban resultados de escritura y cierran archivos; el JSON se escribe a un temporal hermano y se reemplaza atómicamente como último paso. Los recursos de generaciones anteriores se conservan. Los paths del JSON son relativos a su carpeta, y los CSV externos de material se copian dentro de la generación para que el paquete siga siendo portable. El lector de proyecto resuelve esas referencias relativas.

El guard de historia en InputWriter sigue activo hasta integrar el sidecar del punto 11. `ModelWriter` restaura el nombre previo de geometría tras exportar STEP, aplaza cambios de `meshSourceFile` hasta publicar el proyecto y también dirige el `.k` legacy generado al staging. Pruebas fuente nuevas cubren colisiones/directorio preexistente, limpieza limitada a la generación propia, fallo al publicar el temporal y fallo al procesar la segunda pieza conservando la referencia anterior. No se ejecutaron build ni runtime por la restricción vigente.

Auditoría del punto 10 aceptada por lectura. La publicación de settings de Job también usa temporal hermano y reemplazo atómico.

### Progreso: punto 11

`InputWriter` deriva los casos implícitos al escritor MINI3D antes de validar el modo. Solo el input 3D con formulación `rigid_viscoplastic` y una única parte deformable puede exportar historia; `initialStateFile` se añade al `DomainBlocks` correspondiente y el helper genera BDF más sidecar dentro de la generación antes de publicar el input. Se rechazan J2, casos explícitos, 2D y `restartFile`. Es un límite preventivo de GUI; la carga y aplicación real por Engine no se validó.

`InputReader` rechaza claves JSON duplicadas y exige esa misma configuración MINI3D, una única pieza file-backed, ningún `restartFile` y ninguna referencia de historia desde `RigidBodies`. El sidecar v1 valida esquema/campos exactos, referencia BDF relativa que resuelve al mismo archivo del bloque, SHA-256 de bytes, CTETRA4 reales (incluye escaneo de cards para rechazar mezcla de CQUAD4/CTETRA10/continuaciones), cobertura exacta EID, claves EID decimales canónicas y valores finitos/no negativos. Bufferiza todos los datos y solo después los asigna. El lector no inventa IDs de celda VTK que v1 no almacena.

Como consecuencia, el formato inline de `.wfmodel` ahora permite omitir `sourceCellId` solo cuando la historia ya proviene de un sidecar y no hay índice VTK original. Historias importadas directamente desde VTK mantienen el requisito de source-cell IDs únicos. Pruebas fuente cubren paquete BDF/sidecar movido, roundtrip inline sin provenance inventada, hash/cobertura/claves duplicadas, IDs no canónicos, mezcla CQUAD4 y CTETRA10. La actualización de settings de Job publica atómicamente y bloquea combinar `restartFile` con sidecar. Runtime y compilación siguen pendientes, sin builds conforme a la restricción.

Auditoría del punto 11 aceptada por inspección de código; runtime pendiente.

### Progreso: punto 12

Al adjuntar historia válida, `Mesh` guarda un snapshot de los punteros de elementos, la conectividad ordenada de cada elemento y el conjunto de nodos. Las coordenadas y los IDs no forman parte del snapshot: traslación, rotación y renumeración preservan la asociación `Element* → pl_strain`; agregar o quitar entidades, reasignar arrays de malla o cambiar conectividad invalida `historyValid` con diagnóstico. Las verificaciones de exportación de input/BDF sidecar y serialización de proyecto observan la invalidez; guardar `.wfmodel` la conserva en provenance en vez de rehabilitarla. La decisión de material no refresca el snapshot.

`Part::setMesh`, generación BDF/Gmsh y borrado rechazan reemplazar una malla que aún tenga historia, incluso si fue invalidada; primero hay que descartarla explícitamente. Editor y WorkflowAPI bloquean escala, corte y remallado antes de efectos destructivos. La rotación rígida admite ejes globales para mallas sin geometría, rota los nodos sobre el pivot seleccionado y reconstruye el `GraphicMesh` sin borrar estado. La traslación actualiza los nodos incluso cuando no hay `GraphicMesh`. El modelo retiene el storage compartido de entidades importadas mientras sus listas legacy de punteros puedan referenciarlas, evitando dangling pointers al descartar/remallar y conservar el `Model`. No se modifica la biblioteca de remallado.

Pruebas fuente cubren traslación/rotación y hash BDF actualizado con escalar estable, renumeración, edición directa de conectividad, eliminación de elemento, bloqueo de sidecar, persistencia de historia invalidada y rechazo de reemplazo/borrado por `Part`. No se ejecutaron build ni runtime. La suite registrada para correr tras compilar es `initial_state_vtk_reader_tests`; la integración con Engine permanece pendiente de verificación.

Auditoría del punto 12 aceptada por inspección de código. La compilación, ejecución de pruebas y validación runtime con Engine siguen pendientes.
