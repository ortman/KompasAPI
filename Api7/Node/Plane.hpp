#pragma once

#include "../../Include/Node/Plane.h"
#include "../Node.hpp"

class PlaneApi7 : public NodeApi7, virtual public Plane::PlaneImpl {
protected:
	K5::ksSurfacePtr GetSurface() {
		if (K5::ksDefaultObjectPtr d = def) return d->GetSurface();
		if (K5::ksPlaneOffsetDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneAngleDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlane3PointsDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneNormalToSurfaceDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneTangentToSurfaceDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneEdgePointDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneParallelDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlanePerpendicularDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneLineToEdgeDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneLineToPlaneDefinitionPtr d = def) return d->GetSurface();
		if (K5::ksPlaneMiddleDefinitionPtr d = def) return d->GetSurface();
		return nullptr;
	}
	K5::ksPlacementPtr GetPlacement() {
		K5::ksSurfacePtr surface = GetSurface();
		if (!surface) return nullptr;
		K5::ksPlaneParamPtr param = surface->GetSurfaceParam();
		if (!param) return nullptr;
		return param->GetPlacement();
	}

public :
	PlaneApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void Show(bool show) override {
		entity->hidden = !show;
	}
	Vertex::Point3D GetVector(Plane::AxisType type) override {
		K5::ksPlacementPtr surfPl = GetPlacement();
		if (!surfPl) return {0., 0., 0.};
		Vertex::Point3D res;
		if (surfPl->GetVector((long)type, &res.x, &res.y, &res.z)) return res;
		return {0., 0., 0.};
	}
	Plane::Point2D Projection(const Vertex::Point3D& point) override {
		K5::ksPlacementPtr surfPl = GetPlacement();
		if (!surfPl) return {0., 0.};
		Plane::Point2D res;
		if (surfPl->PointProjection(point.x, point.y, point.z, &res.x, &res.y)) return res;
		return {0., 0.};
	}
	Vertex::Point3D Projection(const Plane::Point2D& point) override {
		K5::ksPlacementPtr surfPl = GetPlacement();
		if (!surfPl) return {0., 0., 0.};
		Vertex::Point3D res;
		if (surfPl->PointOn(point.x, point.y, &res.x, &res.y, &res.z)) return res;
		return {0., 0., 0.};
	}
};

class ParallelPlaneApi7 : public PlaneApi7, public ParallelPlane::ParallelPlaneImpl {
public :
	ParallelPlaneApi7(K5::ksEntityPtr e, IDispatchPtr d) : PlaneApi7(e, d) {}
	void SetPlane(const Face& planarFace) override {
		K5::ksPlaneParallelDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(planarFace.node.get());
		if (node) {
			K5::ksEntityPtr faceEntity = node->entity;
			if (!faceEntity) throw Kompas3DException("Не могу получить плоскую грань для Плоскости");
			d->SetPlane(faceEntity);
		}
	}
	void SetPoint(const Vertex& point) override {
		K5::ksPlaneParallelDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(point.node.get());
		if (node) d->SetPoint(node->def);
	}
};

class EdgePointPlaneApi7 : public PlaneApi7, public EdgePointPlane::EdgePointPlaneImpl {
public :
	EdgePointPlaneApi7(K5::ksEntityPtr e, IDispatchPtr d) : PlaneApi7(e, d) {}
	void SetEdge(const Axis& axis) override {
		K5::ksPlaneEdgePointDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(axis.node.get());
		if (node) {
			K5::ksEntityPtr axisEntity = node->entity;
			if (!axisEntity) throw Kompas3DException("Не могу получить ось для Плоскости");
			d->SetEdge(axisEntity);
		}
	}
	void SetPoint(const Vertex& point) override {
		K5::ksPlaneEdgePointDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(point.node.get());
		if (node) d->SetPoint(node->def);
	}
};
