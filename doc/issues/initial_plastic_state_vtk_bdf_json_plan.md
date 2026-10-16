# Plan GUI: preparar una operación desde VTK y exportar BDF + JSON

Estado: propuesta documental; no implementado. Fecha: 2026-10-16.

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

Solo plan: no implementar durante esta tarea. Para implementación futura, revisar instrucciones locales y respetar `prompt.txt` del Engine en cambios compartidos: sin build CMake y con DEVLOG por cambio de código. No modificar librerías de remallado 2D sin autorización.

Diferir temperatura y sus unidades/precedencias, campos nodales, presión, velocidades, tensiones, formatos binarios y transferencia entre mallas distintas. No agregar infraestructura genérica de variables internas. Antes de comenzar tareas acotadas, fijar propiedad del estado en el modelo y contrato de exportación; separar implementación y revisión conforme a la política de agentes aplicable.
