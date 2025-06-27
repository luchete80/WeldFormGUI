
######################################
from model import *;
msh = Mesh();
msh.addNode(0,0); 
msh.addNode(1,0); 
msh.addNode(1,1); 
msh.addNode(0,1); 
print("Node count: ", msh.getNodeCount());
msh.addQuad(0,1,2,3);
p = Part(msh);
getApp().getActiveModel().addPart(p);
getApp().Update(); #NOT NECESARY

#from model import *;
msh2 = Mesh();
msh2.addNode(0,0,1); 
msh2.addNode(1,0,1); 
msh2.addNode(1,1,1); 
msh2.addNode(0,1,1); 
print("Node count: ", msh2.getNodeCount());
msh2.addQuad(0,1,2,3);
p2 = Part(msh2);
getApp().getActiveModel().addPart(p2);