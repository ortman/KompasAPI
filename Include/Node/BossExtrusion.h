#ifndef _KompasAPI_BossExtrusion_h_
#define _KompasAPI_BossExtrusion_h_

#include "Sketch.h"
#include "Extrusion.h"

class BossExtrusion : public Node {
public:
	class BossExtrusionImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetDepth1(double depth) = 0;
		virtual void SetDepth2(double depth) = 0;
		virtual void SetSketch(Sketch& sketch) = 0;
		virtual void SetParams(const ExtrusionParams& params) = 0;
	};

	static inline int TYPE = 25; /* o3d_bossExtrusion: приклеить выдавливанием */
	BossExtrusion(std::unique_ptr<NodeImpl> p, Sketch& sketch, double depth1, double depth2 = 0.0, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		BossExtrusionImpl* extrusion = dynamic_cast<BossExtrusionImpl*>(node.get());
		if (extrusion) {
			extrusion->SetDepth1(depth1);
			extrusion->SetDepth2(depth2);
			extrusion->SetSketch(sketch);
		}
		node->Create();
	}
	BossExtrusion(std::unique_ptr<NodeImpl> p, Sketch& sketch, const ExtrusionParams& params, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		BossExtrusionImpl* extrusion = dynamic_cast<BossExtrusionImpl*>(node.get());
		if (extrusion) {
			extrusion->SetParams(params);
			extrusion->SetSketch(sketch);
		}
		node->Create();
	}
	BossExtrusion& SetParams(const ExtrusionParams& params) {
		BossExtrusionImpl* extrusion = dynamic_cast<BossExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetParams(params);
		return *this;
	}
	BossExtrusion(Node& node) : Node(std::move(node.node)) {}
	BossExtrusion() : Node(nullptr) {}
	BossExtrusion(Node&& node) : Node(std::move(node)) {}
	BossExtrusion& SetDepth1(double depth) {
		BossExtrusionImpl* extrusion = dynamic_cast<BossExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetDepth1(depth);
		return *this;
	}
	BossExtrusion& SetDepth2(double depth) {
		BossExtrusionImpl* extrusion = dynamic_cast<BossExtrusionImpl*>(node.get());
		if (extrusion) extrusion->SetDepth2(depth);
		return *this;
	}
};

#endif
