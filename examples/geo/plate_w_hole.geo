SetFactory("OpenCASCADE");

// -------------------------
// Geometry parameters [mm]
// -------------------------
A  = 40.0;   // square side
R  = 5.0;
lc = 2.0;

// Mesh divisions
nRadial = 12;
nCirc   = 24;
nOuter  = 24;

h = A/2;
r45 = R/Sqrt(2);

// -------------------------
// Outer square points
// -------------------------
Point(1) = {-h, -h, 0, lc};
Point(2) = { h, -h, 0, lc};
Point(3) = { h,  h, 0, lc};
Point(4) = {-h,  h, 0, lc};

Point(10) = {0, 0, 0, lc};

// Hole points at 45 degrees
Point(11) = { r45,  r45, 0, lc};
Point(12) = {-r45,  r45, 0, lc};
Point(13) = {-r45, -r45, 0, lc};
Point(14) = { r45, -r45, 0, lc};

// -------------------------
// Outer boundary
// -------------------------
Line(1) = {1, 2}; // bottom
Line(2) = {2, 3}; // right
Line(3) = {3, 4}; // top
Line(4) = {4, 1}; // left

// Hole arcs
Circle(11) = {11, 10, 12};
Circle(12) = {12, 10, 13};
Circle(13) = {13, 10, 14};
Circle(14) = {14, 10, 11};

// Diagonal partition lines to square vertices
Line(21) = {11, 3};
Line(22) = {12, 4};
Line(23) = {13, 1};
Line(24) = {14, 2};

// -------------------------
// Four 4-sided subdomains
// -------------------------

// Top
Curve Loop(101) = {3, -22, -11, 21};
Plane Surface(101) = {101};

// Left
Curve Loop(102) = {4, -23, -12, 22};
Plane Surface(102) = {102};

// Bottom
Curve Loop(103) = {1, -24, -13, 23};
Plane Surface(103) = {103};

// Right
Curve Loop(104) = {2, -21, -14, 24};
Plane Surface(104) = {104};

// -------------------------
// Transfinite mesh
// -------------------------
Transfinite Curve {21,22,23,24} = nRadial + 1;
Transfinite Curve {11,12,13,14} = nCirc/4 + 1;
Transfinite Curve {1,2,3,4} = nOuter/4 + 1;

Transfinite Surface {101,102,103,104};
Recombine Surface {101,102,103,104};

Mesh.Smoothing = 20;

// -------------------------
// Physical groups
// -------------------------
Physical Surface("PLATE") = {101,102,103,104};

Physical Curve("LEFT")   = {4};
Physical Curve("RIGHT")  = {2};
Physical Curve("TOP")    = {3};
Physical Curve("BOTTOM") = {1};
Physical Curve("HOLE")   = {11,12,13,14};