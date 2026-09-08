SetFactory("OpenCASCADE");

mm = 1.0e-3;

// Parameters
D = 16.0 * mm;
R = D/2;
thickness = 0.1; // 0.1 m extrusion in the z direction
lc = 1.0 * mm;

// Points
Point(1) = {0, -R, 0, lc};
Point(2) = {0, 0, 0, lc};
Point(3) = {0, R, 0, lc};
Point(4) = {R, 0, 0, lc};

// Right semicircle
Circle(1) = {1, 2, 4};
Circle(2) = {4, 2, 3};

// Diameter
Line(3) = {3, 1};

// Surface
Curve Loop(1) = {1, 2, 3};
Plane Surface(1) = {1};

// Extrude the workpiece to a 0.1 m-deep solid.
workpiece[] = Extrude {0, 0, thickness} {
  Surface{1};
  Layers{1};
  Recombine;
};

Physical Surface("arc") = {workpiece[2], workpiece[3]};
Physical Surface("diameter") = {workpiece[4]};
Physical Surface("end_faces") = {1, workpiece[0]};
Physical Volume("workpiece") = {workpiece[1]};
