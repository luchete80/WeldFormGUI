SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
depth = 6.5 * mm;
yc    = 1.0 * mm;
L     = 8.0 * mm;
thickness = 0.1; // 0.1 m extrusion in the z direction

lc = 1.0 * mm;

// Chaflan a 45 grados
ch = 1.0 * mm;

// Radio circular
R = yc + depth;

// Punto donde termina el arco (y = -ch)
xEnd = Sqrt(R*R - yc*yc);
yChamArc = -ch;
xChamArc = Sqrt(R*R - (yChamArc - yc)*(yChamArc - yc));

// Fin del chaflan e inicio de la linea
xChamLine = xChamArc + ch;
xLineEnd = xEnd + L;

// Points
Point(1) = {0, -depth, 0, lc};
Point(2) = {0, yc, 0, lc};
Point(3) = {xChamArc, yChamArc, 0, lc};
Point(4) = {xChamLine, 0, 0, lc};
Point(5) = {xLineEnd, 0, 0, lc};

// Geometry
Circle(1) = {1, 2, 3};
Line(2) = {3, 4};
Line(3) = {4, 5};

// Extrude the complete die profile to a 0.1 m-deep surface.
arc[] = Extrude {0, 0, thickness} { Curve{1}; Layers{1}; Recombine; };
chamfer[] = Extrude {0, 0, thickness} { Curve{2}; Layers{1}; Recombine; };
line[] = Extrude {0, 0, thickness} { Curve{3}; Layers{1}; Recombine; };

Physical Surface("arc") = {arc[1]};
Physical Surface("chamfer") = {chamfer[1]};
Physical Surface("line") = {line[1]};
Physical Surface("tool") = {arc[1], chamfer[1], line[1]};
