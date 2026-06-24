SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
depth = 6.5 * mm;     // profundidad del arco
yc    = 1.0 * mm;     // centro del círculo
L     = 8.0 * mm;     // largo de la línea

lc = 1.0 * mm;

// Chaflán 45°
ch = 1.0 * mm;        // cateto horizontal y vertical

// Radio circular
R = yc + depth;

// Punto original donde el arco llega a y=0
xEnd = Sqrt(R*R - yc*yc);

// Punto donde termina el arco (y = -ch)
yChamArc = -ch;
xChamArc = Sqrt(R*R - (yChamArc - yc)*(yChamArc - yc));

// Fin del chaflán e inicio de la línea
xChamLine = xChamArc + ch;
yChamLine = 0.0;

// Fin de la línea horizontal
xLineEnd = xEnd + L;

// Points
Point(1) = {0, -depth, 0, lc};          // punto más bajo del arco
Point(2) = {0, yc,     0, lc};          // centro del círculo
Point(3) = {xChamArc, yChamArc, 0, lc}; // fin del arco
Point(4) = {xChamLine, 0,       0, lc}; // fin del chaflán
Point(5) = {xLineEnd,  0,       0, lc}; // fin de la línea

// Geometry
Circle(1) = {1, 2, 3};
Line(2) = {3, 4};   // chaflán 45°
Line(3) = {4, 5};   // línea horizontal

// Physical groups
Physical Curve("arc") = {1};
Physical Curve("chamfer") = {2};
Physical Curve("line") = {3};
Physical Curve("tool") = {1, 2, 3};