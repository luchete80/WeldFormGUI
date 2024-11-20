from model import *


from math import *
#import numpy as np

debug = True

def writeFloatField(number, length, decimals):
  fmt ='%.' + str(decimals) + 'e'
  # print ('format ' + fmt)
  s = fmt % number
  spaces = ''
  for i in range ((int)(length - len(s))):
    spaces = spaces + ' '
  output = spaces + s
  # print (spaces + s)
  return output

def writeIntField(number, length):
  s = '%d' % number
  spaces = ''
  for i in range ((int)(length - len(s))):
    spaces = spaces + ' '
  output = spaces + s
  # print (spaces + s)
  return output

def Norm2(v):
  norm = 0.0
  if isinstance(v, Vector):
    for i in range (len(v.components)):
      norm = norm + v.components[i] * v.components[i]
  return norm
      
class Vector:
  def __init__(self, *components):
      self.components = components
  def __mul__(self, other):
    components = []
    if isinstance(other, Vector):
      # if (len(self.components)!=len(other.components)):
        # print ("Different length size")
      for i in range (len(self.components)):
        components.append(self.components[i] * other.components[i])
    else:
      components = (other * x for x in self.components)
    return Vector(*components)
  # addition is normally a binary operation between self and other object
  # def __add__(self, other):
    # if isinstance(other, Vector):
      # new_vec = Vector()
      # new_vec.X = self.X + other.X
      # new_vec.Y = self.X + other.Y
      # return new_vec
    # else:
      # raise TypeError("value must be a vector.")
  # def __add__(self, other):
    # added =[]
    # for i in range(len(self.components)):
      # #added = tuple( a + b for a, b in zip(self, other) )
      # added.append(self.components[i] + other.components[i])
      # return Vector(*added)
  def __add__(self, other):
    if isinstance(other, Vector):
    # other.args is the correct analog of self.args
      a = [arg1 + arg2 for arg1, arg2 in zip(self.components, other.components)]
    return self.__class__(*a)
  def __sub__(self, other):
    if isinstance(other, Vector):
    # other.args is the correct analog of self.args
      a = [arg1 - arg2 for arg1, arg2 in zip(self.components, other.components)]
    return self.__class__(*a)
  def Norm():
    norm = 0.0
    norm = [norm + arg1 for arg1 in self.components]
    norm = sqrt (norm)
    return norm
    
  def __repr__(self):
      return str(self.components)
      
  # # __repr__ and __str__ must return string
  # def __repr__(self):
    # # return str(self.components)
    # # return f"Vector{self.components}"
    # return str(self.components)

  def __str__(self):
    # return str(self.components)
    return "Vector{self.components}"
    # return str(self.components)
    
# def __add__(self, other):
  # if isinstance(other, Vector):
    # new_vec = Vector()
    # new_vec.X = self.X + other.X
    # new_vec.Y = self.X + other.Y
    # return new_vec
  # else:
    # raise TypeError("value must be a vector.")

#TODO: CHANGE ALL TO NODE
class Node(Vector):
  def __init__(self, id, *components):
    self.components = components
    self.id = id
    
    
    


