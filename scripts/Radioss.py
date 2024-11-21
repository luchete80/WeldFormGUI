from model import *
from math import *

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
    return f"Vector{self.components}"
    # return str(self.components)
    
# def __add__(self, other):
  # if isinstance(other, Vector):
    # new_vec = Vector()
    # new_vec.X = self.X + other.X
    # new_vec.Y = self.X + other.Y
    # return new_vec
  # else:
    # raise TypeError("value must be a vector.")
    
class RadiossExporter:
  def __init__(self, model):
    self.model = model
  def printNodes(self,f):
    m = self.model
    f.write('/NODES\n')
    print ("printing ", m.getNodeCount(), " nodes")
    for n in range(m.getNodeCount()):
      line = writeIntField(n+1,10)
      
      for d in range (3):
        line = line + writeFloatField(m.getNode(n).getPos(d),20,6) 
      # f.write("%.6e, %.6e\n" % (self.nodes[i][0],self.nodes[i][1]))
      f.write(line + '\n')
      
  def printPart(self, part,f):                          
    f.write('/SHELL/' + str(part.getId()) + '\n')
    print ("Printing Element ", part.getMesh().getElemCount())
    
    for i in range (part.getMesh().getElemCount()):
      line = ''
      e = part.getMesh().getElem(i)
      #line = writeIntField(i + part.mesh[0].ini_elem_id ,10
      line = writeIntField(e.getId(), 10)
      #line = writeIntField(i + part.getMesh()mesh[0].ini_elem_id ,10)
      #line = writeIntField(i + part.getMesh()mesh[0].ini_elem_id ,10)
      for d in range (e.getNodeCount()):
        # print (part.mesh[0].ini_node_id, ", ")
        line = line + writeIntField(e.getNodeId(d),10)
      f.write(line + '\n')  

      line = "/PART/%d\n" % part.getId()
      f.write(line)
      #f.write(part.title)                                                                                            
      f.write("#     pid     mid\n")
      #f.write(writeIntField(part.pid, 10) + "         2\n") 
      f.write(writeIntField(part.getId(), 10) + "         2\n") 
      #line = "/GRNOD/PART/%d\n" % part.id    
      #line = line + "PART_%d\n" % part.id
      #line = line + writeIntField(part.id,10) + "\n"
      f.write(line)
    
           
  def printModel(self, fname):
    f = open(fname,"w+")
    self.printNodes(f)
    for p in (range(self.model.getPartCount())):
      print ("Self part count ", self.model.getPartCount())
      self.printPart(self.model.getPart(p),f)
    #line = "/PART/%d\n" % part.id
    #f.write(line)
    #f.write(part.title)                                                                                            
    #f.write("#     pid     mid\n")
    #f.write("      1         2\n") 
    #line = "/GRNOD/PART/%d\n" % part.id    
    #line = line + "PART_%d\n" % part.id
    #line = line + writeIntField(part.id,10) + "\n"
    #f.write(line)
    #
    #GRNOD FOR MOVE 
    #if (part.is_moving):
    #  line = "/GRNOD/NODE/%d\n" % part.id_grn_move    
#      line = line + "MOVE_%d\n" % part.id
#      line = line + writeIntField(part.mesh[0].ini_node_id + part.mesh[0].node_count - 1, 10) + "\n"
#      f.write(line)
#
    f.close()
#      line = "/BCS/%d\n" % part.id
#      line = line + "BoundSpcSet_1 \n"  
#      line = line + "#  Tra rot   skew_ID  grnod_ID\n"
#      line = line + "   000 111         0" + writeIntField(100+part.id, 10) + "\n"
#      f.write(line)

    
#    if (part.mesh[0].print_segments):
#      part.mesh[0].printESurfsRadioss(f) 
#    if (part.is_rigid):
#      part.mesh[0].printRigidRadioss(f) 
