from model import *
msh = Mesh()
msh.addPlane(0.0,0.0,1,1,0.1)
m = Model()
getApp().setActiveModel(m)
m.addPart(Part(msh))
print(msh.getNodeCount())

