#pragma once

#include "../../Include/Node/Features.h"
#include "../Node.hpp"
#include "Sketch.hpp"

namespace Api7Features {

inline K5::ksEntityPtr EntityOf(const Node& n, const char* what) {
	NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
	if (!n7 || !n7->entity) throw Kompas3DException(std::string("Не могу получить объект: ") + what);
	return n7->entity;
}

inline void Add(K5::ksEntityCollectionPtr collection, const Node& n, const char* what) {
	if (!collection) throw Kompas3DException(std::string("Не могу получить список объектов: ") + what);
	K5::ksEntityPtr entity = EntityOf(n, what);
	collection->Add(entity);
}

} // namespace Api7Features

class ShellApi7 : public NodeApi7, public Shell::ShellImpl {
	K5::ksShellDefinitionPtr Def() {
		K5::ksShellDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksShellDefinition");
		return d;
	}
public:
	ShellApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetThickness(double thickness) override { Def()->thickness = thickness; }
	// thinType: TRUE — стенка внутрь от исходных граней, FALSE — наружу
	void SetOutward(bool outward) override { Def()->thinType = outward ? VARIANT_FALSE : VARIANT_TRUE; }
	void AddFace(const Node& face) override { Api7Features::Add(Def()->FaceArray(), face, "грань оболочки"); }
	NodeParams GetParamMap() override { return {{"thickness", Def()->thickness}}; }
	void SetParamMap(const NodeParams& params) override {
		if (auto it = params.find("thickness"); it != params.end()) Def()->thickness = it->second;
	}
};

class MirrorCopyApi7 : public NodeApi7, public MirrorCopy::MirrorCopyImpl {
	K5::ksMirrorCopyDefinitionPtr Def() {
		K5::ksMirrorCopyDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksMirrorCopyDefinition");
		return d;
	}
public:
	MirrorCopyApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetPlane(const Node& plane) override { Def()->SetPlane(Api7Features::EntityOf(plane, "плоскость симметрии")); }
	void AddOperation(const Node& feature) override {
		Api7Features::Add(Def()->GetOperationArray(), feature, "копируемая операция");
	}
};

template <int Type, typename DefinitionPtr>
class EvolutionNodeApi7 : public NodeApi7, public EvolutionImpl {
	DefinitionPtr Def() {
		DefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить параметры кинематической операции");
		return d;
	}
public:
	EvolutionNodeApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& profile) override {
		DefinitionPtr d = Def();
		d->SetThinParam(false, 0, 0, 0);   // сплошное тело, не тонкая стенка
		d->SetSketch(Api7Features::EntityOf(profile, "эскиз сечения"));
	}
	void AddPath(const Node& path) override { Api7Features::Add(Def()->PathPartArray(), path, "траектория"); }
};

using BaseEvolutionApi7 = EvolutionNodeApi7<45, K5::ksBaseEvolutionDefinitionPtr>;
using BossEvolutionApi7 = EvolutionNodeApi7<46, K5::ksBossEvolutionDefinitionPtr>;

template <int Type, typename DefinitionPtr>
class LoftNodeApi7 : public NodeApi7, public LoftImpl {
	DefinitionPtr Def() {
		DefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить параметры операции по сечениям");
		return d;
	}
public:
	LoftNodeApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void AddSection(Sketch& section) override { Api7Features::Add(Def()->Sketchs(), section, "эскиз сечения"); }
	void SetClosed(bool closed) override {
		DefinitionPtr d = Def();
		d->SetThinParam(false, 0, 0, 0);
		d->SetLoftParam(closed, false, true);
		if constexpr (Type == 32) d->cut = true;
	}
};

using BaseLoftApi7 = LoftNodeApi7<30, K5::ksBaseLoftDefinitionPtr>;
using BossLoftApi7 = LoftNodeApi7<31, K5::ksBossLoftDefinitionPtr>;
using CutLoftApi7 = LoftNodeApi7<32, K5::ksCutLoftDefinitionPtr>;

class RibApi7 : public NodeApi7, public Rib::RibImpl {
	K5::ksRibDefinitionPtr Def() {
		K5::ksRibDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksRibDefinition");
		return d;
	}
public:
	RibApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& sketch) override { Def()->SetSketch(Api7Features::EntityOf(sketch, "эскиз ребра")); }
	void SetParams(double thickness, int side) override {
		K5::ksRibDefinitionPtr d = Def();
		d->index = 0;
		d->angle = 0;
		d->side = side;
		d->SetThinParam(2 /*в обе стороны*/, thickness / 2, thickness / 2);
	}
	NodeParams GetParamMap() override {
		short type = 0;
		double t1 = 0, t2 = 0;
		Def()->GetThinParam(&type, &t1, &t2);
		return {{"thickness", t1 + t2}};
	}
	void SetParamMap(const NodeParams& params) override {
		if (auto it = params.find("thickness"); it != params.end()) {
			Def()->SetThinParam(2, it->second / 2, it->second / 2);
		}
	}
};
