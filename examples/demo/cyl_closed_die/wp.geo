SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
D = 16.0 * mm;
R = D/2;
lc = 1.0 * mm;

// Points
Point(1) = {0, -R, 0, lc};   // bottom
Point(2) = {0,  0, 0, lc};   // center
Point(3) = {0,  R, 0, lc};   // top
Point(4) = {R,  0, 0, lc};   // right

// Right semicircle
Circle(1) = {1, 2, 4};
Circle(2) = {4, 2, 3};

// Diameter
Line(3) = {3, 1};

// Surface
Curve Loop(1) = {1, 2, 3};
Plane Surface(1) = {1};

// Physical groups
Physical Curve("arc") = {1,2};
Physical Curve("diameter") = {3};
Physical Surface("semicircle_surface") = {1};
