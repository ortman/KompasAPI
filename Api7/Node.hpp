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

inline Variable ReadVariable(K7::IVariable7Ptr v) {
	Variable out;
	if (!v) return out;
	out.isExternal = v->External != VARIANT_FALSE;
	out.value = v->Value;
	out.name = BstrToUtf8(v->Name);
	out.comment = BstrToUtf8(v->Note);
	out.expression = BstrToUtf8(v->Expression);
	out.parameter = BstrToUtf8(v->ParameterNote);
	out.type = (Variable::Type)(int)v->VariableType;
	out.information = v->Information != VARIANT_FALSE;
	// «Исключить из расчёта» — служебный параметр с хвостовым пробелом
	while (!out.parameter.empty() && out.parameter.back() == ' ') out.parameter.pop_back();
	return out;
}

// Переменные объекта дерева (детали или операции)
inline std::vector<K7::IVariable7Ptr> FeatureVariables(K7::IFeature7Ptr feature) {
	std::vector<K7::IVariable7Ptr> list;
	if (!feature) return list;
	const long count = feature->GetVariablesCount(VARIANT_FALSE, VARIANT_FALSE);
	for (long i = 0; i < count; ++i) {
		if (K7::IVariable7Ptr v = feature->GetVariable(VARIANT_FALSE, VARIANT_FALSE, _variant_t(i))) list.push_back(v);
	}
	return list;
}

inline K7::IVariable7Ptr FindFeatureVariable(K7::IFeature7Ptr feature, const std::string& name) {
	for (K7::IVariable7Ptr v : FeatureVariables(feature)) {
		if (BstrToUtf8(v->Name) == name) return v;
	}
	return nullptr;
}

// Цвет в КОМПАСе — COLORREF (0x00BBGGRR), снаружи — 0xRRGGBB
inline unsigned long SwapRedBlue(unsigned long c) {
	return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
}

inline Appearance ReadAppearance(K7::IColorParam7Ptr cp) {
	Appearance a;
	a.color = SwapRedBlue((unsigned long)cp->Color);
	a.ambient = cp->Ambient;
	a.diffuse = cp->Diffuse;
	a.specularity = cp->Specularity;
	a.shininess = cp->Shininess;
	a.opacity = cp->Transparency;   // проверено на v24: 1 — непрозрачный, 0.2 — почти прозрачный
	a.emission = cp->Emission;
	const int use = (int)cp->UseColor;
	a.own = use == KConst3D::useColorOur || use == KConst3D::useColorUnknown;   // у детали — useColorUnknown
	return a;
}

inline void WriteAppearance(K7::IColorParam7Ptr cp, const Appearance& a) {
	cp->Color = (long)SwapRedBlue(a.color);
	cp->Ambient = a.ambient;
	cp->Diffuse = a.diffuse;
	cp->Specularity = a.specularity;
	cp->Shininess = a.shininess;
	cp->Transparency = a.opacity;
	cp->Emission = a.emission;
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
	std::vector<std::string> GetSubFeatureNames() const override {
		// IFeature7::SubFeatures в v23 пуст; эскиз операции отдаёт её определение — GetSketch есть у
		// выдавливания, выреза, вращения, ребра и др. Вызов по имени через IDispatch
		// У эскиза так же GetPlane отдаёт его плоскость
		std::vector<std::string> out;
		if (!def) return out;
		for (const wchar_t* name : {L"GetSketch", L"GetPlane"}) {
			OLECHAR* method = const_cast<OLECHAR*>(name);
			DISPID id = 0;
			if (FAILED(def->GetIDsOfNames(IID_NULL, &method, 1, LOCALE_USER_DEFAULT, &id))) continue;
			DISPPARAMS none{nullptr, nullptr, 0, 0};
			_variant_t result;
			if (FAILED(def->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &none, &result, nullptr, nullptr))) continue;
			if (result.vt == VT_DISPATCH && result.pdispVal) {
				if (K5::ksEntityPtr entity = result.pdispVal) out.push_back(BstrToUtf8(entity->name));
			}
		}
		return out;
	}
	K7::IFeature7Ptr Feature7() const {
		K7::IModelObjectPtr model = entity ? ToApi7<K7::IModelObjectPtr>(entity) : nullptr;
		return model ? model->Owner : nullptr;
	}
	std::vector<Variable> GetVariables() override {
		std::vector<Variable> list;
		for (K7::IVariable7Ptr v : FeatureVariables(Feature7())) list.push_back(ReadVariable(v));
		return list;
	}
	bool SetVariableExpression(const std::string& name, const std::string& expression) override {
		K7::IVariable7Ptr v = FindFeatureVariable(Feature7(), name);
		if (!v) return false;
		v->Expression = Utf8ToBstr(expression);
		return true;
	}
	std::optional<Appearance> GetAppearance() override {
		K7::IColorParam7Ptr cp = entity ? ToApi7<K7::IColorParam7Ptr>(entity) : nullptr;
		if (!cp) return std::nullopt;
		return ReadAppearance(cp);
	}
	bool SetAppearance(const std::optional<Appearance>& appearance) override {
		K7::IModelObjectPtr model = entity ? ToApi7<K7::IModelObjectPtr>(entity) : nullptr;
		K7::IColorParam7Ptr cp = model;
		if (!cp) return false;
		if (appearance) {
			cp->UseColor = KConst3D::useColorOur;
			WriteAppearance(cp, *appearance);
		} else {
			// Как у новых объектов: грань берёт цвет операции, операция — источника (детали)
			cp->UseColor = entity->type == 6 /*o3d_face*/ ? KConst3D::useColorOwner : KConst3D::useColorSource;
		}
		return model->Update() != VARIANT_FALSE;
	}
};