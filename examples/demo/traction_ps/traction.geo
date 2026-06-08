SetFactory("OpenCASCADE");

mm = 1.0e-3;

// -------------------------
// Dimensions
// -------------------------
L_grip  = 25.0 * mm;
L_trans = 15.0 * mm;
L_gauge = 50.0 * mm;

W_grip  = 20.0 * mm;
W_gauge = 12.5 * mm;

// Target element size
h = 2.0 * mm;

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

// Automatic divisions
nx_grip  = Ceil(L_grip / h);
nx_gauge = Ceil(L_gauge / h);

// transition diagonal length
Ldiag = Sqrt(L_trans^2 + (yb - yg)^2);
nx_trans = Ceil(Ldiag / h);

ny_grip  = Ceil(W_grip / h);
ny_gauge = Ceil(W_gauge / h);

// important: use same ny through all vertical interfaces
ny = Max(ny_grip, ny_gauge);

// -------------------------
// Points
// -------------------------
Point(1)  = {x0, -yb, 0, h};
Point(2)  = {x1, -yb, 0, h};
Point(3)  = {x2, -yg, 0, h};
Point(4)  = {x3, -yg, 0, h};
Point(5)  = {x4, -yb, 0, h};
Point(6)  = {x5, -yb, 0, h};

Point(7)  = {x0,  yb, 0, h};
Point(8)  = {x1,  yb, 0, h};
Point(9)  = {x2,  yg, 0, h};
Point(10) = {x3,  yg, 0, h};
Point(11) = {x4,  yb, 0, h};
Point(12) = {x5,  yb, 0, h};

// -------------------------
// Curves
// -------------------------
Line(1) = {1,2};
Line(2) = {2,3};
Line(3) = {3,4};
Line(4) = {4,5};
Line(5) = {5,6};

Line(6)  = {7,8};
Line(7)  = {8,9};
Line(8)  = {9,10};
Line(9)  = {10,11};
Line(10) = {11,12};

Line(11) = {1,7};
Line(12) = {2,8};
Line(13) = {3,9};
Line(14) = {4,10};
Line(15) = {5,11};
Line(16) = {6,12};

// -------------------------
// Surfaces
// -------------------------
Curve Loop(1) = {1,12,-6,-11};
Plane Surface(1) = {1};

Curve Loop(2) = {2,13,-7,-12};
Plane Surface(2) = {2};

Curve Loop(3) = {3,14,-8,-13};
Plane Surface(3) = {3};

Curve Loop(4) = {4,15,-9,-14};
Plane Surface(4) = {4};

Curve Loop(5) = {5,16,-10,-15};
Plane Surface(5) = {5};

// -------------------------
// Transfinite curves
// -------------------------
Transfinite Curve {1,6}   = nx_grip + 1;
Transfinite Curve {2,7}   = nx_trans + 1;
Transfinite Curve {3,8}   = nx_gauge + 1;
Transfinite Curve {4,9}   = nx_trans + 1;
Transfinite Curve {5,10}  = nx_grip + 1;

Transfinite Curve {11,12,13,14,15,16} = ny + 1;

// Explicit corners: very important
Transfinite Surface {1} = {1,2,8,7};
Transfinite Surface {2} = {2,3,9,8};
Transfinite Surface {3} = {3,4,10,9};
Transfinite Surface {4} = {4,5,11,10};
Transfinite Surface {5} = {5,6,12,11};

Recombine Surface {1,2,3,4,5};

Mesh.RecombineAll = 1;
Mesh.Algorithm = 8;

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
