#ifndef _Kompas3DPrint_Edge_h_
#define _Kompas3DPrint_Edge_h_

#include "Face.h"
#include "Vertex.h"

#include <optional>
#include <vector>

class Edge : public Node {
public:
	enum class Kind { Line, Circle, Arc, Ellipse, Other };

	class EdgeImpl : virtual public Node::NodeImpl {
	public:
		virtual std::unique_ptr<Node::NodeImpl> GetAdjacentFace(bool left) = 0;
		virtual std::unique_ptr<Node::NodeImpl> GetVertex(bool begin) = 0;
		virtual Vertex::Point3D GetOrigin() = 0;
		virtual Kind GetKind() { return Kind::Other; }
		virtual double GetLength() { return 0.0; }
		virtual std::optional<Box3D> GetBox() { return std::nullopt; }
		virtual std::optional<Vertex::Point3D> GetPointAt(double t) { return std::nullopt; }
		virtual std::vector<std::optional<Vertex::Point3D>> GetPointsAt(const std::vector<double>& ts) {
			std::vector<std::optional<Vertex::Point3D>> points;
			for (double t : ts) points.push_back(GetPointAt(t));
			return points;
		}
		virtual std::optional<double> GetRadius() { return std::nullopt; }
		virtual std::string GetOwnerName() { return std::string(); }
	};

	static inline int TYPE = 7; /* o3d_edge */
	Edge(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	Edge(Node&& node) : Node(std::move(node)) {}
	Face LeftFace() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Face(edge ? edge->GetAdjacentFace(true) : nullptr);
	}
	Face RightFace() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Face(edge ? edge->GetAdjacentFace(false) : nullptr);
	}
	Vertex GetBeginVertex() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Vertex(edge ? edge->GetVertex(true) : nullptr);
	}
	Vertex GetEndVertex() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return Vertex(edge ? edge->GetVertex(false) : nullptr);
	}
	// Центр дуги, окружности или эллипса
	Vertex::Point3D GetOrigin() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetOrigin() : Vertex::Point3D{0., 0., 0.};
	}
	Kind GetKind() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetKind() : Kind::Other;
	}
	// Длина, мм
	double GetLength() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetLength() : 0.0;
	}
	std::optional<Box3D> GetBox() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetBox() : std::nullopt;
	}
	// Точка кривой: t = 0 — начало, 1 — конец, 0.5 — середина
	std::optional<Vertex::Point3D> GetPointAt(double t) const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetPointAt(t) : std::nullopt;
	}
	// Несколько точек кривой за один проход (быстрее, чем GetPointAt по одной)
	std::vector<std::optional<Vertex::Point3D>> GetPointsAt(const std::vector<double>& ts) const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetPointsAt(ts) : std::vector<std::optional<Vertex::Point3D>>(ts.size());
	}
	// Радиус окружности или дуги
	std::optional<double> GetRadius() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetRadius() : std::nullopt;
	}
	// Имя операции, которая создала ребро
	std::string GetOwnerName() const {
		EdgeImpl* edge = dynamic_cast<EdgeImpl*>(node.get());
		return edge ? edge->GetOwnerName() : std::string();
	}
};

#endif
