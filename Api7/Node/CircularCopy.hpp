#pragma once

#include "../../Include/Node/CircularCopy.h"
#include "../Node.hpp"
#include "Axis.hpp"

class CircularCopyApi7 : public NodeApi7, public CircularCopy::CircularCopyImpl {
public :
	CircularCopyApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetCircularParam(int count, double step, bool factor) override {
		K5::ksCircularCopyDefinitionPtr d = def;
		d->count2 = count;
		d->step2 = step;
		d->factor2 = factor;
	}
	void SetRadialParam(int count, double step, bool factor) override {
		K5::ksCircularCopyDefinitionPtr d = def;
		d->count1 = count;
		d->step1 = step;
		d->factor1 = factor;
	}
	void SetAxis(const Axis& axis) override {
		K5::ksCircularCopyDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(axis.node.get());
		if (node) {
			K5::ksEntityPtr axisEntity = node->entity;
			if (!axisEntity) throw Kompas3DException("Не могу получить Ось для CircularCopy");
			d->SetAxis(axisEntity);
		}
	}
	void AddNode(const Node& n) override {
		K5::ksCircularCopyDefinitionPtr d = def;
		K5::ksEntityCollectionPtr operations = d->GetOperationArray();
		if (!operations) throw Kompas3DException("Не могу получить OperationArray в CircularCopy");
		NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
		if (n7) {
			K5::ksEntityPtr nodeEntity = n7->entity;
			if (nodeEntity) operations->Add(nodeEntity);
		}
	}
};
