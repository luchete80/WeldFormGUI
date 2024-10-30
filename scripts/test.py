from model import *;
msh = Mesh();
msh.addPlane(0.0,0.0,1,1,0.1);
m = Model();
getApp().setActiveModel(m);
m.addPart(Part(msh));
getApp().getActiveModel().addPart(Part(msh));
print(msh.getNodeCount())
print(m.getPart(0).getMesh().getNodeCount())




from model import *;
msh = Mesh();
msh.addPlane(0.0,0.0,1,1,0.1);
#getApp().getActiveModel().addPart(Part(msh));

p = Part(msh);
getApp().getActiveModel().addPart(p);
print(getApp().getActiveModel().getPartCount());
getApp().Update();



getApp().getActiveModel().getPart(0).getMesh().

print(getApp().getActiveModel().getPart(0).getMesh().getNodeCount())
print(getApp().getActiveModel().getPartCount())
getApp().Update()
