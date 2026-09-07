#pragma once

#include "../../Include/Node/CutEvolution.h"
#include "../Node.hpp"
#include "Sketch.hpp"

class CutEvolutionApi7 : public NodeApi7, public CutEvolution::CutEvolutionImpl {
public :
	CutEvolutionApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& sketch) override {
		K5::ksCutEvolutionDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(sketch.node.get());
		if (node) {
			K5::ksEntityPtr sketchEntity = node->entity;
			if (!sketchEntity) throw Kompas3DException("Не могу получить Эскиз");
			d->SetSketch(sketchEntity);
		}
	}
	void AddPath(const Node& n) override {
		K5::ksCutEvolutionDefinitionPtr d = def;
		K5::ksEntityCollectionPtr path = d->PathPartArray();
		if (!path) throw Kompas3DException("Не могу получить PathPartArray в CutEvolution");
		NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
		if (n7) {
			K5::ksEntityPtr nodeEntity = n7->entity;
			if (nodeEntity) path->Add(nodeEntity);
		}
	}
};
