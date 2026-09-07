#pragma once

#include "../../Include/Node/BaseExtrusion.h"
#include "../Node.hpp"

class BaseExtrusionApi7 : public NodeApi7, public BaseExtrusion::BaseExtrusionImpl {
public :
	BaseExtrusionApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetDepth1(double depth) override {
		K5::ksBaseExtrusionDefinitionPtr d = def;
		K5::ksExtrusionParamPtr param = d->ExtrusionParam();
		param->depthNormal = depth;
	}
	void SetDepth2(double depth) override {
		K5::ksBaseExtrusionDefinitionPtr d = def;
		K5::ksExtrusionParamPtr param = d->ExtrusionParam();
		param->depthReverse = depth;
	}
	void SetSketch(Sketch& sketch) override {
		K5::ksBaseExtrusionDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(sketch.node.get());
		if (node) {
			K5::ksEntityPtr sketchEntity = node->entity;
			d->SetSketch(sketchEntity);
		}
	}
};
