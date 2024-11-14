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


#class Sphere_Mesh(Mesh):
#  #NECESSARY TO CREATE SEPARATED NEW LISTS!
#  nodes = []
#  elnod = [] 
#  def __init__(self, id, radius, ox,oy,oz, divisions):
#    print ("Creating Sphere mesh")
#
#
def create_sphere(msh):
    self.id = id
    CubeToSphere_origins = [
    #Vector(-1.0, -1.0, -1.0), #ORGIINAL POINT ONE
    Vector(1.0, -1.0, -1.0),
    Vector(1.0, -1.0, -1.0),
    Vector(1.0, -1.0, 1.0),
    Vector(-1.0, -1.0, 1.0),
    Vector(-1.0, 1.0, -1.0),
    Vector(-1.0, -1.0, 1.0)]
    CubeToSphere_rights = [
    Vector(-2.0, 0.0, 0.0),
    Vector(0.0, 0.0, 2.0),
    Vector(-2.0, 0.0, 0.0),
    Vector(0.0, 0.0, -2.0),
    Vector(2.0, 0.0, 0.0),
    Vector(2.0, 0.0, 0.0)]
    CubeToSphere_ups = [
		Vector(0.0, 2.0, 0.0),
		Vector(0.0, 2.0, 0.0),
		Vector(0.0, 2.0, 0.0),
		Vector(0.0, 2.0, 0.0),
		Vector(0.0, 0.0, 2.0),
		Vector(0.0, 0.0, -2.0) ]
    step = 1.0 / divisions
    step3 = Vector(step, step, step)

    test = Vector (0.,0.,0.)
    n = 0
    for face in range (1): #CUBE FACES 
      origin = CubeToSphere_origins[face]
      right = CubeToSphere_rights[face]
      # print (right)
      up = CubeToSphere_ups[face]
      for j in range (divisions+1):
        j3 = Vector(j,j,j)
        for i in range (divisions+1):
          i3 = Vector(i,i,i)
          # print ("i3 j3 ", i3, j3)
          # print (right)
          # print ("origin ")
          # print (origin)
          # print ("right * origin ")

          # test = right * origin  
          # print (test)
          # test = right + up  
          # print (test)
          # const Vector3 p = origin + step3 * (i3 * right + j3 * up);
          p = origin + ( step3 * (i3 * right  + up *j3 )  )
          p2 = p * p
          # rx = sqrt(1.0 - 0.5 * (p2.y + p2.z) + p2.y*p2.z/3.0)
          # ry = sqrt(1.0 - 0.5 * (p2.z + p2.x) + p2.z*p2.x/3.0)
          # rz = sqrt(1.0 - 0.5 * (p2.x + p2.y) + p2.x*p2.y/3.0)
          rx = p.components[0] * sqrt(1.0 - 0.5 * (p2.components[1] + p2.components[2]) + p2.components[1]*p2.components[2]/3.0)
          ry = p.components[1] * sqrt(1.0 - 0.5 * (p2.components[2] + p2.components[0]) + p2.components[2]*p2.components[0]/3.0)
          rz = p.components[2] * sqrt(1.0 - 0.5 * (p2.components[0] + p2.components[1]) + p2.components[0]*p2.components[1]/3.0)
          
          x = rx * radius + ox;           y = ry * radius + oy ;           z = rz * radius + oz;
          print ("z , z corrected ", rz,z)
          #self.nodes.append((rx,ry,rz))
          self.nodes.append((x,y,z))
          
          n = n +1
    
    
    self.nodes.append((ox,oy,oz)) #CENTER AS RIGID PIVOT
    self.node_count = n + 1
    
    # print ("generated: %d", n , " nodes      ")
    # print ("Node vector count: ", len(self.nodes))
    
      # print (origin)
    
    # for i in range (self.node_count):
      # print ("SPHERE Node ", i, self.nodes[i])
    
    e = 0
    for ey in range (divisions):
      for ex in range (divisions):
        # elem%elnod(i,:)=[(nel(1)+1)*ey + ex+1,(nel(1)+1)*ey + ex+2,(nel(1)+1)*(ey+1)+ex+2,(nel(1)+1)*(ey+1)+ex+1]  
        self.elnod.append(((divisions+1)*ey+ex,(divisions+1)*ey + ex+1,(divisions+1)*(ey+1)+ex+1,(divisions+1)*(ey+1)+ex))      
        e = e + 1
    self.elem_count = e
