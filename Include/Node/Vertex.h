#ifndef _Kompas3DPrint_Vertex_h_
#define _Kompas3DPrint_Vertex_h_

#include "../Node.h"

class Vertex : public Node {
public:
	struct Point3D {
		double x, y, z;
	};

private:
	class VertexImpl : virtual public Node::NodeImpl {
	public:
		virtual Vertex::Point3D ToPoint() = 0;
	};

public:
	static inline int TYPE = 8; /* o3d_vertex */
	//Vertex(const Node& node) : Node(node) {}
	Vertex(std::unique_ptr<VertexImpl> p) : Node(std::move(p)) {}
	operator Point3D const () {
		VertexImpl* vertex = dynamic_cast<VertexImpl*>(node.get());
		return vertex ? vertex->ToPoint() : Vertex::Point3D{0, 0, 0};
	}
};

#endif
