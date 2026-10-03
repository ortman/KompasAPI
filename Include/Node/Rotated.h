#ifndef _KompasAPI_Rotated_h_
#define _KompasAPI_Rotated_h_

#include "Sketch.h"
#include "Axis.h"
#include "Extrusion.h"

// Операции вращения: BaseRotated (первое тело), BossRotated (приклеить). Вырез
// вращением — CutRotated. Ось — объект модели (Axis) или, если не задана, осевая
// линия эскиза.

struct RotatedParams {
	double angle1 = 360.0;   // угол в прямом направлении, градусы
	double angle2 = 0.0;     // угол в обратном направлении (для Both)
	ExtrusionDirection direction = ExtrusionDirection::Normal;
};

class RotatedImpl : virtual public Node::NodeImpl {
public:
	virtual void SetSketch(Sketch& sketch) = 0;
	virtual void SetAxis(const Axis& axis) = 0;
	virtual void SetParams(const RotatedParams& params) = 0;
};

template <int Type>
class RotatedNode : public Node {
public:
	static inline int TYPE = Type;
	RotatedNode(std::unique_ptr<NodeImpl> p, Sketch& sketch, const Axis* axis, const RotatedParams& params,
	            const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		RotatedImpl* rotated = dynamic_cast<RotatedImpl*>(node.get());
		if (rotated) {
			rotated->SetParams(params);
			rotated->SetSketch(sketch);
			if (axis) rotated->SetAxis(*axis);
		}
		node->Create();
	}
	RotatedNode(Node&& node) : Node(std::move(node)) {}
	RotatedNode() : Node(nullptr) {}
};

using BaseRotated = RotatedNode<27>;   /* o3d_baseRotated */
using BossRotated = RotatedNode<28>;   /* o3d_bossRotated */

#endif
