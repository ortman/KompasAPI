#ifndef _Kompas3DPrint_Face_h_
#define _Kompas3DPrint_Face_h_

#include "../Node.h"

class Face : public Node {
public:
	class FaceImpl : virtual public Node::NodeImpl {
	public:
		virtual bool IsPlanar() = 0;
		virtual bool IsCylinder() = 0;
	};

public:
	static inline int TYPE = 6; /* o3d_face */
	Face(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	// Перехват уже существующего узла
	Face(Node&& node) : Node(std::move(node)) {}
	bool IsPlanar() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->IsPlanar() : false;
	}
	bool IsCylinder() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->IsCylinder() : false;
	}
};

#endif
