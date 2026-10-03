#pragma once

#include "../../Include/Node/Edge.h"
#include "../Node.hpp"
#include "Face.hpp"
#include "Geometry.hpp"
#include "Vertex.hpp"

class EdgeApi7 : public NodeApi7, virtual public Edge::EdgeImpl {
	K5::ksEdgeDefinitionPtr Def() {
		K5::ksEdgeDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksEdgeDefinition");
		return d;
	}

public :
	EdgeApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	std::unique_ptr<Node::NodeImpl> GetAdjacentFace(bool left) override {
		K5::ksEdgeDefinitionPtr d = Def();
		K5::ksFaceDefinitionPtr faceDef = d->GetAdjacentFace(left);
		if (!faceDef) throw Kompas3DException(left ? "Не могу получить левую грань ребра" : "Не могу получить правую грань ребра");
		K5::ksEntityPtr face = faceDef->GetEntity();
		return std::make_unique<FaceApi7>(face, faceDef);
	}
	std::unique_ptr<Node::NodeImpl> GetVertex(bool begin) override {
		K5::ksEdgeDefinitionPtr d = Def();
		K5::ksVertexDefinitionPtr vertexDef = d->GetVertex(begin);
		if (!vertexDef) throw Kompas3DException("Не могу получить вершину ребра");
		return std::make_unique<VertexApi7>(nullptr, vertexDef);
	}
	Vertex::Point3D GetOrigin() override {
		K5::ksEdgeDefinitionPtr d = Def();
		K5::ksCurve3DPtr curve = d->GetCurve3D();
		if (!curve) return {0., 0., 0.};
		Vertex::Point3D res;
		if (K5::ksArc3dParamPtr param = curve->GetCurveParam()) {
			K5::ksPlacementPtr pl = param->GetPlacement();
			if (pl && pl->GetOrigin(&res.x, &res.y, &res.z)) return res;
		}
		if (K5::ksCircle3dParamPtr param = curve->GetCurveParam()) {
			K5::ksPlacementPtr pl = param->GetPlacement();
			if (pl && pl->GetOrigin(&res.x, &res.y, &res.z)) return res;
		}
		if (K5::ksEllipse3dParamPtr param = curve->GetCurveParam()) {
			K5::ksPlacementPtr pl = param->GetPlacement();
			if (pl && pl->GetOrigin(&res.x, &res.y, &res.z)) return res;
		}
		return {0., 0., 0.};
	}
	Edge::Kind GetKind() override {
		K5::ksEdgeDefinitionPtr d = Def();
		if (d->IsStraight()) return Edge::Kind::Line;
		if (d->IsCircle()) return Edge::Kind::Circle;
		if (d->IsArc()) return Edge::Kind::Arc;
		if (d->IsEllipse() || d->IsEllipseArc()) return Edge::Kind::Ellipse;
		return Edge::Kind::Other;
	}
	double GetLength() override {
		return Def()->GetLength(0x1 /*ST_MIX_MM*/);
	}
	std::optional<Box3D> GetBox() override {
		return Api7Geometry::CurveBox(Def()->GetCurve3D());
	}
	std::optional<Vertex::Point3D> GetPointAt(double t) override {
		K5::ksCurve3DPtr curve = Def()->GetCurve3D();
		if (!curve) return std::nullopt;
		double tMin = curve->GetParamMin(), tMax = curve->GetParamMax();
		Vertex::Point3D p;
		if (!curve->GetPoint(tMin + (tMax - tMin) * t, &p.x, &p.y, &p.z)) return std::nullopt;
		return p;
	}
	std::vector<std::optional<Vertex::Point3D>> GetPointsAt(const std::vector<double>& ts) override {
		std::vector<std::optional<Vertex::Point3D>> points(ts.size());
		K5::ksCurve3DPtr curve = Def()->GetCurve3D();
		if (!curve) return points;
		double tMin = curve->GetParamMin(), tMax = curve->GetParamMax();
		for (size_t i = 0; i < ts.size(); ++i) {
			Vertex::Point3D p;
			if (curve->GetPoint(tMin + (tMax - tMin) * ts[i], &p.x, &p.y, &p.z)) points[i] = p;
		}
		return points;
	}
	std::optional<double> GetRadius() override {
		K5::ksCurve3DPtr curve = Def()->GetCurve3D();
		if (!curve) return std::nullopt;
		if (K5::ksArc3dParamPtr param = curve->GetCurveParam()) return param->radius;
		if (K5::ksCircle3dParamPtr param = curve->GetCurveParam()) return param->radius;
		return std::nullopt;
	}
	std::string GetOwnerName() override {
		K5::ksEntityPtr owner = Def()->GetOwnerEntity();
		return owner ? BstrToUtf8(owner->name) : std::string();
	}
};

inline std::vector<std::unique_ptr<Node::NodeImpl>> FaceApi7::GetEdges() {
	std::vector<std::unique_ptr<Node::NodeImpl>> result;
	K5::ksEdgeCollectionPtr edges = Def()->EdgeCollection();
	int count = edges ? edges->GetCount() : 0;
	for (int i = 0; i < count; ++i) {
		K5::ksEdgeDefinitionPtr edge = edges->GetByIndex(i);
		if (edge) result.push_back(std::make_unique<EdgeApi7>(edge->GetEntity(), edge));
	}
	return result;
}
