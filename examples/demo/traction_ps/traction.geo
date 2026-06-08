SetFactory("OpenCASCADE");

// -------------------------
// Units
// -------------------------
mm = 1.0e-3;

// -------------------------
// Dimensions
// -------------------------
L_grip  = 25.0 * mm;
L_trans = 15.0 * mm;
L_gauge = 50.0 * mm;

W_grip  = 20.0 * mm;
W_gauge = 12.5 * mm;

lc = 2.0 * mm;

// Mesh divisions
nx_grip  = 10;
nx_trans = 8;
nx_gauge = 24;
ny       = 8;

// -------------------------
// Derived coordinates
// -------------------------
x0 = -(L_gauge/2 + L_trans + L_grip);
x1 = -(L_gauge/2 + L_trans);
x2 = - L_gauge/2;
x3 =   L_gauge/2;
x4 =   L_gauge/2 + L_trans;
x5 =   L_gauge/2 + L_trans + L_grip;

yg = W_gauge/2;
yb = W_grip/2;

// -------------------------
// Points
// -------------------------
// bottom row
Point(1)  = {x0, -yb, 0, lc};
Point(2)  = {x1, -yb, 0, lc};
Point(3)  = {x2, -yg, 0, lc};
Point(4)  = {x3, -yg, 0, lc};
Point(5)  = {x4, -yb, 0, lc};
Point(6)  = {x5, -yb, 0, lc};

// top row
Point(7)  = {x0,  yb, 0, lc};
Point(8)  = {x1,  yb, 0, lc};
Point(9)  = {x2,  yg, 0, lc};
Point(10) = {x3,  yg, 0, lc};
Point(11) = {x4,  yb, 0, lc};
Point(12) = {x5,  yb, 0, lc};

// -------------------------
// Curves
// -------------------------
// bottom
Line(1) = {1,2};
Line(2) = {2,3};
Line(3) = {3,4};
Line(4) = {4,5};
Line(5) = {5,6};

// top
Line(6)  = {7,8};
Line(7)  = {8,9};
Line(8)  = {9,10};
Line(9)  = {10,11};
Line(10) = {11,12};

// vertical separators
Line(11) = {1,7};
Line(12) = {2,8};
Line(13) = {3,9};
Line(14) = {4,10};
Line(15) = {5,11};
Line(16) = {6,12};

// -------------------------
// Surfaces
// -------------------------
// left grip
Curve Loop(1) = {1,12,-6,-11};
Plane Surface(1) = {1};

// left transition
Curve Loop(2) = {2,13,-7,-12};
Plane Surface(2) = {2};

// gauge section
Curve Loop(3) = {3,14,-8,-13};
Plane Surface(3) = {3};

// right transition
Curve Loop(4) = {4,15,-9,-14};
Plane Surface(4) = {4};

// right grip
Curve Loop(5) = {5,16,-10,-15};
Plane Surface(5) = {5};

// -------------------------
// Transfinite mesh
// -------------------------
Transfinite Curve {1,6} = nx_grip + 1;
Transfinite Curve {2,7} = nx_trans + 1;
Transfinite Curve {3,8} = nx_gauge + 1;
Transfinite Curve {4,9} = nx_trans + 1;
Transfinite Curve {5,10} = nx_grip + 1;

Transfinite Curve {11,12,13,14,15,16} = ny + 1;

Transfinite Surface {1};
Transfinite Surface {2};
Transfinite Surface {3};
Transfinite Surface {4};
Transfinite Surface {5};

Recombine Surface {1,2,3,4,5};

// -------------------------
// Physical groups
// -------------------------
Physical Surface("Specimen") = {1,2,3,4,5};

Physical Curve("LeftGrip")  = {11};
Physical Curve("RightGrip") = {16};

Physical Curve("Bottom") = {1,2,3,4,5};
Physical Curve("Top")    = {6,7,8,9,10};

Physical Curve("GaugeBottom") = {3};
Physical Curve("GaugeTop")    = {8};
