// Gmsh - Copyright (C) 1997-2024 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file in the Gmsh root directory for license information.
// Please report all issues on https://gitlab.onelab.info/gmsh/gmsh/issues.

#ifndef GMSH_CONFIG_H
#define GMSH_CONFIG_H

/* #undef HAVE_3M */
#define HAVE_64BIT_SIZE_T
/* #undef HAVE_ACIS */
/* #undef HAVE_ALGLIB */
/* #undef HAVE_ANN */
/* #undef HAVE_BAMG */
/* #undef HAVE_BLAS */
/* #undef HAVE_BLOSSOM */
/* #undef HAVE_CAIRO */
#define HAVE_DLOPEN
/* #undef HAVE_DINTEGRATION */
/* #undef HAVE_DOMHEX */
/* #undef HAVE_EIGEN */
/* #undef HAVE_FLTK */
/* #undef HAVE_GEOMETRYCENTRAL */
/* #undef HAVE_GMM */
/* #undef HAVE_GMP */
/* #undef HAVE_HXT */
/* #undef HAVE_KBIPACK */
/* #undef HAVE_LAPACK */
/* #undef HAVE_LIBCGNS */
/* #undef HAVE_LIBCGNS_CPEX0045 */
/* #undef HAVE_LIBJPEG */
/* #undef HAVE_LIBPNG */
/* #undef HAVE_LIBZ */
#define HAVE_LINUX_JOYSTICK
/* #undef HAVE_MATHEX */
/* #undef HAVE_MED */
#define HAVE_MESH
/* #undef HAVE_MESQUITE */
/* #undef HAVE_METIS */
/* #undef HAVE_MMG */
/* #undef HAVE_MPEG_ENCODE */
/* #undef HAVE_MPI */
/* #undef HAVE_MUMPS */
/* #undef HAVE_NETGEN */
/* #undef HAVE_NII2MESH */
/* #undef HAVE_NUMPY */
/* #undef HAVE_NO_INTPTR_T */
/* #undef HAVE_NO_SOCKLEN_T */
/* #undef HAVE_NO_STDINT_H */
/* #undef HAVE_NO_VSNPRINTF */
#define HAVE_OCC
#define HAVE_OCC_CAF
#define HAVE_ONELAB
/* #undef HAVE_ONELAB2 */
/* #undef HAVE_ONELAB_METAMODEL */
/* #undef HAVE_UDT */
/* #undef HAVE_OPENGL */
/* #undef HAVE_OPTHOM */
/* #undef HAVE_OSMESA */
/* #undef HAVE_P4EST */
/* #undef HAVE_PARASOLID */
/* #undef HAVE_PARASOLID_STEP */
#define HAVE_PARSER
/* #undef HAVE_PETSC */
/* #undef HAVE_PETSC4PY */
/* #undef HAVE_PLUGINS */
/* #undef HAVE_POST */
/* #undef HAVE_POPPLER */
/* #undef HAVE_QUADTRI */
/* #undef HAVE_QUADMESHINGTOOLS */
/* #undef HAVE_REVOROPT */
/* #undef HAVE_SALOME */
/* #undef HAVE_SGEOM */
/* #undef HAVE_SLEPC */
#define HAVE_SOLVER
/* #undef HAVE_UNTANGLE */
/* #undef HAVE_TAUCS */
#define HAVE_TETGENBR
/* #undef HAVE_TINYXML2 */
/* #undef HAVE_TOUCHBAR */
/* #undef HAVE_VISUDEV */
/* #undef HAVE_VOROPP */
/* #undef HAVE_WINSLOWUNTANGLER */
/* #undef HAVE_ZIPPER */

#define GMSH_CONFIG_OPTIONS " 64Bit Dlopen LinuxJoystick Mesh ONELAB OpenCASCADE OpenCASCADE-CAF OpenMP Parser Solver TetGen/BR"



#endif
