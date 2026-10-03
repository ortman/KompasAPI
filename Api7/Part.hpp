#pragma once

#include "../Include/Part.h"
#include <cmath>
#include "../Include/Kompas3D.h"
#include "Node.hpp"
#include "Node/Vertex.hpp"
#include "Node/Face.hpp"
#include "Node/Edge.hpp"
#include "Node/Axis.hpp"
#include "Node/Plane.hpp"
#include "Node/Sketch.hpp"
#include "Node/NodeMacro.hpp"
#include "Node/CutExtrusion.hpp"
#include "Node/CutRotated.hpp"
#include "Node/Rotated.hpp"
#include "Node/Fillet.hpp"
#include "Node/Features.hpp"
#include "Node/CutEvolution.hpp"
#include "Node/BaseExtrusion.hpp"
#include "Node/BossExtrusion.hpp"
#include "Node/MeshCopy.hpp"
#include "Node/CircularCopy.hpp"
#include "Node/CylindricSpiral.hpp"
#include "Node/ThreadDesignation.hpp"

class PartApi7 : public Part::PartImpl {
public:
	K5::ksDocument3DPtr doc;
	K5::ksPartPtr part;
	
	PartApi7(K5::ksDocument3DPtr d, K5::ksPartPtr p) : doc(d), part(p) {}
	
	std::string Name() {
		return BstrToUtf8(part->name);
	}
	
	std::unique_ptr<Node::NodeImpl> EntityToNode(K5::ksEntityPtr entity, int type) {
		switch (type) {
			case 5:  return std::make_unique<SketchApi7>(entity, nullptr);
			case 6:  return std::make_unique<FaceApi7>(entity, nullptr);
			case 7:  return std::make_unique<EdgeApi7>(entity, nullptr);
			case 8:  return std::make_unique<VertexApi7>(entity, nullptr);
			case 11: return std::make_unique<ConeAxisApi7>(entity, nullptr);
			case 14: return std::make_unique<OffsetPlaneApi7>(entity, nullptr);
			case 19: return std::make_unique<EdgePointPlaneApi7>(entity, nullptr);
			case 20: return std::make_unique<ParallelPlaneApi7>(entity, nullptr);
			case 24: return std::make_unique<BaseExtrusionApi7>(entity, nullptr);
			case 25: return std::make_unique<BossExtrusionApi7>(entity, nullptr);
			case 26: return std::make_unique<CutExtrusionApi7>(entity, nullptr);
			case 27: return std::make_unique<BaseRotatedApi7>(entity, nullptr);
			case 28: return std::make_unique<BossRotatedApi7>(entity, nullptr);
			case 29: return std::make_unique<CutRotatedApi7>(entity, nullptr);
			case 30: return std::make_unique<BaseLoftApi7>(entity, nullptr);
			case 31: return std::make_unique<BossLoftApi7>(entity, nullptr);
			case 32: return std::make_unique<CutLoftApi7>(entity, nullptr);
			case 33: return std::make_unique<ChamferApi7>(entity, nullptr);
			case 34: return std::make_unique<FilletApi7>(entity, nullptr);
			case 35: return std::make_unique<MeshCopyApi7>(entity, nullptr);
			case 36: return std::make_unique<CircularCopyApi7>(entity, nullptr);
			case 43: return std::make_unique<ShellApi7>(entity, nullptr);
			case 44: return std::make_unique<RibApi7>(entity, nullptr);
			case 45: return std::make_unique<BaseEvolutionApi7>(entity, nullptr);
			case 46: return std::make_unique<BossEvolutionApi7>(entity, nullptr);
			case 47: return std::make_unique<CutEvolutionApi7>(entity, nullptr);
			case 48: return std::make_unique<MirrorCopyApi7>(entity, nullptr);
			case 56: return std::make_unique<CylindricSpiralApi7>(entity, nullptr);
			case 58: return std::make_unique<ThreadDesignationApi7>(entity, nullptr);
			case 63: return std::make_unique<NodeMacroApi7>(entity, nullptr);
			default: return std::make_unique<NodeApi7>(entity, nullptr);
		}
	}
	
	std::unique_ptr<Node::NodeImpl> CreateImpl(int type) {
		K5::ksEntityPtr entity = part->NewEntity(type);
		if (!entity) return nullptr;
		return EntityToNode(entity, type);
	}

