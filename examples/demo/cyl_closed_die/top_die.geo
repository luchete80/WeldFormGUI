SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
depth = 6.5 * mm;
yc    = 1.0 * mm;
L     = 8.0 * mm;

lc = 0.5 * mm;

// Radio de empalme
rf = 1.0 * mm;

// Radio circular principal
R = yc + depth;

// Punto original donde el arco llegaria a y=0
xEnd = Sqrt(R*R - yc*yc);

// Punto donde termina el arco principal
yRadArc = rf;
xRadArc = Sqrt(R*R - (yRadArc + yc)*(yRadArc + yc));

// Punto donde empieza la linea horizontal
xRadLine = xRadArc + rf;
yRadLine = 0.0;

// Centro del radio
xRadCenter = xRadLine;
yRadCenter = rf;

// Fin linea
xLineEnd = xEnd + L;

// Points
Point(1) = {0, depth, 0, lc};          // punto mas alto del arco
Point(2) = {0, -yc, 0, lc};            // centro arco principal
Point(3) = {xRadArc, yRadArc, 0, lc};  // fin arco principal

Point(4) = {xRadLine, 0, 0, lc};       // inicio linea horizontal
Point(5) = {xLineEnd, 0, 0, lc};       // fin linea

Point(6) = {xRadCenter, yRadCenter, 0, lc}; // centro del radio

// Geometry
Circle(1) = {1, 2, 3};
Circle(2) = {3, 6, 4};   // radio
Line(3) = {4, 5};

// Physical groups
Physical Curve("arc") = {1};
Physical Curve("radius") = {2};
Physical Curve("line") = {3};
Physical Curve("tool") = {1, 2, 3};
