#pragma once

#include "../../Include/Node/Rotated.h"
#include "../Node.hpp"

// Общая реализация вращения для ksBaseRotatedDefinition, ksBossRotatedDefinition и
// ksCutRotatedDefinition: эскиз и углы через API5, ось — через API7 IRotated::Axis.
template <typename DefinitionPtr>
class RotatedApi7 {
protected:
	static void SetSketch(IDispatchPtr def, Sketch& sketch) {
		DefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(sketch.node.get());
		if (!d || !node || !node->entity) throw Kompas3DException("Не могу получить эскиз для вращения");
		K5::ksEntityPtr sketchEntity = node->entity;
		d->SetSketch(sketchEntity);
	}
	static void SetAxis(K5::ksEntityPtr entity, const Axis& axis) {
		NodeApi7* node = dynamic_cast<NodeApi7*>(axis.node.get());
		if (!node || !node->entity) throw Kompas3DException("Не могу получить ось вращения");
		K7::IRotatedPtr rotated = ToApi7<K7::IRotatedPtr>(entity);
		if (!rotated) throw Kompas3DException("Не могу получить IRotated");
		rotated->Axis = ToApi7<K7::IAxis3DPtr>(node->entity);
	}
	// direction (ExtrusionDirection), angle1/angle2 (градусы)
	static NodeParams GetParamMap(IDispatchPtr def) {
		DefinitionPtr d = def;
		K5::ksRotatedParamPtr p = d ? d->RotatedParam() : nullptr;
		if (!p) return {};
		return {{"direction", (double)p->direction}, {"angle1", p->angleNormal}, {"angle2", p->angleReverse}};
	}
	static void SetParamMap(IDispatchPtr def, const NodeParams& params) {
		DefinitionPtr d = def;
		K5::ksRotatedParamPtr p = d ? d->RotatedParam() : nullptr;
		if (!p) throw Kompas3DException("Не могу получить параметры вращения");
		for (const auto& [key, value] : params) {
			if (key == "direction") { p->direction = (short)value; d->directionType = (short)value; }
			else if (key == "angle1") p->angleNormal = value;
			else if (key == "angle2") p->angleReverse = value;
		}
	}
	static void SetParams(IDispatchPtr def, const RotatedParams& params) {
		DefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить параметры вращения");
		d->directionType = (short)params.direction;
		d->toroidShapeType = false;
		K5::ksRotatedParamPtr param = d->RotatedParam();
		if (!param) throw Kompas3DException("Не могу получить параметры вращения");
		param->direction = (short)params.direction;
		param->angleNormal = params.angle1;
		param->angleReverse = params.angle2;
		param->toroidShape = false;
	}
};

template <int Type, typename DefinitionPtr>
class RotatedNodeApi7 : public NodeApi7, public RotatedImpl, private RotatedApi7<DefinitionPtr> {
	using Helper = RotatedApi7<DefinitionPtr>;
public:
	RotatedNodeApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& sketch) override { Helper::SetSketch(def, sketch); }
	void SetAxis(const Axis& axis) override { Helper::SetAxis(entity, axis); }
	void SetParams(const RotatedParams& params) override { Helper::SetParams(def, params); }
	NodeParams GetParamMap() override { return Helper::GetParamMap(def); }
	void SetParamMap(const NodeParams& params) override { Helper::SetParamMap(def, params); }
};

using BaseRotatedApi7 = RotatedNodeApi7<27, K5::ksBaseRotatedDefinitionPtr>;
using BossRotatedApi7 = RotatedNodeApi7<28, K5::ksBossRotatedDefinitionPtr>;
