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

// Строки COM ↔ UTF-8 без потерь (пути и имена бывают не только в CP1251)
inline std::string BstrToUtf8(const _bstr_t& s) {
	const wchar_t* w = (const wchar_t*)s;
	if (!w || !*w) return std::string();
	int size = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
	std::string out(size > 0 ? size - 1 : 0, '\0');
	if (size > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), size, nullptr, nullptr);
	return out;
}

inline _bstr_t Utf8ToBstr(const std::string& s) {
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(size > 0 ? size - 1 : 0, L'\0');
	if (size > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), size);
	return _bstr_t(w.c_str());
}

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
		return BstrToUtf8(entity->name);   // имя — Unicode: «Ø», «×» не теряются
	}
	void SetName(const std::string& name) override {
		entity->name = Utf8ToBstr(name);
	}
	void Create() override {
		entity->Create();
	}
	void Update() override {
		entity->Update();
	}
	const void* GetIdentity() const override {
		// Правило COM: IUnknown одного объекта всегда один и тот же указатель
		IUnknownPtr unknown;
		if (entity) unknown = entity;
		else if (def) unknown = def;
		return unknown.GetInterfacePtr();
	}
	int GetError() const override {
		K5::ksFeaturePtr feature = entity->GetFeature();
		return feature ? (int)feature->objectError : 0;
	}
};