	Plane GetPlane(int type) {
		K5::ksEntityPtr entity = part->GetDefaultEntity(type);
		return Plane(std::make_unique<PlaneApi7>(entity, nullptr));
	}
	
	Axis GetAxis(int type) {
		K5::ksEntityPtr entity = part->GetDefaultEntity(type);
		return Axis(std::make_unique<AxisApi7>(entity, nullptr));
	}
	std::vector<Node> GetNodes() {
		std::vector<Node> nodes;
		if (!part) return nodes;
		K5::ksFeaturePtr topFeature = part->GetFeature();
		if (!topFeature) return nodes;
		K5::ksFeatureCollectionPtr subFeatures = topFeature->SubFeatureCollection(true, true);
		if (!subFeatures) return nodes;
		int count = subFeatures->GetCount();
		for (int i = 0; i < count; ++i) {
			K5::ksFeaturePtr feature = subFeatures->GetByIndex(i);
			K5::ksEntityPtr entity = feature->GetObject();
			if (entity) {
				nodes.push_back(Node(EntityToNode(entity, entity->type)));
			}
		}
		return nodes;
	}
	std::vector<Part::Variable> GetVariables(bool isExternal) {
		std::vector<Part::Variable> variables;
		if (!part) return variables;
		K5::ksVariableCollectionPtr vs = part->VariableCollection();
		if (!vs) return variables;
		int cnt = vs->GetCount();
		for (int i = 0; i < cnt; ++i) {
			K5::ksVariablePtr v = vs->GetByIndex(i);
			if (!v) continue;
			if (isExternal && !v->external) continue;
			variables.push_back({
				(bool)v->external,
				v->value,
				BstrToUtf8(v->name),
				Kompas3D::Cp1251ToUtf8(v->note)
			});
		}
		return variables;
	}
	std::optional<Part::BoundingBox> GetBoundingBox() override {
		if (!part) return std::nullopt;
		// У сборки своих тел нет — габарит по компонентам
		if (GetBodiesCount() == 0 && (!doc || doc->IsDetail())) return std::nullopt;
		Part::BoundingBox box;
		if (!part->GetGabarit(false, false, &box.x1, &box.y1, &box.z1, &box.x2, &box.y2, &box.z2)) return std::nullopt;
		if (box.x1 > box.x2 || box.y1 > box.y2 || box.z1 > box.z2) return std::nullopt;
		return box;
	}
	std::optional<Part::Measurement> Measure(const Node& a, const Node& b) override {
		NodeApi7* na = dynamic_cast<NodeApi7*>(a.node.get());
		NodeApi7* nb = dynamic_cast<NodeApi7*>(b.node.get());
		if (!part || !na || !nb || !na->entity || !nb->entity) return std::nullopt;
		K5::ksMeasurerPtr m = part->GetMeasurer();
		if (!m) return std::nullopt;
		K5::ksEntityPtr ea = na->entity, eb = nb->entity;
		if (!m->SetObject1(ea) || !m->SetObject2(eb) || !m->Calc()) return std::nullopt;
		Part::Measurement r{};
		r.distance = m->distance;
		if (m->IsAngleValid()) r.angle = m->angle;
		m->GetPoint1(&r.point1.x, &r.point1.y, &r.point1.z);
		m->GetPoint2(&r.point2.x, &r.point2.y, &r.point2.z);
		return r;
	}
	std::string GetFileName() override {
		return part ? Kompas3D::Cp1251ToUtf8(part->fileName) : std::string();
	}
	std::string GetDesignation() override {
		K7::IPart7Ptr part7 = part ? ToApi7<K7::IPart7Ptr>(part) : nullptr;
		return part7 ? BstrToUtf8(part7->Marking) : std::string();
	}
	bool SetTitle(const std::string& name, const std::string& designation) override {
		K7::IPart7Ptr part7 = part ? ToApi7<K7::IPart7Ptr>(part) : nullptr;
		if (!part7) return false;
		if (!name.empty()) part7->Name = Utf8ToBstr(name);
		if (!designation.empty()) part7->Marking = Utf8ToBstr(designation);
		part7->Update();
		return (name.empty() || BstrToUtf8(part7->Name) == name) && (designation.empty() || BstrToUtf8(part7->Marking) == designation);
	}
	bool IsFixed() override { return part && part->fixedComponent; }
	void SetFixed(bool fixed) override { if (part) part->fixedComponent = fixed; }
	// Положение компонента — через API7: ksPart::GetPlacement отдаёт положение при вставке,
	// а не после решения сопряжений
	K7::IPlacement3DPtr Placement7() {
		if (!part) return nullptr;
		K7::IPart7Ptr part7 = ToApi7<K7::IPart7Ptr>(part);
		if (!part7) return nullptr;
		return part7->Placement;
	}
	std::optional<Part::Placement> GetPlacement() override {
		K7::IPlacement3DPtr pl = Placement7();
		if (!pl) return std::nullopt;
		Part::Placement p{};
		pl->GetOrigin(&p.origin.x, &p.origin.y, &p.origin.z);
		pl->GetVector(KConst3D::o3d_axisOX, &p.x.x, &p.x.y, &p.x.z);
		pl->GetVector(KConst3D::o3d_axisOY, &p.y.x, &p.y.y, &p.y.z);
		pl->GetVector(KConst3D::o3d_axisOZ, &p.z.x, &p.z.y, &p.z.z);
		return p;
	}
	bool SetPlacement(const Part::Placement& p) override {
		// Запись — через API5 SetAxes (обе оси явно): IPlacement3D::SetVector переставляет оси
		if (!part) return false;
		K5::ksPlacementPtr pl = part->GetPlacement();
		if (!pl) return false;
		pl->SetOrigin(p.origin.x, p.origin.y, p.origin.z);
		pl->SetAxes(p.x.x, p.x.y, p.x.z, p.y.x, p.y.y, p.y.z);
		part->SetPlacement(pl);
		part->UpdatePlacement();
		// Код возврата UpdatePlacement ненадёжен: проверяем фактическое положение
		std::optional<Part::Placement> now = GetPlacement();
		if (!now) return false;
		auto same = [](double a, double b) { return std::abs(a - b) < 1e-6; };
		return same(now->origin.x, p.origin.x) && same(now->origin.y, p.origin.y) && same(now->origin.z, p.origin.z) &&
		       same(now->x.x, p.x.x) && same(now->x.y, p.x.y) && same(now->x.z, p.x.z) &&
		       same(now->y.x, p.y.x) && same(now->y.y, p.y.y) && same(now->y.z, p.y.z);
	}
	std::optional<Part::MassProperties> GetMassProperties() override {
		if (!part) return std::nullopt;
		// ST_MIX_MM | ST_MIX_KG: длины в мм, масса в кг
		K5::ksMassInertiaParamPtr mass = part->CalcMassInertiaProperties(0x1 | 0x10);
		if (!mass) return std::nullopt;
		return Part::MassProperties{
			mass->m, mass->v, mass->F,
			mass->xc, mass->yc, mass->zc,
			part->density,
			Kompas3D::Cp1251ToUtf8(part->material)
		};
	}
	int GetBodiesCount() override {
		if (!part) return 0;
		K5::ksBodyCollectionPtr bodies = part->BodyCollection();
		return bodies ? (int)bodies->GetCount() : 0;
	}
	K5::ksVariablePtr FindVariable(const std::string& name) {
		if (!part) return nullptr;
		K5::ksVariableCollectionPtr vs = part->VariableCollection();
		if (!vs) return nullptr;
		int cnt = vs->GetCount();
		for (int i = 0; i < cnt; ++i) {
			K5::ksVariablePtr v = vs->GetByIndex(i);
			if (v && BstrToUtf8(v->name) == name) return v;
		}
		return nullptr;
	}
	bool SetVariable(const std::string& name, double value) override {
		K5::ksVariablePtr v = FindVariable(name);
		if (!v) return false;
		v->value = value;
		return true;
	}
	bool SetVariableExpression(const std::string& name, const std::string& expression) override {
		K5::ksVariablePtr v = FindVariable(name);
		if (!v) return false;
		v->Expression = Utf8ToBstr(expression);
		return true;
	}
	bool Rebuild() override {
		return part && part->RebuildModel();
	}
	template <typename T, typename Impl>
	std::vector<T> Entities(short type) {
		std::vector<T> result;
		if (!part) return result;
		K5::ksEntityCollectionPtr entities = part->EntityCollection(type);
		int count = entities ? entities->GetCount() : 0;
		for (int i = 0; i < count; ++i) {
			K5::ksEntityPtr entity = entities->GetByIndex(i);
			if (entity) result.push_back(T(std::make_unique<Impl>(entity, nullptr)));
		}
		return result;
	}
	std::vector<Face> GetFaces() override {
		return Entities<Face, FaceApi7>(6 /*o3d_face*/);
	}
	std::vector<Edge> GetEdges() override {
		return Entities<Edge, EdgeApi7>(7 /*o3d_edge*/);
	}
	void Remove(const Node& n) {
		NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
		if (n7 && doc) {
			K5::ksEntityPtr entity = n7->entity;
			doc->DeleteObject(entity);
		}
	}
};

