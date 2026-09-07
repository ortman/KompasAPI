#ifndef _Kompas3DPrint_CutRotated_h_
#define _Kompas3DPrint_CutRotated_h_

#include "Sketch.h"
#include "Axis.h"

class CutRotated : public Node {
public:
	class CutRotatedImpl : virtual public Node::NodeImpl {
	public:
	};

	static inline int TYPE = 29; /* o3d_cutRotated */
	CutRotated(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {
		node->Create();
	}
	//CutRotated(const Node& node) : Node(node.pEntity, node.pDefinition) {}
	//CutRotated(IUnknown* pEntity, IDispatch* pDefinition, Sketch& sketch, const Axis& axis, const std::optional<std::string>& name = std::nullopt);
};

#endif
