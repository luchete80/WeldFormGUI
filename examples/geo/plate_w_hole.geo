SetFactory("OpenCASCADE");

// -------------------------
// Geometry parameters [mm]
// -------------------------
L  = 100.0;
W  = 40.0;
R  = 5.0;
lc = 2.0;

// Mesh divisions
nRadial = 12;
nCirc   = 24;
nOuter  = 24;

// -------------------------
// Points
// -------------------------
Point(1) = {-L/2, -W/2, 0, lc};
Point(2) = { L/2, -W/2, 0, lc};
Point(3) = { L/2,  W/2, 0, lc};
Point(4) = {-L/2,  W/2, 0, lc};

Point(10) = {0, 0, 0, lc};

// Hole quadrant points
Point(11) = { R, 0, 0, lc};
Point(12) = { 0, R, 0, lc};
Point(13) = {-R, 0, 0, lc};
Point(14) = { 0,-R, 0, lc};

// Outer mid-side points
Point(21) = { L/2, 0, 0, lc};
Point(22) = { 0, W/2, 0, lc};
Point(23) = {-L/2, 0, 0, lc};
Point(24) = { 0,-W/2, 0, lc};

// -------------------------
// Outer boundary split
// -------------------------
Line(1) = {24, 2};
Line(2) = {2, 21};
Line(3) = {21, 3};
Line(4) = {3, 22};
Line(5) = {22, 4};
Line(6) = {4, 23};
Line(7) = {23, 1};
Line(8) = {1, 24};

// Hole arcs
Circle(11) = {11, 10, 12};
Circle(12) = {12, 10, 13};
Circle(13) = {13, 10, 14};
Circle(14) = {14, 10, 11};

// Diagonal/radial partition lines
Line(21) = {11, 21};
Line(22) = {12, 22};
Line(23) = {13, 23};
Line(24) = {14, 24};

// -------------------------
// Four subdomains
// -------------------------

// Right-top
Curve Loop(101) = {21, 3, 4, -22, -11};
Plane Surface(101) = {101};

// Left-top
Curve Loop(102) = {22, 5, 6, -23, -12};
Plane Surface(102) = {102};

// Left-bottom
Curve Loop(103) = {23, 7, 8, -24, -13};
Plane Surface(103) = {103};

// Right-bottom
Curve Loop(104) = {24, 1, 2, -21, -14};
Plane Surface(104) = {104};

// -------------------------
// Transfinite mesh
// -------------------------
Transfinite Curve {21,22,23,24} = nRadial + 1;

Transfinite Curve {11,12,13,14} = nCirc/4 + 1;

Transfinite Curve {3,4,5,6,7,8,1,2} = nOuter/2 + 1;

Transfinite Surface {101,102,103,104};
Recombine Surface {101,102,103,104};

// Optional smoothing
Mesh.Smoothing = 20;

// -------------------------
// Physical groups
// -------------------------
Physical Surface("PLATE") = {101,102,103,104};

Physical Curve("LEFT")   = {6,7};
Physical Curve("RIGHT")  = {2,3};
Physical Curve("TOP")    = {4,5};
Physical Curve("BOTTOM") = {8,1};
Physical Curve("HOLE")   = {11,12,13,14};