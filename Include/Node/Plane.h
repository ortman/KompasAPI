#ifndef _ComTest_Plane_h_
#define _ComTest_Plane_h_

#include "../Node.h"
#include "Face.h"
#include "Vertex.h"
#include "Axis.h"

class Plane : public Node {
public:
	struct Point2D {
		double x, y;
	};
	enum AxisType {
		OX = 0,
		OY = 1,
		OZ = 2
	};
	class PlaneImpl : virtual public Node::NodeImpl {
	public:
		virtual void Show(bool show) = 0;
		virtual Vertex::Point3D GetVector(AxisType type) = 0;
		virtual Plane::Point2D Projection(const Vertex::Point3D& point) = 0;
		virtual Vertex::Point3D Projection(const Plane::Point2D& point) = 0;
	};

	static inline int TYPE = 1; /* o3d_planeXOY */
	Plane(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {
		node->Create();
	}
	Plane(Node&& node) : Node(std::move(node)) {}
	Vertex::Point3D GetVector(AxisType type) {
		PlaneImpl* plane = dynamic_cast<PlaneImpl*>(node.get());
		return plane ? plane->GetVector(type) : Vertex::Point3D{0., 0., 0.};
	}
	Plane::Point2D Projection(const Vertex::Point3D& point) {
		PlaneImpl* plane = dynamic_cast<PlaneImpl*>(node.get());
		return plane ? plane->Projection(point) : Plane::Point2D{0., 0.};
	}
	Vertex::Point3D Projection(const Plane::Point2D& point) {
		PlaneImpl* plane = dynamic_cast<PlaneImpl*>(node.get());
		return plane ? plane->Projection(point) : Vertex::Point3D{0., 0., 0.};
	}
	Plane& Show(bool show = true) {
		PlaneImpl* plane = dynamic_cast<PlaneImpl*>(node.get());
		if (plane) plane->Show(show);
		return *this;
	}
	Plane& Hide() { return Show(false); }

protected:
	// Для производных плоскостей: Create() вызывается после настройки параметров
	Plane(std::unique_ptr<NodeImpl> p, bool create) : Node(std::move(p)) {
		if (create) node->Create();
	}
};

class ParallelPlane : public Plane {
public:
	class ParallelPlaneImpl : virtual public Plane::PlaneImpl {
	public:
		virtual void SetPlane(const Face& planarFace) = 0;
		virtual void SetPoint(const Vertex& point) = 0;
	};

	static inline int TYPE = 20; /* o3d_planeParallel */
	ParallelPlane(std::unique_ptr<NodeImpl> p, const Face& planarFace, const Vertex& point, bool show = false, const std::optional<std::string>& name = std::nullopt) : Plane(std::move(p), false) {
		if (name.has_value()) node->SetName(name.value());
		ParallelPlaneImpl* plane = dynamic_cast<ParallelPlaneImpl*>(node.get());
		if (plane) {
			plane->SetPlane(planarFace);
			plane->SetPoint(point);
			plane->Show(show);
		}
		node->Create();
	}
	ParallelPlane(Node&& node) : Plane(std::move(node)) {}
	ParallelPlane& SetPlane(const Face& planarFace) {
		ParallelPlaneImpl* plane = dynamic_cast<ParallelPlaneImpl*>(node.get());
		if (plane) plane->SetPlane(planarFace);
		return *this;
	}
	ParallelPlane& SetPoint(const Vertex& point) {
		ParallelPlaneImpl* plane = dynamic_cast<ParallelPlaneImpl*>(node.get());
		if (plane) plane->SetPoint(point);
		return *this;
	}
};

class EdgePointPlane : public Plane {
public:
	class EdgePointPlaneImpl : virtual public Plane::PlaneImpl {
	public:
		virtual void SetEdge(const Axis& axis) = 0;
		virtual void SetPoint(const Vertex& point) = 0;
	};

	static inline int TYPE = 19; /* o3d_planeEdgePoint */
	EdgePointPlane(std::unique_ptr<NodeImpl> p, const Axis& axis, const Vertex& point, bool show = false, const std::optional<std::string>& name = std::nullopt) : Plane(std::move(p), false) {
		if (name.has_value()) node->SetName(name.value());
		EdgePointPlaneImpl* plane = dynamic_cast<EdgePointPlaneImpl*>(node.get());
		if (plane) {
			plane->SetEdge(axis);
			plane->SetPoint(point);
			plane->Show(show);
		}
		node->Create();
	}
	EdgePointPlane(Node&& node) : Plane(std::move(node)) {}
	EdgePointPlane& SetEdge(const Axis& axis) {
		EdgePointPlaneImpl* plane = dynamic_cast<EdgePointPlaneImpl*>(node.get());
		if (plane) plane->SetEdge(axis);
		return *this;
	}
	EdgePointPlane& SetPoint(const Vertex& point) {
		EdgePointPlaneImpl* plane = dynamic_cast<EdgePointPlaneImpl*>(node.get());
		if (plane) plane->SetPoint(point);
		return *this;
	}
};

#endif