#def sphere_ok(msh):
#def __init__(self, id, radius, ox,oy,oz, divisions):
def sphere(msh,id, radius, ox,oy,oz, divisions):
    nodes = []
    elnod = []
    existin_vtx = np.zeros((6, divisions+1, divisions+1)).astype(int)
    rep = 0
    # print ("existing vtk ", existin_vtx)
    print ("Creating Sphere mesh")
    #self.id = id
    CubeToSphere_origins = [
    Vector(-1.0, -1.0, -1.0), #ORGIINAL POINT ONE
    Vector(1.0, -1.0, -1.0),
    Vector(1.0, -1.0, 1.0),
    Vector(-1.0, -1.0, 1.0),
    Vector(-1.0, 1.0, -1.0),
    Vector(-1.0, -1.0, 1.0)]
    CubeToSphere_rights = [
    Vector(2.0, 0.0, 0.0),
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
    
    
    n = 0
    for face in range (6): #CUBE FACES 
      origin = CubeToSphere_origins[face]
      right = CubeToSphere_rights[face]
      # print (right)
      up = CubeToSphere_ups[face]
      for j in range (divisions+1):
        j3 = Vector(j,j,j)
        for i in range (divisions+1):
          i3 = Vector(i,i,i)
          put_node = True
          # print ("i3 j3 ", i3, j3)
          # print (right)
          # print ("origin ")
          # print (origin)
          # print ("right * origin ")

          # const Vector3 p = origin + step3 * (i3 * right + j3 * up);
          p = origin + ( step3 * (i3 * right  + up *j3 )  )
          p2 = p * p
          
          rx = p.components[0] * sqrt(1.0 - 0.5 * (p2.components[1] + p2.components[2]) + p2.components[1]*p2.components[2]/3.0)
          ry = p.components[1] * sqrt(1.0 - 0.5 * (p2.components[2] + p2.components[0]) + p2.components[2]*p2.components[0]/3.0)
          rz = p.components[2] * sqrt(1.0 - 0.5 * (p2.components[0] + p2.components[1]) + p2.components[0]*p2.components[1]/3.0)
          
          x = rx * radius + ox;           y = ry * radius + oy ;           z = rz * radius + oz;
          # print ("z , z corrected ", rz,z)
          
          # print ("node ", n, ", coords " ,x,y,z)
          existin_vtx[face][i][j] = n 
          
          for k in range (n): #CHECK IF THERE IS AN EXISTENT NODE THERE
            #if ( abs(x - nodes[k][0])<1.0e-4 and abs(y - nodes[k][1])<1.0e-4 and abs(z - nodes[k][2])<1.0e-4 ):
            if ( abs(x - msh.getNode(k).getPos(0))<1.0e-4 and abs(y - msh.getNode(k).getPos().y)<1.0e-4 and abs(z - msh.getNode(k).getPos().z)<1.0e-4 ):
              # print ("FOUND SIMILAR X in node ", k ,"face", face, "i, j ", i, j, "pos: ", x,y,z)
              rep = rep + 1
              put_node = False
              existin_vtx[face][i][j] = k 
          

          if (put_node):
            #self.nodes.append((n,x,y,z))
            msh.addNode(x,y,z)
            # print ("vertex ", n)
            n = n +1
    
    # print ("existing vtk ", existin_vtx)
    #print ("repeated nodes: ", rep)
    ##self.nodes.append((ox,oy,oz)) #CENTER AS RIGID PIVOT
    #print ("Sphere Origin: ", ox,oy,oz)
    #self.node_count = n + 1
    
    print ("Sphere generated: %d", n , " nodes      ")
    # print ("Node vector count: ", len(self.nodes))
    
      # print (origin)
    
    # for i in range (self.node_count):
      # print ("SPHERE Node ", i, self.nodes[i])

    # ORIGINAL 
    e = 0
    for face in range (6): #CUBE FACES
      for ey in range (divisions):
        for ex in range (divisions):  
          # self.elnod.append(((divisions+1)*ey+ex,(divisions+1)*ey + ex+1,(divisions+1)*(ey+1)+ex+1,(divisions+1)*(ey+1)+ex))      
          # print ("connectivity: ",(divisions+1)*ey+ex,(divisions+1)*ey + ex+1,(divisions+1)*(ey+1)+ex+1,(divisions+1)*(ey+1)+ex)  
          # print ("connectivity: ",existin_vtx[face][ex][ey], existin_vtx[face][ex+1][ey], existin_vtx[face][ex+1][ey+1], existin_vtx[face][ex][ey+1])
          #self.elnod.append((existin_vtx[face][ex][ey], existin_vtx[face][ex+1][ey], existin_vtx[face][ex+1][ey+1], existin_vtx[face][ex][ey+1]))
          msh.addQuad(existin_vtx[face][ex][ey], existin_vtx[face][ex+1][ey], existin_vtx[face][ex+1][ey+1], existin_vtx[face][ex][ey+1])
          e = e + 1
    #msh.elem_count = e
    
    
    
  
def sphere_segment(msh, id, radius, ox,oy,oz, divisions):
    print ("Creating Sphere mesh")
    #self.id = id
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
          #self.nodes.append((x,y,z))
          msh.addNode(x,y,z)
          n = n +1
    
    
    #self.nodes.append((ox,oy,oz)) #CENTER AS RIGID PIVOT
    #self.node_count = n + 1
    
    msh.addNode(ox,oy,oz)
    
    # print ("generated: %d", n , " nodes      ")
    # print ("Node vector count: ", len(self.nodes))
    
      # print (origin)
    
    # for i in range (self.node_count):
      # print ("SPHERE Node ", i, self.nodes[i])
    print("Mesh node count: ", msh.getNodeCount())
    e = 0
    for ey in range (divisions):
      for ex in range (divisions):
        # elem%elnod(i,:)=[(nel(1)+1)*ey + ex+1,(nel(1)+1)*ey + ex+2,(nel(1)+1)*(ey+1)+ex+2,(nel(1)+1)*(ey+1)+ex+1]  
        print ("Connectivty: ", (divisions+1)*ey+ex, (divisions+1)*ey + ex+1, (divisions+1)*(ey+1)+ex+1,(divisions+1)*(ey+1)+ex) 
         #self.elnod.append(((divisions+1)*ey+ex,(divisions+1)*ey + ex+1,(divisions+1)*(ey+1)+ex+1,(divisions+1)*(ey+1)+ex))    
        msh.addQuad((divisions+1)*ey+ex, (divisions+1)*ey + ex+1, (divisions+1)*(ey+1)+ex+1,(divisions+1)*(ey+1)+ex)  
        e = e + 1
    #self.elem_count = e
