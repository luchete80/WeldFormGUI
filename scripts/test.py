from model import *;
msh = Mesh();
msh.addPlane(0.0,0.0,1,1,0.1);
m = Model();
getApp().setActiveModel(m);
m.addPart(Part(msh));
getApp().getActiveModel().addPart(Part(msh));
print(msh.getNodeCount())
print(m.getPart(0).getMesh().getNodeCount())


######################################

from model import *;
msh = Mesh();
msh.addPlane(0.0,0.0,1,1,0.1);
#getApp().getActiveModel().addPart(Part(msh));

p = Part(msh);
getApp().getActiveModel().addPart(p);
print(getApp().getActiveModel().getPartCount());
getApp().Update();





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
getApp().Update();



#########################################
#  def __init__(self, id, radius, ox,oy,oz, divisions):

from Sphere import *;
msh=Mesh();
s = sphere(msh,1,1.0,0.0,0.0,0.0,5);
p = Part(s);
getApp().getActiveModel().addPart(p);
getApp().Update();

########################################

from Sphere import *;
msh = Mesh();
sphere_segment(msh,1,1.0,0.0,0.0,0.0,5);
p = Part(msh);
getApp().getActiveModel().addPart(p);
getApp().Update();


from Sphere import *;
msh = Mesh();
sphere_segment(msh,1,1.0,0.0,0.0,0.0,5);
print(msh.getNode(0).getId());
print(msh.getNode(0).getPos().x)

#############################################
from Radioss import *;
msh = Mesh();
msh.addNode(0,0); 
msh.addNode(1,0); 
msh.addNode(1,1); 
msh.addNode(0,1); 
print("Node count: ", msh.getNodeCount());
msh.addQuad(0,1,2,3);
p = Part(msh);
getApp().getActiveModel().addPart(p);
getApp().Update();
exp = RadiossExporter(getApp().getActiveModel());
exp.printModel("test.rad");


print ("element count ", p.getMesh().getElemCount());
print (p.getId());

from importlib import reload;
importlib.reload()
