from manim import *
import numpy as np
import sys
import os

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.append(PROJECT_ROOT)

from core.BaseFEMScene import BaseFEMScene


# fem/element.py
from .matrix import jacobian, grad_N_global, B_matrix
from .shape_functions import shape_functions, dN_dxi, map_to_physical   

class Element:
    def __init__(self, nodes):
        self.nodes = np.array(nodes)

    def compute_jacobian(self, xi):
        dN = dN_dxi(xi)
        return jacobian(dN, self.nodes)

    def compute_gradients(self, xi):
        dN = dN_dxi(xi)
        J, detJ, invJ = jacobian(dN, self.nodes)
        gradN = grad_N_global(dN, invJ)
        return gradN, detJ

    def B(self, xi):
        gradN, _ = self.compute_gradients(xi)
        return B_matrix(gradN)


def create_quad_shape(scale=1.0):
    """Devuelve los vértices del cuadrilátero natural (-1,1)x(-1,1)."""
    return np.array([
        [-1, -1],
        [ 1, -1],
        [ 1,  1],
        [-1,  1]
    ]) * scale

## OLD
def create_quad_with_grid(scale=1.0, ndiv=5, color=YELLOW):
    """Devuelve un VGroup con cuadrado, ejes, cuadrícula y nodos numerados."""
    group = VGroup()

    #--- cuadrado principal ---
    verts = create_quad_shape(scale)
    square = Polygon(*[np.array([x, y, 0]) for x, y in verts], color=color)
    group.add(square)

    #--- cuadrícula interna ---
    ξ = np.linspace(-1, 1, ndiv)
    η = np.linspace(-1, 1, ndiv)
    for x in ξ:
        group.add(Line([x*scale, -scale, 0], [x*scale, scale, 0], color=GRAY, stroke_width=1))
    for y in η:
        group.add(Line([-scale, y*scale, 0], [scale, y*scale, 0], color=GRAY, stroke_width=1))

    #--- ejes ---
    group.add(Line([-scale, 0, 0], [scale, 0, 0], color=WHITE))
    group.add(Line([0, -scale, 0], [0, scale, 0], color=WHITE))
    group.add(MathTex(r"\xi").next_to([scale, 0, 0], RIGHT*0.4))
    group.add(MathTex(r"\eta").next_to([0, scale, 0], UP*0.4))

    #--- nodos ---
    node_positions = [
        [-scale, -scale, 0],
        [ scale, -scale, 0],
        [ scale,  scale, 0],
        [-scale,  scale, 0],
    ]
    for i, pos in enumerate(node_positions, start=1):
        dot = Dot(pos, color=RED)
        label = Text(str(i), font_size=28, color=RED).next_to(pos, DOWN*0.2 if i < 3 else UP*0.2)
        group.add(dot, label)

    return group


#OLD, NO AXES
def create_quad_with_grid_nodes(nodes, color=WHITE, grid_color=GRAY, ndiv=6):
    """
    Crea un elemento cuadrilátero Q4 con grilla interna.
    nodes = [[x1,y1], [x2,y2], [x3,y3], [x4,y4]]
    """
    #nodes = np.array(nodes)
    nodes = np.array([[x, y, 0] for (x, y) in nodes])
    quad = Polygon(*nodes, color=color)

    group = VGroup(quad)

    # Malla de líneas — usamos espacio natural (ξ, η)
    xis = np.linspace(-1, 1, ndiv)
    etas = np.linspace(-1, 1, ndiv)

    # Líneas verticales (η variable, ξ fijo)
    for xi in xis:
        pts = [map_to_physical(nodes, xi, eta) for eta in etas]
        group.add(Line(pts[0], pts[-1], color=grid_color, stroke_width=1))

    
    # Líneas horizontales (ξ variable, η fijo)
    for eta in etas:
        pts = [map_to_physical(nodes, xi, eta) for xi in xis]
        group.add(Line(pts[0], pts[-1], color=grid_color, stroke_width=1))

    return group

def create_reference_quad(scale=1.2, ndiv=6, color=WHITE):
    """
    Cuadrado de coordenadas naturales (ξ, η)
    con grilla, ejes y etiquetas.
    """
    group = VGroup()

    # vértices
    verts = [
        (-scale, -scale, 0),
        ( scale, -scale, 0),
        ( scale,  scale, 0),
        (-scale,  scale, 0),
    ]
    square = Polygon(*verts, color=color, stroke_width=3)
    group.add(square)

    # grilla
    xi_vals = np.linspace(-1, 1, ndiv)
    eta_vals = np.linspace(-1, 1, ndiv)

    for ξ in xi_vals:
        x = ξ * scale
        group.add(Line([x, -scale, 0], [x, scale, 0], color=GRAY, stroke_width=1))

    for η in eta_vals:
        y = η * scale
        group.add(Line([-scale, y, 0], [scale, y, 0], color=GRAY, stroke_width=1))

    # ejes
    group.add(Line([-scale, 0, 0], [scale, 0, 0], color=WHITE))
    group.add(Line([0, -scale, 0], [0, scale, 0], color=WHITE))

    group.add(MathTex(r"\xi").next_to([scale, 0, 0], RIGHT, buff=0.25))
    group.add(MathTex(r"\eta").next_to([0, scale, 0], UP, buff=0.25))

    return group
    
    

