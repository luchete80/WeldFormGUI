*KEYWORD
*TITLE
LS-DYNA Export from Mesh Class
Created by Mesh::exportToLSDYNA()
$# END OF HEADER
*NODE
$#   nid               x               y               z      tc      rc
       1 0.0000000000000 0.0000000000000 0.0000000000000     0     0
       2 0.0399999991059 0.0000000000000 0.0000000000000     0     0
       3 0.0044444445521 0.0000000000000 0.0000000000000     0     0
       4 0.0088888891041 0.0000000000000 0.0000000000000     0     0
       5 0.0133333336562 0.0000000000000 0.0000000000000     0     0
       6 0.0177777782083 0.0000000000000 0.0000000000000     0     0
       7 0.0222222227603 0.0000000000000 0.0000000000000     0     0
       8 0.0266666673124 0.0000000000000 0.0000000000000     0     0
       9 0.0311111118644 0.0000000000000 0.0000000000000     0     0
      10 0.0355555564165 0.0000000000000 0.0000000000000     0     0
$# END OF NODES
*ELEMENT_SOLID
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
$# END OF ELEMENTS
*PART
$#     pid       secid       mid     eosid      hgid      grav    adpopt      tmid
       1       1       1       0       0       0       0       0
$# END OF PART
*MAT_ELASTIC
$#     mid        ro         e        pr        da        db         k
       17.850000e+032.100000e+113.000000e-010.000000e+000.000000e+000.000000e+00
$# END OF MATERIAL
*BOUNDARY_SPC_SET
$#      id       cid      dofx      dofy      dofz     dofrx     dofry     dofrz
       1       0       1       1       1       0       0       0
$# END OF BOUNDARY CONDITIONS
*END
