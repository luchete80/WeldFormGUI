SetFactory("OpenCASCADE");

// -------------------------
// Parámetros principales
// -------------------------
L_base = 93;
H_total = 80;

mm = 0.001;

R1 = 1.5*mm;
R2 = 2.0*mm;
R5 = 5.0*mm;

a2 = 2 * Pi / 180;   // 2 grados en radianes
a2_4 = 2.4 * Pi / 180;   // 2 grados en radianes
a3 = 3.0 * Pi / 180;   // 2 grados en radianes

// Ajustes intermedios (según plano)
x1 = 15.57*mm;

y1= 33.8*mm;
y2 = 9.6*mm;
y3 = 9.8*mm;


x2 = x1+R2+y1*Tan(a2);
x3 = x2+R5+10.44*mm;
x4 = x3+R2+y2*Tan(a2_4);
x5 = x4+R2+22.41*mm;
x6 = x5+R2+y3*Tan(a3);
x7 = 87.9*mm;

// -------------------------
// Escalonado izquierdo
// -------------------------
Point(1) = {0, 0, 0};
Point(2) = {x1, 0, 0};
Point(3) = {x1, R2, 0};

Point(4) = {x1+R2, R2, 0};
Point(5) = {x2, y1-R5, 0};
Point(6) = {x2+R5, y1-R5, 0};
Point(7) = {x2+R5, y1, 0};
Point(8) = {x3, y1, 0};
Point(9) = {x3, y1+R2, 0};
Point(10) = {x3+R2, y1+R2, 0};
Point(11) = {x4, y1+y2-R2, 0};
Point(12) = {x4+R2, y1+y2-R2, 0};
Point(13) = {x4+R2, y1+y2, 0};
Point(14) = {x5, y1+y2, 0};
Point(15) = {x5, y1+y2+R2, 0};
Point(16) = {x5+R2, y1+y2+R2, 0};
Point(17) = {x6, y1+y2+y3-R2, 0};
Point(18) = {x6+R2, y1+y2+y3-R2, 0};
Point(19) = {x6+R2, y1+y2+y3, 0};
Point(20) = {x7, y1+y2+y3, 0};


// -------------------------
// Líneas rectas
// -------------------------
Line(1) = {1,2};
Line(3) = {4,5};
Line(5) = {7,8};
Line(7) = {10,11};
Line(9) = {13,14};
Line(11) = {16,17};
Line(13) = {19,20};


// -------------------------
// Arcos
// -------------------------
Circle(2) = {2,3,4};   // R2
Circle(4) = {5,6,7};   // R2
Circle(6) = {8,9,10};   // R2
Circle(8) = {11,12,13};   // R2
Circle(10) = {14,15,16};   // R2
Circle(12) = {17,18,19};   // R2


// -------------------------
// Loop y superficie
// -------------------------
//Curve Loop(100) = {1,2,3,4,5,10,6};
//Plane Surface(200) = {100};

// -------------------------
// Mallado
// -------------------------
Mesh.CharacteristicLengthMin = 0.5*mm;
Mesh.CharacteristicLengthMax = 1.0*mm;

