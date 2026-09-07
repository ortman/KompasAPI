#pragma once

#include "../../Include/Node/CutExtrusion.h"
#include "../Node.hpp"

class CutExtrusionApi7 : public NodeApi7, public CutExtrusion::CutExtrusionImpl {
public :
	CutExtrusionApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetDepth1(double depth) override {
		K5::ksCutExtrusionDefinitionPtr d = def;
		K5::ksExtrusionParamPtr param = d->ExtrusionParam();
		param->depthNormal = depth;
	}
	void SetDepth2(double depth) override {
		K5::ksCutExtrusionDefinitionPtr d = def;
		K5::ksExtrusionParamPtr param = d->ExtrusionParam();
		param->depthReverse = depth;
	}
	void SetSketch(Sketch& sketch) override {
		K5::ksCutExtrusionDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(sketch.node.get());
		if (node) {
			K5::ksEntityPtr sketchEntity = node->entity;
			d->SetSketch(sketchEntity);
		}
	}
};
