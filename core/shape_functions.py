# fem/shape_functions.py
import numpy as np

"""
Shape functions N(ξ) and their derivatives dN/dξ for common FEM elements.
Supported:
    - "quad4" : 4-node bilinear quadrilateral (2D)
    - "tri3"  : 3-node linear triangle (2D)
"""


# ------------------------------------------------------------------------------
# QUAD4  (bilinear)
# ------------------------------------------------------------------------------

def N_quad4(xi):
    """
    Shape functions for a 4-node bilinear quadrilateral.

    Parameters
    ----------
    xi : array_like of length 2
        Local coords (ξ, η)

    Returns
    -------
    N : (4,) array
    """
    ξ, η = xi
    return np.array([
        0.25 * (1 - ξ) * (1 - η),
        0.25 * (1 + ξ) * (1 - η),
        0.25 * (1 + ξ) * (1 + η),
        0.25 * (1 - ξ) * (1 + η)
    ])


def dN_dxi_quad4(xi):
    """
    Derivatives dN/dξ and dN/dη for QUAD4.

    Parameters
    ----------
    xi : array_like of length 2

    Returns
    -------
    dN_dxi : (4,2) array
        [ [dN1/dξ, dN1/dη],
          ...
          [dN4/dξ, dN4/dη] ]
    """
    ξ, η = xi

    return np.array([
        [-(1 - η), -(1 - ξ)],
        [ (1 - η), -(1 + ξ)],
        [ (1 + η),  (1 + ξ)],
        [-(1 + η),  (1 - ξ)],
    ]) * 0.25


# ------------------------------------------------------------------------------
# TRI3  (linear triangle)
# ------------------------------------------------------------------------------

def N_tri3(xi):
    """
    Shape functions for a 3-node linear triangle.

    Local coords (ξ, η) satisfy: ξ>=0, η>=0, ξ+η<=1.

    Returns
    -------
    N : (3,) array
    """
    ξ, η = xi
    N1 = 1 - ξ - η
    N2 = ξ
    N3 = η
    return np.array([N1, N2, N3])


def dN_dxi_tri3(xi):
    """
    Derivatives dN/dξ and dN/dη for TRI3.

    Returns
    -------
    dN_dxi : (3,2) array
    """
    return np.array([
        [-1.0, -1.0],
        [ 1.0,  0.0],
        [ 0.0,  1.0],
    ])


# ------------------------------------------------------------------------------
# Unified interface
# ------------------------------------------------------------------------------

def shape_functions(elem_type, xi):
    """
    Unified interface for retrieving N for any supported element type.
    """
    elem_type = elem_type.lower()

    if elem_type == "quad4":
        return N_quad4(xi)

    elif elem_type == "tri3":
        return N_tri3(xi)

    else:
        raise NotImplementedError(f"Shape functions for '{elem_type}' not implemented.")


def dN_dxi(elem_type, xi):
    """
    Unified interface for retrieving dN/dξ for any supported element type.
    """
    elem_type = elem_type.lower()

    if elem_type == "quad4":
        return dN_dxi_quad4(xi)

    elif elem_type == "tri3":
        return dN_dxi_tri3(xi)

    else:
        raise NotImplementedError(f"Derivatives dN/dξ for '{elem_type}' not implemented.")

def map_to_physical(nodes, xi, eta):
    """
    nodes = [[x1, y1], [x2, y2], [x3, y3], [x4, y4]]
    o también
    nodes = [[x1, y1, z], ...]  (z se ignora)
    """
    nodes = np.array(nodes)

    # Si vienen como (x, y, z), ignoramos z
    if nodes.shape[1] == 3:
        nodes = nodes[:, :2]

    x1, y1 = nodes[0]
    x2, y2 = nodes[1]
    x3, y3 = nodes[2]
    x4, y4 = nodes[3]

    # Shape functions Q4
    N1 = 0.25*(1 - xi)*(1 - eta)
    N2 = 0.25*(1 + xi)*(1 - eta)
    N3 = 0.25*(1 + xi)*(1 + eta)
    N4 = 0.25*(1 - xi)*(1 + eta)

    x = N1*x1 + N2*x2 + N3*x3 + N4*x4
    y = N1*y1 + N2*y2 + N3*y3 + N4*y4

    return np.array([x, y, 0.0])  # manim necesita 3D
