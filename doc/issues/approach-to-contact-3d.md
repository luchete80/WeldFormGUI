# Initial positioning: Approach to contact (3D)

## Objetivo

Agregar al editor de una pieza rígida seleccionada una acción explícita que la traslade, sin rotarla, a lo largo de una dirección dada hasta el primer contacto con una pieza deformable 3D. La primera versión debe permitir previsualizar el resultado, resaltar la región candidata y aplicar la traslación solamente después de una confirmación del usuario.

Esta operación es de preprocesamiento. La GUI modifica las coordenadas reales de la malla, guarda cómo se obtuvo la posición y exporta el BDF ya reposicionado. El solver no debe hacer `autoSnap`.

## Alcance de la primera entrega

- Solo `Solid3D`.
- Una pieza rígida móvil con malla superficial triangular y una pieza deformable objetivo con superficie extraíble.
- Dirección automática desde una `VelocityBC` aplicada a la pieza rígida y dirección manual `(dx, dy, dz)`.
- La magnitud de la velocidad no participa: siempre se normaliza el vector.
- `targetGap >= 0`, con valor inicial `0.0`.
- `maximumMovement > 0`, expresado en las unidades geométricas del modelo. La interfaz puede mostrar `mm`, pero no debe introducir una conversión oculta: debe usar la unidad activa del modelo cuando exista.
- `Preview` no modifica `Part`, `Mesh`, `Geom` ni el estado de undo/redo.
- `Apply` usa el camino existente `Editor::applyPartTranslation(...)` para que geometría, polydata y nodos exportables permanezcan sincronizados.
- Sin cambios iniciales al algoritmo de contacto ni a la iteración de Picard del solver.

Fuera de alcance del MVP: 2D/axisimetría, rotación automática, varias piezas deformables simultáneas, deformación de la herramienta, movimiento automático al abrir/cargar el modelo y `autoSnap` dentro del solver.

## Comportamiento visible

Agregar al menú contextual de una pieza rígida mallada:

```text
Initial positioning
  Approach to contact...
```

El diálogo debe contener:

```text
Initial positioning
[ Approach to contact ]

Target part: <deformable part>
Direction: [Automatic from velocity BC | Manual]
Manual direction: dx, dy, dz
Target clearance: 0.0
Maximum movement: 10.0

[Preview] [Apply] [Cancel]
```

Después de `Preview`, mostrar como mínimo:

```text
Current minimum directional gap: 0.188
Proposed translation: (0, 0, -0.188)
Final target gap: 0.0
Contact region: 2 candidate faces
```

Reglas de habilitación y errores:

- Mostrar la acción solo para `Part::getType() == Rigid`, análisis `Solid3D` y malla disponible.
- Listar como objetivos solo piezas `Elastic`, distintas de la móvil y con malla válida.
- En modo automático buscar una única `VelocityBC` con `ApplyToPart` y `targetId == rigidPart.id`. Si no existe, el vector es nulo o hay más de una dirección no colineal, deshabilitar `Preview` y explicar el problema. Las BC colineales pueden resolverse a la misma dirección normalizada.
- Respetar la máscara de grados de libertad de la BC al formar el vector efectivo.
- Rechazar dirección manual casi nula, `targetGap < 0`, `maximumMovement <= 0`, mallas vacías y superficies no triangulables.
- Si no hay contacto dentro del límite, no proponer movimiento y mostrar `No contact within maximum movement`.
- Si las superficies ya se interpenetran, no mover y mostrar un error específico.
- `Apply` solo se habilita para el último preview válido. Cualquier cambio de pieza, dirección, clearance, límite o geometría invalida ese preview.
- `Cancel` elimina únicamente actores/overlays de preview. Nunca debe “deshacer” una transformación que no se aplicó.

## Modelo de datos y persistencia

Agregar a `Part` un valor propio, sin depender del diálogo, por ejemplo:

```cpp
enum class InitialPositioningMode { None, ApproachToContact };
enum class DirectionSource { Manual, VelocityBC };

struct InitialPositioning {
  InitialPositioningMode mode = InitialPositioningMode::None;
  DirectionSource directionSource = DirectionSource::Manual;
  double3 direction = make_double3(0.0, 0.0, 0.0); // normalizada y efectivamente usada
  double targetGap = 0.0;
  double3 appliedTranslation = make_double3(0.0, 0.0, 0.0);
  int targetPartId = -1;
};
```

