#ifndef _Kompas3DPrint_Face_h_
#define _Kompas3DPrint_Face_h_

#include "../Node.h"

class Face : public Node {
private:
	class FaceImpl : virtual public Node::NodeImpl {
	public:
		virtual bool IsPlanar() = 0;
		virtual bool IsCylinder() = 0;
	};

public:
	static inline int TYPE = 6; /* o3d_face */
	Face(std::unique_ptr<FaceImpl> p) : Node(std::move(p)) {
		node->Create();
	}
	//Face(const Node& node) : Node(node) {}
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
