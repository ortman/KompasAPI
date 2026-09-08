#ifndef _Kompas3DPrint_CutRotated_h_
#define _Kompas3DPrint_CutRotated_h_

#include "Sketch.h"
#include "Axis.h"

class CutRotated : public Node {
public:
	class CutRotatedImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetSketch(Sketch& sketch) = 0;
		virtual void SetAxis(const Axis& axis) = 0;
		virtual void SetAngle(double angle) = 0;
	};

	static inline int TYPE = 29; /* o3d_cutRotated */
	CutRotated(std::unique_ptr<NodeImpl> p, Sketch& sketch, const Axis& axis, double angle = 360.0, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		CutRotatedImpl* rotated = dynamic_cast<CutRotatedImpl*>(node.get());
		if (rotated) {
			rotated->SetAngle(angle);
			rotated->SetSketch(sketch);
			rotated->SetAxis(axis);
		}
		node->Create();
	}
	CutRotated(Node&& node) : Node(std::move(node)) {}
	CutRotated& SetAngle(double angle) {
		CutRotatedImpl* rotated = dynamic_cast<CutRotatedImpl*>(node.get());
		if (rotated) rotated->SetAngle(angle);
		return *this;
	}
};

#endif
