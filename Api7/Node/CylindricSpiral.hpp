#pragma once

#include "../../Include/Node/CylindricSpiral.h"
#include "../Node.hpp"
#include "Plane.hpp"
#include "Vertex.hpp"

class CylindricSpiralApi7 : public NodeApi7, public CylindricSpiral::CylindricSpiralImpl {
public :
	CylindricSpiralApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void Show(bool show) override {
		entity->hidden = !show;
	}
	void SetDiam(double diam) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->diam = diam;
	}
	void SetStep(double step) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->step = step;
	}
	void SetHeight(double height) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->height = height;
	}
	void TurnDir(bool dir) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->turnDir = dir;
	}
	void BuildDir(bool dir) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->buildDir = dir;
	}
	void BuildMode(CylindricSpiral::Mode mode) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->buildMode = mode;
	}
	void SetPlane(const Plane& plane) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(plane.node.get());
		if (node) {
			K5::ksEntityPtr planeEntity = node->entity;
			if (!planeEntity) throw Kompas3DException("Не могу получить Плоскость для Спирали");
			d->SetPlane(planeEntity);
		}
	}
	void SetLocation(double x, double y) override {
		K5::ksCylindricSpiralDefinitionPtr d = def;
		d->SetLocation(x, y);
	}
	std::unique_ptr<Node::NodeImpl> GetBeginVertex() override {
		K5::ksFeaturePtr feature = entity->GetFeature();
		if (!feature) throw Kompas3DException("Не могу получить Feature у CylindricSpiral");
		K5::ksEntityCollectionPtr vertexes = feature->EntityCollection(KConst3D::o3d_vertex);
		if (!vertexes) throw Kompas3DException("Не могу получить EntityCollection у CylindricSpiral");
		if (vertexes->GetCount() != 2) throw Kompas3DException("Количество vertexes у CylindricSpiral должно быть 2, но их " + std::to_string(vertexes->GetCount()));
		K5::ksEntityPtr v = vertexes->GetByIndex(1);
		if (!v) throw Kompas3DException("Не могу получить Vertex у CylindricSpiral");
		return std::make_unique<VertexApi7>(v, nullptr);
	}
};