def create_physical_quad(nodes, color=WHITE, grid_color=GRAY, ndiv=6):
    """
    Crea un quad físico deformado con grilla mapeada.
    nodes=[[x1,y1],[x2,y2],[x3,y3],[x4,y4]]
    """
    nodes_xyz = np.array([[x, y, 0] for (x, y) in nodes])
    quad = Polygon(*nodes_xyz, color=color, stroke_width=3)
    group = VGroup(quad)

    xis = np.linspace(-1, 1, ndiv)
    etas = np.linspace(-1, 1, ndiv)

    # verticales
    for ξ in xis:
        pts = [map_to_physical(nodes_xyz, ξ, η) for η in etas]
        group.add(Line(pts[0], pts[-1], color=grid_color, stroke_width=1))

    # horizontales
    for η in etas:
        pts = [map_to_physical(nodes_xyz, ξ, η) for ξ in xis]
        group.add(Line(pts[0], pts[-1], color=grid_color, stroke_width=1))

    return group
        
def create_deformed_quad(vertices, draw_grid=True, color=ORANGE, grid_color=GRAY):
    """
    Crea un cuadrilátero deformado a partir de coordenadas globales.
    
    Parameters:
    -----------
    vertices : list or np.array
        Array de 4 vértices en coordenadas globales [[x1,y1], [x2,y2], [x3,y3], [x4,y4]]
    draw_grid : bool
        Si True, dibuja la cuadrícula deformada
    color : Color
        Color del contorno del elemento
    grid_color : Color
        Color de la cuadrícula interna
        
    Returns:
    --------
    VGroup con el elemento deformado
    """
    group = VGroup()
    
    # --- Convertir vértices a formato 3D ---
    verts_3d = [np.array([x, y, 0]) for x, y in vertices]
    
    # --- Polígono principal ---
    deformed_poly = Polygon(*verts_3d, color=color, fill_opacity=0.2, stroke_width=3)
    group.add(deformed_poly)
    
    if draw_grid:
        # --- Crear cuadrícula deformada usando interpolación bilineal ---
        n_points = 5  # Número de puntos por dimensión
        
        # Coordenadas naturales de la cuadrícula
        xi_vals = np.linspace(-1, 1, n_points)
        eta_vals = np.linspace(-1, 1, n_points)
        
        # Funciones de forma para quadrilátero bilineal
        def shape_functions(xi, eta):
            N1 = 0.25 * (1 - xi) * (1 - eta)
            N2 = 0.25 * (1 + xi) * (1 - eta) 
            N3 = 0.25 * (1 + xi) * (1 + eta)
            N4 = 0.25 * (1 - xi) * (1 + eta)
            return [N1, N2, N3, N4]
        
        # Generar puntos de la cuadrícula
        grid_points = []
        for xi in xi_vals:
            for eta in eta_vals:
                N = shape_functions(xi, eta)
                x = sum(N[i] * vertices[i][0] for i in range(4))
                y = sum(N[i] * vertices[i][1] for i in range(4))
                grid_points.append([x, y, 0])
        
        # Dibujar líneas horizontales (eta constante)
        for j in range(n_points):
            for i in range(n_points - 1):
                idx1 = j * n_points + i
                idx2 = j * n_points + i + 1
                line = Line(grid_points[idx1], grid_points[idx2], 
                           color=grid_color, stroke_width=1)
                group.add(line)
        
        # Dibujar líneas verticales (xi constante)
        for i in range(n_points):
            for j in range(n_points - 1):
                idx1 = j * n_points + i
                idx2 = (j + 1) * n_points + i
                line = Line(grid_points[idx1], grid_points[idx2], 
                           color=grid_color, stroke_width=1)
                group.add(line)
    
    # --- Nodos numerados ---
    node_colors = [RED, GREEN, BLUE, YELLOW]
    for i, vertex in enumerate(verts_3d):
        dot = Dot(vertex, color=node_colors[i], radius=0.06)
        label = Text(str(i+1), font_size=20, color=node_colors[i])
        
        # Posicionar label según el nodo
        if i == 0:  # Esquina inferior izquierda
            label.next_to(vertex, DOWN + LEFT, buff=0.1)
        elif i == 1:  # Esquina inferior derecha
            label.next_to(vertex, DOWN + RIGHT, buff=0.1)
        elif i == 2:  # Esquina superior derecha
            label.next_to(vertex, UP + RIGHT, buff=0.1)
        else:  # Esquina superior izquierda
            label.next_to(vertex, UP + LEFT, buff=0.1)
            
        group.add(dot, label)
    
    return group
        
def jacobian_at(xi, eta, vertices):
    # vertices: [[x1,y1],[x2,y2],[x3,y3],[x4,y4]]
    x1,y1 = vertices[0]; x2,y2 = vertices[1]; x3,y3 = vertices[2]; x4,y4 = vertices[3]
    dN1_dxi  = -0.25*(1-eta); dN1_deta = -0.25*(1-xi)
    dN2_dxi  =  0.25*(1-eta); dN2_deta = -0.25*(1+xi)
    dN3_dxi  =  0.25*(1+eta); dN3_deta =  0.25*(1+xi)
    dN4_dxi  = -0.25*(1+eta); dN4_deta =  0.25*(1-xi)
    dN_dxi = [dN1_dxi, dN2_dxi, dN3_dxi, dN4_dxi]
    dN_deta= [dN1_deta,dN2_deta,dN3_deta,dN4_deta]
    J = np.zeros((2,2))
    xs = [x1,x2,x3,x4]; ys = [y1,y2,y3,y4]
    for i in range(4):
        J[0,0] += dN_dxi[i]*xs[i]
        J[0,1] += dN_deta[i]*xs[i]
        J[1,0] += dN_dxi[i]*ys[i]
        J[1,1] += dN_deta[i]*ys[i]
    return J, float(np.linalg.det(J))
