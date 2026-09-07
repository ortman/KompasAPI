#ifndef _Kompas3DPrint_Edge_h_
#define _Kompas3DPrint_Edge_h_

#include "Face.h"
#include "Vertex.h"

class Edge : public Node {
public:
	class EdgeImpl : virtual public Node::NodeImpl {
	public:
		virtual std::unique_ptr<Node::NodeImpl> GetAdjacentFace(bool left) = 0;
		virtual std::unique_ptr<Node::NodeImpl> GetVertex(bool begin) = 0;
		virtual Vertex::Point3D GetOrigin() = 0;
	};

	static inline int TYPE = 7; /* o3d_edge */
	Edge(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	//Edge(const Node& node) : Node(node) {}
	Face LeftFace() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Face(edge ? edge->GetAdjacentFace(true) : nullptr);
	}
	Face RightFace() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Face(edge ? edge->GetAdjacentFace(false) : nullptr);
	}
	Vertex GetBeginVertex() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Vertex(edge ? edge->GetVertex(true) : nullptr);
	}
	Vertex GetEndVertex() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Vertex(edge ? edge->GetVertex(false) : nullptr);
	}
	Vertex::Point3D GetOrigin() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetOrigin() : Vertex::Point3D{0., 0., 0.};
	}
};

#endif