Exponer getters/setters en `src/model/Part.h`; evitar hacer al diálogo `friend` de los nuevos datos. `appliedTranslation` es la traslación acumulada producida por esta acción, no la posición absoluta ni todos los movimientos manuales históricos.

En `src/io/ModelWriter.cpp`, dentro de cada entrada `model.parts[]` rígida que haya aplicado la acción, escribir:

```json
"initialPositioning": {
  "mode": "approachToContact",
  "directionSource": "velocityBC",
  "direction": [0.0, 0.0, -1.0],
  "targetGap": 0.0,
  "appliedTranslation": [0.0, 0.0, -0.000188],
  "targetPartId": 2
}
```

En `src/io/ModelReader.cpp`, leer el objeto de forma opcional y tolerante. Un `.wfmodel` anterior, un modo desconocido o campos ausentes deben producir los defaults sin impedir la carga. No volver a aplicar `appliedTranslation` al cargar: el BDF guardado por `ModelWriter` ya contiene las coordenadas transformadas. El objeto es auditoría/reproducibilidad, no una transformación pendiente.

Mantener el contrato actual de `src/io/InputWriter.cpp`: exportar el BDF desde las coordenadas actuales y escribir `RigidBodies[].start = [0,0,0]`. Agregar una prueba de regresión que impida la doble traslación.

## Utilidad geométrica independiente

Crear `src/geometry/ApproachToContact.h/.cpp` (o `src/geom/` si se prefiere mantener un único módulo geométrico), sin dependencias de ImGui ni de `Editor`.

API sugerida:

```cpp
struct ApproachToContactRequest {
  vtkPolyData* movingSurface = nullptr;
  vtkPolyData* targetSurface = nullptr;
  std::array<double, 3> direction;
  double targetGap = 0.0;
  double maximumMovement = 0.0;
  double tolerance = 0.0; // si es 0, calcularla desde la diagonal del modelo
};

enum class ApproachStatus {
  Success,
  InvalidInput,
  AlreadyPenetrating,
  NoContactWithinLimit,
  ClearanceExceedsGap
};

struct ApproachToContactResult {
  ApproachStatus status;
  double contactDistance = 0.0;
  double translationDistance = 0.0;
  std::array<double, 3> translation;
  std::vector<vtkIdType> movingContactFaces;
  std::vector<vtkIdType> targetContactFaces;
  std::string diagnostic;
};

ApproachToContactResult computeApproachToContact(
    const ApproachToContactRequest& request);
```

La utilidad debe trabajar sobre copias/superficies de solo lectura y devolver IDs estables de celdas para el resaltado. Triangularizar entradas mediante `vtkTriangleFilter` conservando un mapeo al ID de celda original.

### Algoritmo 3D

No usar la distancia euclídea mínima ni `vtkDistancePolyDataFilter` como criterio de movimiento: no representan el primer choque a lo largo de una dirección.

1. Validar y normalizar `direction` a `d`.
2. Construir/reutilizar un índice espacial de la superficie objetivo. Para cada triángulo móvil, consultar candidatos con el AABB barrido entre `t=0` y `t=maximumMovement`; no recorrer todos contra todos.
3. Para cada pareja candidata de triángulos calcular el intervalo de colisión bajo traslación lineal usando SAT continuo. Probar los ejes no degenerados de las dos normales de cara y los nueve productos cruzados arista-arista. En cada eje, proyectar ambos triángulos y resolver el intervalo de tiempo en que se solapan; la intersección de todos los intervalos da `[tEnter, tExit]`.
4. El menor `tEnter >= 0` global es el primer contacto. Agrupar como región de contacto todas las parejas con `abs(tEnter - firstContact) <= contactTolerance`.
5. Si existe solapamiento volumétrico/intersección en `t=0` más allá de la tolerancia, devolver `AlreadyPenetrating`. Un contacto tangencial inicial dentro de tolerancia es contacto a distancia cero, no penetración.
6. Calcular `translationDistance = firstContact - targetGap`. Si el resultado es negativo, devolver `ClearanceExceedsGap`; no mover en sentido contrario en el MVP.
7. Devolver `translation = d * translationDistance`.
8. Verificar globalmente el estado propuesto contra todos los candidatos: no puede existir una entrada anterior a la propuesta ni cruce de superficie. Para `targetGap == 0`, aceptar tangencia dentro de tolerancia; probar además una posición infinitesimal posterior (`translationDistance + penetrationProbe`) para clasificar el lado de penetración y detectar casos degenerados.

