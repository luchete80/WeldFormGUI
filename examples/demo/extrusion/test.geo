SetFactory("OpenCASCADE");

Merge "test.step";

// Escalar TODO (no solo curvas)
Dilate {{0,0,0}, {0.001,0.001,0.001}} {
  Curve{:};
  Surface{:};
}

// Opcional pero útil
Coherence;
