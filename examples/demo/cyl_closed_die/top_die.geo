SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
depth = 6.5 * mm;     // altura/profundidad del arco hacia arriba
yc    = 1.0 * mm;     // centro del círculo, medido hacia abajo desde y=0
L     = 8.0 * mm;     // largo de la línea

lc = 1.0 * mm;

// Radio circular
R = yc + depth;

// Coordenada x del punto donde el arco llega a y=0
xEnd = Sqrt(R*R - yc*yc);

// Points
Point(1) = {0,  depth, 0, lc};        // punto más alto del arco
Point(2) = {0, -yc,    0, lc};        // centro del círculo
Point(3) = {xEnd, 0,   0, lc};        // salida del arco en y=0
Point(4) = {xEnd + L, 0, 0, lc};      // fin de la línea

// Geometry
Circle(1) = {1, 2, 3};
Line(2) = {3, 4};

// Physical groups
Physical Curve("arc") = {1};
Physical Curve("line") = {2};
Physical Curve("tool") = {1, 2};