La tolerancia por defecto debe escalar con la geometría (por ejemplo, `max(1e-12, diagonalAABB * 1e-9)`) y quedar centralizada. No codificar un epsilon en milímetros.

Para el broad phase se puede usar un locator de VTK si permite consultar celdas por bounds en la versión enlazada; si no, implementar un BVH/AABB pequeño en esta utilidad. Cachear el índice por identidad/revisión de la malla objetivo. La corrección tiene prioridad sobre la caché: cualquier modificación de puntos/celdas debe invalidarla.

Casos degenerados (triángulos de área casi cero, ejes SAT casi nulos, superficies abiertas) deben reportarse en `diagnostic`; los triángulos degenerados se omiten, pero si no quedan triángulos válidos la solicitud falla.

## Integración con el editor

Archivos previstos:

- `src/geometry/ApproachToContact.h/.cpp`: cálculo puro, broad phase y SAT continuo.
- `src/CMakeLists.txt` o el `CMakeLists.txt` del módulo elegido: registrar fuentes y componentes VTK necesarios.
- `src/approach_to_contact_dialog.h/.cpp`: estado y render ImGui; no realiza geometría directamente.
- `src/editor.h/.cpp`: apertura desde menú contextual, resolución de BC/objetivo, construcción de superficies, preview, apply e invalidación.
- `src/model/Part.h`: estado persistible.
- `src/io/ModelWriter.cpp` y `src/io/ModelReader.cpp`: round trip de `initialPositioning`.

No extender `graphics/PolyDataCollisionSystem.h`: hoy mezcla actores, distancia euclídea y colisión interactiva aproximada; no cumple el contrato direccional ni es una base testeable para este cálculo.

Flujo de `Preview`:

1. Capturar IDs de pieza, revisión de mallas y parámetros.
2. Obtener polydata en coordenadas de mundo. Preferir las coordenadas reales de `Mesh`; evitar aplicar de nuevo un `UserTransform` del actor.
3. Ejecutar `computeApproachToContact`.
4. Dibujar una copia fantasma del rígido en la posición propuesta y resaltar las caras candidatas de ambas superficies. Los actores de preview deben ser no seleccionables.
5. Guardar el resultado y la firma de entrada solo en estado transitorio del editor.

Flujo de `Apply`:

1. Comprobar que la firma continúa vigente; si no, exigir un preview nuevo.
2. Recalcular inmediatamente antes de aplicar para evitar usar geometría obsoleta.
3. Si el resultado coincide dentro de tolerancia, invocar una sola vez `Editor::applyPartTranslation(part, tx, ty, tz)`.
4. Actualizar `Part::initialPositioning`, marcar el modelo como modificado y limpiar el preview.
5. Integrar una acción de undo/redo que traslade por `-translation`/`+translation` y restaure el metadato anterior/nuevo. No dejar `Apply` fuera del historial.
6. Forzar actualización/render de VTK y ejecutar otra verificación con la geometría ya movida. Ante un fallo inesperado, revertir inmediatamente la misma traslación y no crear la acción de undo.

El mecanismo existente `Editor::applyPartTranslation` ya mueve `Geom`, el polydata visual y los nodos de `GraphicMesh`; debe seguir siendo el único punto de mutación. Revisar el caso de una pieza con geometría y malla para evitar que una representación sea trasladada dos veces.

## Pruebas

Crear un ejecutable de pruebas geométricas sin ventana/render interactivo y registrarlo con CTest. No agregar los casos al actual `src/test.cpp`, que es un ejecutable de aplicación, no una suite unitaria.

Casos mínimos de la utilidad:

