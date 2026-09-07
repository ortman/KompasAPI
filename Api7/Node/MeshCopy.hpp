#pragma once

#include "../../Include/Node/MeshCopy.h"
#include "../Node.hpp"

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