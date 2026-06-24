// C-shaped horizontal die profile
mm = 1.0e-3;

// Parameters
D = 20 * mm;     // arc diameter
L = 10 * mm;     // straight line length

R = D/2;
lc = 1.0 * mm;

// Points
Point(1) = {0, 0, 0, lc};
Point(2) = {L, 0, 0, lc};
Point(3) = {L + R, R, 0, lc};      // center of arc
Point(4) = {L + D, 0, 0, lc};
Point(5) = {L + D + L, 0, 0, lc};

// Auxiliary top point for the semicircle
Point(6) = {L + R, R, 0, lc};

// Geometry
Line(1) = {1, 2};
Circle(2) = {2, 3, 4};
Line(3) = {4, 5};

// Optional physical names
Physical Curve("lower_line") = {1};
Physical Curve("upper_semicircle") = {2};
Physical Curve("right_line") = {3};
Physical Curve("die_C_profile") = {1, 2, 3};
