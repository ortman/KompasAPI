#pragma once

#include "../../Include/Node/CutRotated.h"
#include "../Node.hpp"
#include "Axis.hpp"
#include "Sketch.hpp"

class CutRotatedApi7 : public NodeApi7, public CutRotated::CutRotatedImpl {
public :
	CutRotatedApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& sketch) override {
		K5::ksCutRotatedDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(sketch.node.get());
		if (node) {
			K5::ksEntityPtr sketchEntity = node->entity;
			if (!sketchEntity) throw Kompas3DException("Не могу получить Эскиз");
			d->SetSketch(sketchEntity);
		}
	}
	void SetAxis(const Axis& axis) override {
		NodeApi7* node = dynamic_cast<NodeApi7*>(axis.node.get());
		if (!node) return;
		K7::ICutRotatedPtr iRotated = ToApi7<K7::ICutRotatedPtr>(entity);
		if (!iRotated) throw Kompas3DException("Не могу получить ICutRotated");
		iRotated->Axis = ToApi7<K7::IAxis3DPtr>(node->entity);
	}
	void SetAngle(double angle) override {
		K5::ksCutRotatedDefinitionPtr d = def;
		d->cut = true;
		d->directionType = KConst3D::dtNormal;
		d->toroidShapeType = true;
		d->SetSideParam(true, angle);
		K5::ksRotatedParamPtr param = d->RotatedParam();
		param->direction = KConst3D::dtNormal;
		param->angleNormal = angle;
		param->angleReverse = 0.;
		param->toroidShape = true;
	}
};
