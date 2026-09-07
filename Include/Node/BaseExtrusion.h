#ifndef _ComTest_BaseExtrusion_h_
#define _ComTest_BaseExtrusion_h_

#include "Sketch.h"

class BaseExtrusion : public Node {
public:
	class BaseExtrusionImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetDepth1(double depth) = 0;
		virtual void SetDepth2(double depth) = 0;
		virtual void SetSketch(Sketch& sketch) = 0;
	};

	static inline int TYPE = 24; /* o3d_baseExtrusion */
	BaseExtrusion(std::unique_ptr<NodeImpl> p, Sketch& sketch, double depth1, double depth2 = 0.0, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		BaseExtrusionImpl* extrusion = dynamic_cast<BaseExtrusionImpl*>(node.get());
		if (extrusion) {
			extrusion->SetDepth1(depth1);
			extrusion->SetDepth2(depth2);
			extrusion->SetSketch(sketch);
		}
		node->Create();
	}
	//BaseExtrusion(const Node& node) : Node(node.pEntity, node.pDefinition) {}
	BaseExtrusion() : Node(nullptr) {}
	BaseExtrusion& SetDepth1(double depth) {
		BaseExtrusionImpl* extrusion = dynamic_cast<BaseExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetDepth2(depth);
		return *this;
	}
	BaseExtrusion& SetDepth2(double depth) {
		BaseExtrusionImpl* extrusion = dynamic_cast<BaseExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetDepth1(depth);
		return *this;
	}
};

#endif
