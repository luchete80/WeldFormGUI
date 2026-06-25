SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
depth = 6.5 * mm;
yc    = 1.0 * mm;
L     = 8.0 * mm;

lc = 1.0 * mm;

// Chaflan a 45 grados
ch = 1.0 * mm;   // cateto vertical y horizontal del chaflan

// Radio circular
R = yc + depth;

// Punto original donde el arco llegaria a y=0
xEnd = Sqrt(R*R - yc*yc);

// Punto donde termina el arco, a una altura ch
yChamArc = ch;
xChamArc = Sqrt(R*R - (yChamArc + yc)*(yChamArc + yc));

// Punto donde empieza la linea horizontal
xChamLine = xChamArc + ch;
yChamLine = 0.0;

// Points
Point(1) = {0, depth, 0, lc};              // punto mas alto del arco
Point(2) = {0, -yc,   0, lc};              // centro del circulo
Point(3) = {xChamArc, yChamArc, 0, lc};    // fin del arco recortado
Point(4) = {xChamLine, yChamLine, 0, lc};  // fin del chaflan / inicio linea
Point(5) = {xEnd + L, 0, 0, lc};           // fin de la linea horizontal

// Geometry
Circle(1) = {1, 2, 3};
Line(2) = {3, 4};   // chaflan a 45 grados
Line(3) = {4, 5};   // linea horizontal

// Physical groups
Physical Curve("arc") = {1};
Physical Curve("chamfer") = {2};
Physical Curve("line") = {3};
Physical Curve("tool") = {1, 2, 3};