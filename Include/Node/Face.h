#ifndef _Kompas3DPrint_Face_h_
#define _Kompas3DPrint_Face_h_

#include "../Node.h"
#include "Vertex.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class Face : public Node {
public:
	enum class Kind { Plane, Cylinder, Cone, Sphere, Torus, Other };

	struct Cylinder {
		double radius;
		Vertex::Point3D origin;   // точка на оси
		Vertex::Point3D axis;     // единичный вектор оси
	};

	// Точка в середине области параметров и внешняя нормаль в ней (из материала)
	struct Sample {
		Vertex::Point3D point;
		Vertex::Point3D normal;
	};

	class FaceImpl : virtual public Node::NodeImpl {
	public:
		virtual bool IsPlanar() = 0;
		virtual bool IsCylinder() = 0;
		virtual Kind GetKind() { return IsPlanar() ? Kind::Plane : IsCylinder() ? Kind::Cylinder : Kind::Other; }
		virtual double GetArea() { return 0.0; }
		virtual std::optional<Vertex::Point3D> GetNormal() { return std::nullopt; }
		virtual std::optional<Sample> GetSample() { return std::nullopt; }
		virtual std::optional<Box3D> GetBox() { return std::nullopt; }
		virtual std::string GetOwnerName() { return std::string(); }
		virtual std::optional<Cylinder> GetCylinder() { return std::nullopt; }
		virtual std::vector<std::unique_ptr<Node::NodeImpl>> GetEdges() { return {}; }
	};

public:
	static inline int TYPE = 6; /* o3d_face */
	Face(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	// Перехват уже существующего узла
	Face(Node&& node) : Node(std::move(node)) {}
	bool IsPlanar() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->IsPlanar() : false;
	}
	bool IsCylinder() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->IsCylinder() : false;
	}
	Kind GetKind() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetKind() : Kind::Other;
	}
	// Площадь, мм²
	double GetArea() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetArea() : 0.0;
	}
	// Внешняя нормаль плоской грани (направлена из материала); nullopt для неплоских
	std::optional<Vertex::Point3D> GetNormal() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetNormal() : std::nullopt;
	}
	// Точка грани и внешняя нормаль в ней: у цилиндра или конуса отличает наружную
	// поверхность (нормаль от оси) от отверстия (к оси)
	std::optional<Sample> GetSample() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetSample() : std::nullopt;
	}
	// Габарит грани по её рёбрам, мм
	std::optional<Box3D> GetBox() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetBox() : std::nullopt;
	}
	// Имя операции, которая создала грань
	std::string GetOwnerName() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetOwnerName() : std::string();
	}
	// Радиус и ось цилиндрической грани
	std::optional<Cylinder> GetCylinder() {
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		return face ? face->GetCylinder() : std::nullopt;
	}
	// Рёбра грани; типизировать через Edge(std::move(node))
	std::vector<Node> GetEdges() {
		std::vector<Node> edges;
		FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
		if (face) {
			for (auto& impl : face->GetEdges()) edges.push_back(Node(std::move(impl)));
		}
		return edges;
	}
};

#endif
