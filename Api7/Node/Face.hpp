#pragma once

#include "../../Include/Node/Face.h"
#include "../Node.hpp"
#include "Geometry.hpp"

#include <cmath>

class FaceApi7 : public NodeApi7, public Face::FaceImpl {
	K5::ksFaceDefinitionPtr Def() {
		K5::ksFaceDefinitionPtr face = def;
		if (!face) throw Kompas3DException("Не могу получить ksFaceDefinition");
		return face;
	}

public :
	FaceApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}

	bool IsPlanar() override {
		K5::ksFaceDefinitionPtr face = def;
		return face ? face->IsPlanar() : false;
	}

	bool IsCylinder() override {
		K5::ksFaceDefinitionPtr face = def;
		return face ? face->IsCylinder() : false;
	}

	Face::Kind GetKind() override {
		K5::ksFaceDefinitionPtr face = Def();
		if (face->IsPlanar()) return Face::Kind::Plane;
		if (face->IsCylinder()) return Face::Kind::Cylinder;
		if (face->IsCone()) return Face::Kind::Cone;
		if (face->IsSphere()) return Face::Kind::Sphere;
		if (face->IsTorus()) return Face::Kind::Torus;
		return Face::Kind::Other;
	}

	double GetArea() override {
		return Def()->GetArea(0x1 /*ST_MIX_MM*/);
	}

	std::optional<Vertex::Point3D> GetNormal() override {
		K5::ksFaceDefinitionPtr face = Def();
		if (!face->IsPlanar()) return std::nullopt;
		K5::ksSurfacePtr surface = face->GetSurface();
		if (!surface) return std::nullopt;
		// У плоскости нормаль одна во всех точках: берём (0, 0) без запроса границ параметров
		Vertex::Point3D n;
		if (!surface->GetNormal(0.0, 0.0, &n.x, &n.y, &n.z)) {
			double u = (surface->GetParamUMin() + surface->GetParamUMax()) / 2;
			double v = (surface->GetParamVMin() + surface->GetParamVMax()) / 2;
			if (!surface->GetNormal(u, v, &n.x, &n.y, &n.z)) return std::nullopt;
		}
		// Нормаль поверхности не знает, с какой стороны материал: учитываем ориентацию грани
		if (!face->GetnormalOrientation()) n = {-n.x, -n.y, -n.z};
		double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		if (len < 1e-12) return std::nullopt;
		return Vertex::Point3D{n.x / len, n.y / len, n.z / len};
	}

	std::optional<Face::Sample> GetSample() override {
		K5::ksFaceDefinitionPtr face = Def();
		K5::ksSurfacePtr surface = face ? face->GetSurface() : nullptr;
		if (!surface) return std::nullopt;
		const double u = (surface->GetParamUMin() + surface->GetParamUMax()) / 2;
		const double v = (surface->GetParamVMin() + surface->GetParamVMax()) / 2;
		Face::Sample s;
		if (!surface->GetPoint(u, v, &s.point.x, &s.point.y, &s.point.z)) return std::nullopt;
		if (!surface->GetNormal(u, v, &s.normal.x, &s.normal.y, &s.normal.z)) return std::nullopt;
		if (!face->GetnormalOrientation()) s.normal = {-s.normal.x, -s.normal.y, -s.normal.z};
		const double len = std::sqrt(s.normal.x * s.normal.x + s.normal.y * s.normal.y + s.normal.z * s.normal.z);
		if (len < 1e-12) return std::nullopt;
		s.normal = {s.normal.x / len, s.normal.y / len, s.normal.z / len};
		return s;
	}

	std::optional<Box3D> GetBox() override {
		K5::ksFaceDefinitionPtr face = Def();
		std::optional<Box3D> box;
		if (K5::ksEdgeCollectionPtr edges = face->EdgeCollection()) {
			int count = edges->GetCount();
			for (int i = 0; i < count; ++i) {
				K5::ksEdgeDefinitionPtr edge = edges->GetByIndex(i);
				if (!edge) continue;
				K5::ksCurve3DPtr curve = edge->GetCurve3D();
				if (curve) Api7Geometry::Unite(box, Api7Geometry::CurveBox(curve));
			}
		}
		if (!box) {
			// Грань без рёбер (сфера): габарит поверхности
			if (K5::ksSurfacePtr surface = face->GetSurface()) {
				Box3D b;
				if (surface->GetGabarit(&b.min.x, &b.min.y, &b.min.z, &b.max.x, &b.max.y, &b.max.z)) box = b;
			}
		}
		return box;
	}

	std::string GetOwnerName() override {
		K5::ksEntityPtr owner = Def()->GetOwnerEntity();
		return owner ? BstrToUtf8(owner->name) : std::string();
	}

	std::optional<Face::Cylinder> GetCylinder() override {
		K5::ksFaceDefinitionPtr face = Def();
		if (!face->IsCylinder()) return std::nullopt;
		Face::Cylinder c{};
		double height = 0;
		if (!face->GetCylinderParam(&height, &c.radius)) return std::nullopt;
		K5::ksSurfacePtr surface = face->GetSurface();
		K5::ksCylinderParamPtr param;
		if (surface) param = surface->GetSurfaceParam();
		K5::ksPlacementPtr placement;
		if (param) placement = param->GetPlacement();
		if (placement) {
			placement->GetOrigin(&c.origin.x, &c.origin.y, &c.origin.z);
			placement->GetVector(2 /*ось Z*/, &c.axis.x, &c.axis.y, &c.axis.z);
		}
		return c;
	}

	std::vector<std::unique_ptr<Node::NodeImpl>> GetEdges() override;   // Edge.hpp
};