1. Dos planos triangulados paralelos, dirección normal: distancia y vector exactos dentro de tolerancia.
2. Mismo caso con velocidad `(0, 0, -20)`: mismo resultado que `(0, 0, -1)`.
3. Contacto inclinado: encuentra el primer evento direccional, distinto de la distancia euclídea mínima.
4. Dos regiones que llegan simultáneamente: devuelve ambas caras candidatas.
5. Una región secundaria contacta antes: selecciona el mínimo global y no atraviesa esa zona.
6. Contacto arista-arista sin vértice que impacte una cara: cubre los ejes SAT cruzados.
7. Contacto inicial tangencial: distancia cero, sin clasificar como penetración.
8. Penetración inicial: `AlreadyPenetrating` y ninguna propuesta.
9. Contacto más allá de `maximumMovement`: `NoContactWithinLimit`.
10. `targetGap > firstContact`: `ClearanceExceedsGap`.
11. `targetGap > 0`: se detiene esa distancia antes del primer contacto.
12. Dirección nula, triángulos degenerados y malla vacía: error controlado.
13. Mallas grandes sintéticas: verificar que el número de narrow-phase pairs es muy inferior a `N*M` (exponer contador solo en diagnóstico de test si hace falta).

Pruebas de integración/persistencia:

- Resolver dirección desde una `VelocityBC` de pieza y aplicar su máscara DOF.
- Rechazar BC automáticas ambiguas/no colineales.
- `Preview` no cambia coordenadas ni ensucia el modelo.
- `Apply`, undo y redo mantienen nodos, visual y metadato coherentes.
- Guardar/cargar conserva `initialPositioning` sin volver a trasladar nodos.
- El BDF guardado contiene las coordenadas trasladadas.
- El JSON de ejecución conserva `start: [0,0,0]`, evitando doble traslación.
- Cargar un `.wfmodel` antiguo sin `initialPositioning` sigue funcionando.

## Entregas sugeridas para Luna

### PR 1 — Núcleo geométrico

- Crear la utilidad independiente, triangulación, BVH/AABB y SAT continuo.
- Agregar CTest y todos los casos geométricos.
- Sin cambios de UI ni persistencia.

Criterio de salida: resultados deterministas, sin dependencia de renderer, sin `O(N*M)` en el caso grande y sanitizers limpios si están disponibles.

### PR 2 — Datos y round trip

- Agregar `InitialPositioning` a `Part`.
- Leer/escribir el bloque opcional en `.wfmodel`.
- Probar que el BDF incorpora la posición y `InputWriter` mantiene `start = 0`.

Criterio de salida: round trip exacto y compatibilidad hacia atrás.

### PR 3 — Preview y Apply

- Agregar diálogo, resolución automática/manual de dirección y selector de objetivo.
- Implementar overlay de preview y resaltado de caras.
- Aplicar mediante `Editor::applyPartTranslation`, con revalidación y undo/redo.
- Añadir pruebas de integración y validaciones de UX.

Criterio de salida: el martillo puede previsualizarse y aplicarse sin mutación durante preview, sin penetración y con estado reproducible después de guardar/cargar.

### PR 4 — Diagnóstico del solver (repositorio del solver)

El núcleo del solver no está en este repositorio. Implementar allí, como entrega separada:

```ini
[InitialContactCheck]
minimum_gap=...
active_points=...
penetrated_points=...
maximum_penetration=...
policy=warn|error_on_penetration
```

WeldFormGUI solo deberá exponer/persistir la política cuando el solver soporte formalmente esas claves. No agregar `autoSnap=true`.

## Criterios de aceptación finales

- Con el martillo rígido seleccionado, el usuario puede elegir la pieza deformable, tomar la dirección de su BC de velocidad o ingresarla manualmente, y obtener el primer contacto direccional 3D dentro del límite.
- La magnitud de la BC no cambia el resultado.
- El preview informa gap, vector, gap final y caras candidatas, y resalta la región sin cambiar el modelo.
- Apply produce exactamente una traslación rígida, no introduce penetración, puede deshacerse/rehacerse y marca el modelo modificado.
- `.wfmodel` conserva dirección, fuente, clearance, objetivo y traslación aplicada; al recargar no se aplica dos veces.
- El BDF contiene las coordenadas finales y el input del solver conserva origen rígido cero.
- Los cinco escenarios solicitados (planos, inclinado, dos zonas, límite y prevención de penetración), más arista-arista y compatibilidad de persistencia, están automatizados.

## Validación funcional Biassone

Como verificación manual final, guardar dos variantes del caso Biassone: original y reposicionada con `targetGap = 0`. Ejecutar ambas con la misma velocidad constante baja y sin rampa, manteniendo iguales malla, contacto, material y paso temporal. Archivar para cada una el `.wfmodel`, BDF exportado, log inicial y curvas de fuerza/energía. La comparación debe aislar el efecto del gap inicial; no mezclar en esta prueba cambios de Picard ni de penalización.
