#pragma once

#include "../../Include/Node/BaseExtrusion.h"
#include "Extrusion.hpp"

class BaseExtrusionApi7 : public NodeApi7, public BaseExtrusion::BaseExtrusionImpl,
                          private ExtrusionApi7<K5::ksBaseExtrusionDefinitionPtr> {
public :
	BaseExtrusionApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetDepth1(double depth) override { ExtrusionApi7::SetDepth1(def, depth); }
	void SetDepth2(double depth) override { ExtrusionApi7::SetDepth2(def, depth); }
	void SetParams(const ExtrusionParams& params) override { ExtrusionApi7::SetParams(def, params); }
	void SetSketch(Sketch& sketch) override { ExtrusionApi7::SetSketch(def, sketch); }
	NodeParams GetParamMap() override { return ExtrusionApi7::GetParamMap(def); }
	void SetParamMap(const NodeParams& params) override { ExtrusionApi7::SetParamMap(def, params); }
};
