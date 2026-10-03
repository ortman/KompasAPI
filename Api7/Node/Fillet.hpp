#pragma once

#include "../../Include/Node/Fillet.h"
#include "../Node.hpp"

inline void AddToCollection(K5::ksEntityCollectionPtr collection, const Node& object) {
	if (!collection) throw Kompas3DException("Не могу получить список объектов операции");
	NodeApi7* node = dynamic_cast<NodeApi7*>(object.node.get());
	if (!node || !node->entity) throw Kompas3DException("Объект для скругления/фаски недоступен");
	K5::ksEntityPtr entity = node->entity;
	collection->Add(entity);
}

class FilletApi7 : public NodeApi7, public Fillet::FilletImpl {
	K5::ksFilletDefinitionPtr Def() {
		K5::ksFilletDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksFilletDefinition");
		return d;
	}
public:
	FilletApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetRadius(double radius) override { Def()->radius = radius; }
	NodeParams GetParamMap() override { return {{"radius", Def()->radius}}; }
	void SetParamMap(const NodeParams& params) override {
		if (auto it = params.find("radius"); it != params.end()) Def()->radius = it->second;
	}
	void SetTangent(bool tangent) override { Def()->tangent = tangent; }
	void Add(const Node& object) override { AddToCollection(Def()->array(), object); }
};

class ChamferApi7 : public NodeApi7, public Chamfer::ChamferImpl {
	K5::ksChamferDefinitionPtr Def() {
		K5::ksChamferDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksChamferDefinition");
		return d;
	}
public:
	ChamferApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetParams(double distance1, double distance2, bool transfer) override {
		Def()->SetChamferParam(transfer, distance1, distance2);
	}
	NodeParams GetParamMap() override {
		VARIANT_BOOL transfer = VARIANT_FALSE;
		double d1 = 0, d2 = 0;
		Def()->GetChamferParam(&transfer, &d1, &d2);
		return {{"distance1", d1}, {"distance2", d2}, {"transfer", transfer ? 1.0 : 0.0}};
	}
	void SetParamMap(const NodeParams& params) override {
		NodeParams p = GetParamMap();
		for (const auto& [key, value] : params) p[key] = value;
		Def()->SetChamferParam(p["transfer"] != 0, p["distance1"], p["distance2"]);
	}
	void SetTangent(bool tangent) override { Def()->tangent = tangent; }
	void Add(const Node& object) override { AddToCollection(Def()->array(), object); }
};
