#ifndef _ComTest_Axis_h_
#define _ComTest_Axis_h_

#include "Face.h"
#include "Vertex.h"

class Axis : public Node {
public:
	class AxisImpl : virtual public Node::NodeImpl {
	public:
		virtual void Show(bool show) = 0;
		virtual Vertex::Point3D GetFirstPoint() = 0;
		virtual Vertex::Point3D GetLastPoint() = 0;
	};

	static inline int TYPE = 71; /* o3d_axisOX */
	Axis(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	//Axis(const Node& node) : Node(node) {}
	Vertex::Point3D GetFirstPoint() const {
		AxisImpl* axis = dynamic_cast<AxisImpl*>(node.get());
		return axis ? axis->GetFirstPoint() : Vertex::Point3D{0., 0., 0.};
	}
	Vertex::Point3D GetLastPoint() const {
		AxisImpl* axis = dynamic_cast<AxisImpl*>(node.get());
		return axis ? axis->GetLastPoint() : Vertex::Point3D{0., 0., 0.};
	}
	Axis& Show(bool show = true) {
		AxisImpl* axis = dynamic_cast<AxisImpl*>(node.get());
		if (axis) axis->Show(show);
		return *this;
	}
	Axis& Hide() { return Show(false); }

protected:
	// Для производных осей: Create() вызывается после настройки параметров
	Axis(std::unique_ptr<NodeImpl> p, bool create) : Node(std::move(p)) {
		if (create) node->Create();
	}
};

class ConeAxis : public Axis {
public:
	class ConeAxisImpl : virtual public Axis::AxisImpl {
	public:
		virtual void SetFace(const Face& coneFace) = 0;
	};

	static inline int TYPE = 11; /* o3d_axisConeFace */
	ConeAxis(std::unique_ptr<NodeImpl> p, const Face& coneFace, bool show = false, const std::optional<std::string>& name = std::nullopt) : Axis(std::move(p), false) {
		if (name.has_value()) node->SetName(name.value());
		ConeAxisImpl* axis = dynamic_cast<ConeAxisImpl*>(node.get());
		if (axis) {
			axis->SetFace(coneFace);
			axis->Show(show);
		}
		node->Create();
	}
	//ConeAxis(const Node& node) : Axis(node) {}
	ConeAxis& SetFace(const Face& coneFace) {
		ConeAxisImpl* axis = dynamic_cast<ConeAxisImpl*>(node.get());
		if (axis) axis->SetFace(coneFace);
		return *this;
	}
};

#endif
