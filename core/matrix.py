# fem/matrix.py
import numpy as np

def jacobian(dN_dxi, node_coords):
    """
    Compute J = dX/dξ for an element.

    Parameters
    ----------
    dN_dxi : (n_nodes, dim) array
        Derivatives of shape functions wrt local coords.
    node_coords : (n_nodes, dim) array
        Global coordinates of nodes.

    Returns
    -------
    J : (dim, dim) array
        Jacobian matrix
    detJ : float
        Determinant of J
    invJ : (dim, dim) array
        Inverse of J
    """
    J = dN_dxi.T @ node_coords
    detJ = np.linalg.det(J)
    invJ = np.linalg.inv(J)
    return J, detJ, invJ


def grad_N_global(dN_dxi, invJ):
    """
    Computes ∇N = invJᵀ * dN_dξ

    Parameters
    ----------
    dN_dxi : (n_nodes, dim)
    invJ   : (dim, dim)

    Returns
    -------
    gradN : (n_nodes, dim)
    """
    return dN_dxi @ invJ.T


def B_matrix(gradN):
    """
    Standard small-strain B-matrix for 2D/3D.

    Parameters
    ----------
    gradN : (n_nodes, dim)

    Returns
    -------
    B : (strain_dim, n_nodes*dim)
    """
    n_nodes, dim = gradN.shape

    if dim == 2:
        # 3x(2*n)
        B = np.zeros((3, 2*n_nodes))
        for i in range(n_nodes):
            dNdx, dNdy = gradN[i]
            B[0, 2*i]   = dNdx
            B[1, 2*i+1] = dNdy
            B[2, 2*i]   = dNdy
            B[2, 2*i+1] = dNdx
        return B

    elif dim == 3:
        # 6x(3*n)
        B = np.zeros((6, 3*n_nodes))
        for i in range(n_nodes):
            dNdx, dNdy, dNdz = gradN[i]
            B[0, 3*i]   = dNdx
            B[1, 3*i+1] = dNdy
            B[2, 3*i+2] = dNdz
            B[3, 3*i]   = dNdy
            B[3, 3*i+1] = dNdx
            B[4, 3*i+1] = dNdz
            B[4, 3*i+2] = dNdy
            B[5, 3*i]   = dNdz
            B[5, 3*i+2] = dNdx
        return B

    else:
        raise ValueError("Only 2D or 3D supported.")

