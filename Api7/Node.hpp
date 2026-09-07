#pragma once

#include "../Include/Node.h"
#include "../Include/Kompas3D.h"

/*
 * Node::Node(IUnknown* pE, IDispatch* pD) : pEntity(pE), pDefinition(pD) {
 * 	if (pEntity) pEntity->AddRef();
 * 	if (pDefinition) pDefinition->AddRef();
 * 	if (pEntity && !pDefinition) {
 * 		K5::ksEntityPtr entity = pEntity;
 * 		IDispatchPtr def = entity->GetDefinition();
 * 		def.AddRef();
 * 		pDefinition = def.GetInterfacePtr();
 * 	}
 * }
 * 
 * Node::~Node() {
 * 	if (pDefinition) pDefinition->Release();
 * 	if (pEntity) pEntity->Release();
 * }
 */

/*
 * int Node::GetType() const {
 * 	K5::ksEntityPtr entity = pEntity;
 * 	return entity ? entity->type : 0;
 * }
 * 
 * Node& Node::operator=(const Node& other) {
 * 	if (this == &other) return *this;
 * 	if (pDefinition) pDefinition->Release();
 * 	if (pEntity) pEntity->Release();
 * 
 * 	pEntity = other.pEntity;
 * 	if (pEntity) pEntity->AddRef();
 * 	pDefinition = other.pDefinition;
 * 	if (pDefinition) pDefinition->AddRef();
 * 	if (pEntity && !pDefinition) {
 * 		K5::ksEntityPtr entity = pEntity;
 * 		IDispatchPtr def = entity->GetDefinition();
 * 		def.AddRef();
 * 		pDefinition = def.GetInterfacePtr();
 * 	}
 * 	return *this;
 * }
 * 
 * 
 */
int Node::TYPE = KConst3D::o3d_unknown;

// Переход от интерфейса API5 к соответствующему дуальному интерфейсу API7
template <typename T>
inline T ToApi7(IUnknown* k5) {
	if (!k5 || !ComEvent::kompas5) return nullptr;
	return ComEvent::kompas5->TransferInterface(k5, KConst::ksAPI7Dual, 0);
}

// Переход от дуального интерфейса API7 к соответствующему интерфейсу API5
template <typename T>
inline T ToApi5(IUnknown* k7) {
	if (!k7 || !ComEvent::kompas5) return nullptr;
	return ComEvent::kompas5->TransferInterface(k7, KConst::ksAPI5Auto, 0);
}

class NodeApi7 : virtual public Node::NodeImpl {
public:
	K5::ksEntityPtr entity;
	IDispatchPtr def;
	NodeApi7(K5::ksEntityPtr e, IDispatchPtr definition) : entity(e), def(definition) {
		if (entity && !def) {
			def = entity->GetDefinition();
		}
	}
	int GetType() const override {
		return entity->type;
	}
	std::string GetName() const override {
		return Kompas3D::Cp1251ToUtf8(entity->name);
	}
	void SetName(const std::string& name) override {
		entity->name = Kompas3D::Utf8ToCp1251(name).c_str();
	}
	void Create() override {
		entity->Create();
	}
	void Update() override {
		entity->Update();
	}
};