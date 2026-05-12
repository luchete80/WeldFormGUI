import model

def make_node_set(mesh, set_id, name, node_ids):
    s = model.NodeSet(mesh)
    s.setEntityId(set_id)
    s.setLabel(name)

    wanted = set(node_ids)
    for i in range(mesh.getNodeCount()):
        n = mesh.getNode(i)
        if n.getId() in wanted:
            s.add(n)

    mesh.addNodeSet(s)
    return s

def make_element_set(mesh, set_id, name, element_ids):
    s = model.ElementSet(mesh)
    s.setEntityId(set_id)
    s.setLabel(name)

    wanted = set(element_ids)
    for i in range(mesh.getElemCount()):
        e = mesh.getElem(i)
        if e.getId() in wanted:
            s.add(e)

    mesh.addElementSet(s)
    return s


