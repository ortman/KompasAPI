#pragma once

#include "../../Include/Node/Axis.h"
#include "../Node.hpp"
#include "Face.hpp"

class AxisApi7 : public NodeApi7, virtual public Axis::AxisImpl {
protected:
	K5::ksCurve3DPtr GetCurve3D() {
		if (K5::ksDefaultObjectPtr d = def) return d->GetCurve3D();
		if (K5::ksAxis2PlanesDefinitionPtr d = def) return d->GetCurve3D();
		if (K5::ksAxis2PointsDefinitionPtr d = def) return d->GetCurve3D();
		if (K5::ksAxisConefaceDefinitionPtr d = def) return d->GetCurve3D();
		if (K5::ksAxisEdgeDefinitionPtr d = def) return d->GetCurve3D();
		if (K5::ksAxisOperationsDefinitionPtr d = def) return d->GetCurve3D();
		return nullptr;
	}
	K5::ksLineSeg3dParamPtr GetLineSegParam() {
		K5::ksCurve3DPtr curve = GetCurve3D();
		if (!curve) return nullptr;
		return curve->GetCurveParam();
	}

public :
	AxisApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void Show(bool show) override {
		entity->hidden = !show;
	}
	Vertex::Point3D GetFirstPoint() override {
		K5::ksLineSeg3dParamPtr param = GetLineSegParam();
		if (!param) return {0., 0., 0.};
		Vertex::Point3D res;
		if (param->GetPointFirst(&res.x, &res.y, &res.z)) return res;
		return {0., 0., 0.};
	}
	Vertex::Point3D GetLastPoint() override {
		K5::ksLineSeg3dParamPtr param = GetLineSegParam();
		if (!param) return {0., 0., 0.};
		Vertex::Point3D res;
		if (param->GetPointLast(&res.x, &res.y, &res.z)) return res;
		return {0., 0., 0.};
	}
};

class ConeAxisApi7 : public AxisApi7, public ConeAxis::ConeAxisImpl {
public :
	ConeAxisApi7(K5::ksEntityPtr e, IDispatchPtr d) : AxisApi7(e, d) {}
	void SetFace(const Face& coneFace) override {
		K5::ksAxisConefaceDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(coneFace.node.get());
		if (node) {
			K5::ksEntityPtr faceEntity = node->entity;
			if (!faceEntity) throw Kompas3DException("Не могу получить коническую грань для Оси");
			d->SetFace(faceEntity);
		}
	}
};
