#pragma once

#include "../../Include/Node/Extrusion.h"
#include "../../Include/Node/Sketch.h"
#include "../Node.hpp"

// Общая реализация выдавливания: у ksBaseExtrusionDefinition, ksBossExtrusionDefinition
// и ksCutExtrusionDefinition одинаковые ExtrusionParam() и SetSketch().
template <typename DefinitionPtr>
class ExtrusionApi7 {
protected:
	static K5::ksExtrusionParamPtr Param(IDispatchPtr def) {
		DefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить параметры выдавливания");
		K5::ksExtrusionParamPtr param = d->ExtrusionParam();
		if (!param) throw Kompas3DException("Не могу получить параметры выдавливания");
		return param;
	}
	static void SetDepth1(IDispatchPtr def, double depth) {
		Param(def)->depthNormal = depth;
	}
	static void SetDepth2(IDispatchPtr def, double depth) {
		Param(def)->depthReverse = depth;
	}
	static void SetParams(IDispatchPtr def, const ExtrusionParams& params) {
		K5::ksExtrusionParamPtr param = Param(def);
		param->direction = (long)params.direction;
		param->typeNormal = (short)params.end1;
		param->depthNormal = params.depth1;
		param->typeReverse = (short)params.end2;
		param->depthReverse = params.depth2;
	}
	// direction (ExtrusionDirection), depth1/depth2 (мм), end1/end2 (ExtrusionEnd)
	static NodeParams GetParamMap(IDispatchPtr def) {
		K5::ksExtrusionParamPtr p = Param(def);
		return {{"direction", (double)p->direction}, {"depth1", p->depthNormal}, {"depth2", p->depthReverse},
		        {"end1", (double)p->typeNormal}, {"end2", (double)p->typeReverse}};
	}
	static void SetParamMap(IDispatchPtr def, const NodeParams& params) {
		K5::ksExtrusionParamPtr p = Param(def);
		for (const auto& [key, value] : params) {
			if (key == "direction") p->direction = (long)value;
			else if (key == "depth1") p->depthNormal = value;
			else if (key == "depth2") p->depthReverse = value;
			else if (key == "end1") p->typeNormal = (short)value;
			else if (key == "end2") p->typeReverse = (short)value;
		}
	}
	static void SetSketch(IDispatchPtr def, Sketch& sketch) {
		DefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(sketch.node.get());
		if (d && node) {
			K5::ksEntityPtr sketchEntity = node->entity;
			d->SetSketch(sketchEntity);
		}
	}
};
