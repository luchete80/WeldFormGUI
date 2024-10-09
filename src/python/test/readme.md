https://www.nottingham.ac.uk/home/eaziaj/uon/swig-1.3.31/Examples/python/class/index.html

Key points
To create a new object, you call a constructor like this:
c = example.new_Circle(10.0)

To access member data, a pair of accessor functions are used. For example:
example.Shape_x_set(c,15)    # Set member data
x = example.Shape_x_get(c)    # Get member data
Note: when accessing member data, the name of the class in which the member data was must be used. In this case, Shape_x_get() and Shape_x_set() are used since 'x' was defined in Shape.

To invoke a member function, you simply do this
print "The area is ", example.Shape_area(c)

Type checking knows about the inheritance structure of C++. For example:
example.Shape_area(c)       # Works (c is a Shape)
example.Circle_area(c)      # Works (c is a Circle)
example.Square_area(c)      # Fails (c is definitely not a Square)

To invoke a destructor, simply do this
example.delete_Shape(c)     # Deletes a shape
(Note: destructors are currently not inherited. This might change later).

Static member variables are wrapped as C global variables. For example:
n = example.cvar.Shape_nshapes     # Get a static data member
example.cvar.Shapes_nshapes = 13   # Set a static data member
