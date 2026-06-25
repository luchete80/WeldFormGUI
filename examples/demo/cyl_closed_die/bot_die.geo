SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
depth = 6.5 * mm;
yc    = 1.0 * mm;
L     = 8.0 * mm;

lc = 1.0 * mm;

// Radio de empalme
rf = 1.0 * mm;

// Radio circular principal
R = yc + depth;

// Punto original donde el arco llega a y=0
xEnd = Sqrt(R*R - yc*yc);

// Punto donde termina el arco antes del radio
yFilletArc = -rf;
xFilletArc = Sqrt(R*R - (yFilletArc - yc)*(yFilletArc - yc));

// Punto donde empieza la línea horizontal
xFilletLine = xFilletArc + rf;
yFilletLine = 0.0;

// Centro del radio de empalme
xFilletCenter = xFilletLine;
yFilletCenter = -rf;

// Fin de la línea
xLineEnd = xEnd + L;

// Points
Point(1) = {0, -depth, 0, lc};                 // punto más bajo del arco
Point(2) = {0, yc,     0, lc};                 // centro del arco principal
Point(3) = {xFilletArc, yFilletArc, 0, lc};    // fin del arco principal
Point(4) = {xFilletCenter, yFilletCenter, 0, lc}; // centro del radio
Point(5) = {xFilletLine, 0, 0, lc};            // inicio de línea horizontal
Point(6) = {xLineEnd, 0, 0, lc};               // fin de línea

// Geometry
Circle(1) = {1, 2, 3};     // arco principal
Circle(2) = {3, 4, 5};     // radio de empalme
Line(3) = {5, 6};          // línea horizontal

// Physical groups
Physical Curve("arc") = {1};
Physical Curve("radius") = {2};
Physical Curve("line") = {3};
Physical Curve("tool") = {1, 2, 3};
