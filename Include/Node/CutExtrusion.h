#ifndef _ComTest_CutExtrusion_h_
#define _ComTest_CutExtrusion_h_

#include "Sketch.h"

class CutExtrusion : public Node {
public:
	class CutExtrusionImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetDepth1(double depth) = 0;
		virtual void SetDepth2(double depth) = 0;
		virtual void SetSketch(Sketch& sketch) = 0;
	};

	static inline int TYPE = 26; /* o3d_cutExtrusion */
	CutExtrusion(std::unique_ptr<NodeImpl> p, Sketch& sketch, double depth1, double depth2 = 0.0, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		CutExtrusionImpl* extrusion = dynamic_cast<CutExtrusionImpl*>(node.get());
		if (extrusion) {
			extrusion->SetDepth1(depth1);
			extrusion->SetDepth2(depth2);
			extrusion->SetSketch(sketch);
		}
		node->Create();
	}
	//CutExtrusion(const Node& node) : Node(node.pEntity, node.pDefinition) {}
	CutExtrusion& SetDepth1(double depth) {
		CutExtrusionImpl* extrusion = dynamic_cast<CutExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetDepth2(depth);
		return *this;
	}
	CutExtrusion& SetDepth2(double depth) {
		CutExtrusionImpl* extrusion = dynamic_cast<CutExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetDepth1(depth);
		return *this;
	}
};

#endif