/*
 * #include "Kompas3D.h"
 * #include "Part.h"
 * 
 * Part::Part(IUnknown* d, IUnknown* p) : pDoc(d), pPart(p) {
 * 	if (pDoc) pDoc->AddRef();
 * 	if (pPart) pPart->AddRef();
 * }
 * 
 * Part::Part(const Part& part) { // Конструктор копирования
 * 	pPart = part.pPart;
 * 	if (pPart) pPart->AddRef();
 * 	pDoc = part.pDoc;
 * 	if (pDoc) pDoc->AddRef();
 * }
 * 
 * Part& Part::operator=(const Part& part) { // Оператор копирующего присваивания
 * 	pPart = part.pPart;
 * 	if (pPart) pPart->AddRef();
 * 	pDoc = part.pDoc;
 * 	if (pDoc) pDoc->AddRef();
 * 	return *this;
 * }
 * 
 * Part::Part(Part&& part) noexcept { // Конструктор перемещения
 * 	if (pPart) pPart->Release();
 * 	if (pDoc) pDoc->Release();
 * 	pPart = part.pPart;
 * 	part.pPart = nullptr;
 * 	pDoc = part.pDoc;
 * 	part.pDoc = nullptr;
 * }
 * 
 * Part& Part::operator=(Part&& part) noexcept { // Оператор перемещающего присваивания
 * 	if (pPart) pPart->Release();
 * 	if (pDoc) pDoc->Release();
 * 	pPart = part.pPart;
 * 	part.pPart = nullptr;
 * 	pDoc = part.pDoc;
 * 	part.pDoc = nullptr;
 * 	return *this;
 * }
 * 
 * Part::~Part() {
 * 	if (pPart) pPart->Release();
 * 	if (pDoc) pDoc->Release();
 * }
 * 
 * std::string Part::Name() {
 * 	K5::ksPartPtr part = pPart;
 * 	if (!part) throw Kompas3DException("Не могу получить объект Part");
 * 	return Node::Cp1251ToUtf8(part->name);
 * }
 * 
 * std::vector<Node> Part::GetNodes() {
 * 	std::vector<Node> nodes;
 * 	K5::ksPartPtr part = pPart;
 * 	if (!part) return nodes;
 * 	K5::ksFeaturePtr topFeature = part->GetFeature();
 * 	if (!topFeature) return nodes;
 * 	K5::ksFeatureCollectionPtr subFeatures = topFeature->SubFeatureCollection(true, true);
 * 	if (!subFeatures) return nodes;
 * 	int count = subFeatures->GetCount();
 * 	for (int i = 0; i < count; ++i) {
 * 		K5::ksFeaturePtr feature = subFeatures->GetByIndex(i);
 * 		K5::ksEntityPtr entity = feature->GetObject();
 * 		if (entity) {
 * 			//entity.AddRef();
 * 			nodes.push_back(Node(entity.GetInterfacePtr()));
 * 		}
 * 	}
 * 	return nodes;
 * }
 * 
 * IUnknown* Part::CreateEntity(int type) {
 * 	K5::ksPartPtr part = pPart;
 * 	if (!part) return nullptr;
 * 	K5::ksEntityPtr entity = part->NewEntity(type);
 * 	if (!entity) return nullptr;
 * 	entity->AddRef();
 * 	return entity;
 * }
 * 
 * std::vector<Part::Variable> Part::GetVariables(bool isExternal) {
 * 	std::vector<Part::Variable> variables;
 * 	K5::ksPartPtr part = pPart;
 * 	K5::ksVariableCollectionPtr vs = part->VariableCollection();
 * 	int cnt = vs->GetCount();
 * 	for (int i = 0; i < cnt; ++i) {
 * 		K5::ksVariablePtr v = vs->GetByIndex(i);
 * 		variables.push_back({
 * 			(bool)v->external,
 * 			v->value,
 * 			Node::Cp1251ToUtf8(v->name),
 * 			Node::Cp1251ToUtf8(v->note)
 * 		});
 * 	}
 * 	return variables;
 *
   }
*/