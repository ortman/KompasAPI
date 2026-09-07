#pragma once

#include "../Include/Part.h"
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
#include "Node/CutEvolution.hpp"
#include "Node/BaseExtrusion.hpp"
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
		return Kompas3D::Cp1251ToUtf8(part->name);
	}
	
	std::unique_ptr<Node::NodeImpl> CreateImpl(int type) {
		K5::ksEntityPtr entity = part->NewEntity(type);
		if (!entity) return nullptr;
		switch (type) {
			case 5:  return std::make_unique<SketchApi7>(entity, nullptr);
			case 6:  return std::make_unique<FaceApi7>(entity, nullptr);
			case 7:  return std::make_unique<EdgeApi7>(entity, nullptr);
			case 8:  return std::make_unique<VertexApi7>(entity, nullptr);
			case 11: return std::make_unique<ConeAxisApi7>(entity, nullptr);
			case 19: return std::make_unique<EdgePointPlaneApi7>(entity, nullptr);
			case 20: return std::make_unique<ParallelPlaneApi7>(entity, nullptr);
			case 24: return std::make_unique<BaseExtrusionApi7>(entity, nullptr);
			case 26: return std::make_unique<CutExtrusionApi7>(entity, nullptr);
			case 29: return std::make_unique<CutRotatedApi7>(entity, nullptr);
			case 35: return std::make_unique<MeshCopyApi7>(entity, nullptr);
			case 36: return std::make_unique<CircularCopyApi7>(entity, nullptr);
			case 47: return std::make_unique<CutEvolutionApi7>(entity, nullptr);
			case 56: return std::make_unique<CylindricSpiralApi7>(entity, nullptr);
			case 58: return std::make_unique<ThreadDesignationApi7>(entity, nullptr);
			case 63: return std::make_unique<NodeMacroApi7>(entity, nullptr);
			default:
				throw Kompas3DException("Создание неизвестного объекта " + std::to_string(type));
				//return std::make_unique<NodeApi7>(entity, nullptr);
		}
	}

	virtual Plane GetPlane(int type) {
		K5::ksEntityPtr entity = part->GetDefaultEntity(type);
		return Plane(std::make_unique<PlaneApi7>(entity, nullptr));
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
 * Part& Part::Remove(Node node) {
 * 	K5::ksDocument3DPtr doc = pDoc;
 * 	if (doc) {
 * 		K5::ksEntityPtr entity = node.pEntity;
 * 		doc->DeleteObject(entity);
 * 	}
 * 	return *this;
 * }
 * 
 * IUnknown* Part::GetDefaultEntity(int type) {
 * 	K5::ksPartPtr part = pPart;
 * 	if (!part) return nullptr;
 * 	K5::ksEntityPtr entity = part->GetDefaultEntity(type);
 * 	entity->AddRef();
 * 	return entity;
 * }
 * 
 * Plane Part::GetPlaneXOY() {
 * 	IUnknown* entity = GetDefaultEntity(KConst3D::o3d_planeXOY);
 * 	if (!entity) return nullptr;
 * 	return Plane(entity);
 * }
 * 
 * Plane Part::GetPlaneXOZ() {
 * 	IUnknown* entity = GetDefaultEntity(KConst3D::o3d_planeXOZ);
 * 	if (!entity) return nullptr;
 * 	return Plane(entity);
 * }
 * 
 * Plane Part::GetPlaneYOZ() {
 * 	IUnknown* entity = GetDefaultEntity(KConst3D::o3d_planeYOZ);
 * 	if (!entity) return nullptr;
 * 	return Plane(entity);
 * }
 * 
 * Axis Part::GetAxisOX() {
 * 	IUnknown* entity = GetDefaultEntity(KConst3D::o3d_axisOX);
 * 	if (!entity) return nullptr;
 * 	return Axis(entity);
 * }
 * 
 * Axis Part::GetAxisOY() {
 * 	IUnknown* entity = GetDefaultEntity(KConst3D::o3d_axisOY);
 * 	if (!entity) return nullptr;
 * 	return Axis(entity);
 * }
 * 
 * Axis Part::GetAxisOZ() {
 * 	IUnknown* entity = GetDefaultEntity(KConst3D::o3d_axisOZ);
 * 	if (!entity) return nullptr;
 * 	return Axis(entity);
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