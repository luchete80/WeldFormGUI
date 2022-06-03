#version 330                                                                        
                                                                                                                                                                                                                                                        
uniform vec4 gPickingColor;
                                                                             
out vec4 FragColor;

void main()                                                                         
{                                                                                   
   //FragColor = vec3(float(gObjectIndex), float(gDrawIndex),float(gl_PrimitiveID + 1));      
  //FragColor = vec3(gObjectIndex/255., 255.0,255.0);        
  FragColor =  gPickingColor; 
}