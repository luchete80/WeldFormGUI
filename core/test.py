from fem_element_plot import plot_quad_element, plot_quad_deformed
import numpy as np

# --- Ejemplo 1: Elemento en coordenadas naturales ---
plot_quad_element()

# --- Ejemplo 2: Elemento físico deformado ---
node_coords = np.array([
    [0.0, 0.0],   # Nodo 1
    [2.0, 0.0],   # Nodo 2
    [2.0, 1.0],   # Nodo 3
    [0.0, 1.0]    # Nodo 4
])

# Supongamos desplazamientos (u_x, u_y)
displacements = np.array([
    [0.0, 0.0],
    [0.1, 0.0],
    [0.1, 0.05],
    [0.0, 0.05]
])

plot_quad_deformed(node_coords, displacements, scale=5.0)
