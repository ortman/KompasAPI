#pragma once

#include "../../Include/Node/MeshCopy.h"
#include "../Node.hpp"
#include "Axis.hpp"

class MeshCopyApi7 : public NodeApi7, public MeshCopy::MeshCopyImpl {
public :
	MeshCopyApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetParam1(int count, double step) {
		K5::ksMeshCopyDefinitionPtr d = def;
		d->count1 = count;
		d->step1 = step;
	}
	void SetParam2(int count, double step) {
		K5::ksMeshCopyDefinitionPtr d = def;
		d->count2 = count;
		d->step2 = step;
	}
	NodeParams GetParamMap() override {
		K5::ksMeshCopyDefinitionPtr d = def;
		return {{"count1", (double)d->count1}, {"step1", d->step1}, {"count2", (double)d->count2}, {"step2", d->step2}};
	}
	void SetParamMap(const NodeParams& params) override {
		K5::ksMeshCopyDefinitionPtr d = def;
		for (const auto& [key, value] : params) {
			if (key == "count1") d->count1 = (long)value;
			else if (key == "step1") d->step1 = value;
			else if (key == "count2") d->count2 = (long)value;
			else if (key == "step2") d->step2 = value;
		}
	}
	void SetAxis1(const Axis& axis) override { SetAxis(axis, true); }
	void SetAxis2(const Axis& axis) override { SetAxis(axis, false); }
	void SetAxis(const Axis& axis, bool first) {
		K5::ksMeshCopyDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(axis.node.get());
		if (!d || !node || !node->entity) throw Kompas3DException("Не могу получить ось для массива");
		K5::ksEntityPtr axisEntity = node->entity;
		if (first) d->SetAxis1(axisEntity);
		else d->SetAxis2(axisEntity);
	}
	void AddNode(const Node& n) {
		K5::ksMeshCopyDefinitionPtr d = def;
		K5::ksEntityCollectionPtr operations = d->OperationArray();
		NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
		if (n7) {
			K5::ksEntityPtr nodeEntity = n7->entity;
			operations->Add(nodeEntity);
		}
	}
};