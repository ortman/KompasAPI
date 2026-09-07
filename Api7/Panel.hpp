#pragma once

#include "../Include/Panel.h"
#include "../Include/Kompas3D.h"

inline _variant_t ToVariantT(const PropertyVariant& boxVar) {
	return std::visit([](const auto& arg) -> _variant_t {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, std::string>) {
			return _variant_t(Kompas3D::Utf8ToCp1251(arg).c_str()); // Создаст VARIANT с типом VT_BSTR
		} else {
			return _variant_t(arg); // Для int (VT_I4), double (VT_R8) и т.д.
		}
	}, boxVar);
}

inline PropertyVariant FromVariantT(const _variant_t& var) {
	switch (var.vt) {
		case VT_I4:   return int(var);
		case VT_R8:   return double(var);
		case VT_BSTR: return Kompas3D::Cp1251ToUtf8(_bstr_t(var.bstrVal));
		case VT_I2:   return static_cast<int>(var);
		case VT_R4:   return static_cast<double>(var);
		case VT_EMPTY:
		case VT_NULL: return 0;
		default: throw Kompas3DException("Unsupported _variant_t type");
	}
}

class PropertyApi7 : public Panel::PropertyImpl {
private:
	K7::IPropertyControlPtr ctrl;

public:
	PropertyApi7(K7::IPropertyControlPtr c) : ctrl(c) {}

	void Create(const std::string& name, const std::optional<PropertyVariant>& val, int id) override {
		ctrl->Name = Kompas3D::Utf8ToCp1251(name).c_str();
		if (val.has_value()) ctrl->Value = ToVariantT(val.value());
		ctrl->Id = id;
	}
	void SetName(const std::string& name) override {
		ctrl->Name = Kompas3D::Utf8ToCp1251(name).c_str();
	}
	PropertyVariant GetValue() override {
		return FromVariantT(ctrl->Value);
	}
	void SetValue(const PropertyVariant& val) override {
		ctrl->Value = ToVariantT(val);
	}
	void Add(const PropertyVariant& val) override {
		K7::IPropertyListPtr list = ctrl;
		if (list) list->Add(ToVariantT(val));
	}
	void ClearList() override {
		K7::IPropertyListPtr list = ctrl;
		if (list) list->ClearList();
	}
	int Find(const PropertyVariant& val) override {
		K7::IPropertyListPtr list = ctrl;
		return list ? list->Find(ToVariantT(val)) : -1;
	}
};

class TabApi7 : public Panel::TabImpl {
private:
	K7::IPropertyTabPtr tab;

	K7::IPropertyControlsPtr Controls() {
		K7::IPropertyControlsPtr ctrls = tab ? tab->PropertyControls : nullptr;
		if (!ctrls) throw Kompas3DException("Can not get PropertyControls of Tab");
		return ctrls;
	}

public:
	TabApi7(K7::IPropertyTabPtr t) : tab(t) {}

	std::unique_ptr<Panel::PropertyImpl> AddProperty(int type) override {
		K7::IPropertyControlPtr c = Controls()->Add((KConst::ControlTypeEnum)type);
		if (!c) return nullptr;
		return std::make_unique<PropertyApi7>(c);
	}
	void Clear(int count) override {
		K7::IPropertyControlsPtr ctrls = Controls();
		for (int i = count - 1; i >= 0; --i) ctrls->Delete(i);
	}
};

class PropertyManagerNotifyLoc : public ComEvent {
private:
	Panel* panel;

public:
	PropertyManagerNotifyLoc(Panel* p) : ComEvent(K7::DIID_ksPropertyManagerNotify), panel(p) {}

	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
	                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
	                    EXCEPINFO* pExcepInfo, UINT* puArgErr) override {
		switch((int)dispIdMember) {
			case KConst::prButtonClick: {
				VARIANT& buttonId = pDispParams->rgvarg[pDispParams->cArgs - 1];
				if (panel && buttonId.vt == VT_I4) {
					VariantInit(pVarResult);
					pVarResult->vt = VT_BOOL;
					pVarResult->boolVal = panel->OnButtonClick(buttonId.lVal);
				}
				break;
			}
			case KConst::prControlCommand: {
				VARIANT& buttonId = pDispParams->rgvarg[pDispParams->cArgs - 2];
				if (panel && buttonId.vt == VT_I4) {
					panel->OnControlCommand(buttonId.lVal);
				}
				break;
			}
			case KConst::prChangeControlValue: {
				VARIANT& ctrl = pDispParams->rgvarg[pDispParams->cArgs - 1];
				if (panel && ctrl.vt == VT_DISPATCH) {
					K7::IPropertyControlPtr pCtrl = ctrl.pdispVal;
					if (pCtrl) panel->OnChangeControlValue(pCtrl->Id);
				}
				break;
			}
		}
		return S_OK;
	}
};

class PanelApi7 : public Panel::PanelImpl {
private:
	K7::IPropertyManagerPtr manager;
	PropertyManagerNotifyLoc* comEvent = nullptr;

public:
	~PanelApi7() override {
		if (comEvent) {
			if (manager) comEvent->Unsubscribe(manager);
			delete comEvent;
			comEvent = nullptr;
		}
	}

	bool Create(Panel* owner, const std::string& caption) override {
		if (!ComEvent::kompas7) return false;
		manager = ComEvent::kompas7->CreatePropertyManager(true);
		if (!manager) throw Kompas3DException("Can not create PropertyManager");
		if (!caption.empty()) manager->Caption = Kompas3D::Utf8ToCp1251(caption).c_str();
		manager->SpecToolbar = KConst::pnEnterEscHelp;
		comEvent = new PropertyManagerNotifyLoc(owner);
		comEvent->Subscribe(manager);
		return true;
	}

	std::unique_ptr<Panel::TabImpl> AddTab(const std::string& name) override {
		K7::IPropertyTabsPtr tabs = manager ? manager->PropertyTabs : nullptr;
		if (!tabs) throw Kompas3DException("Can not get PropertyTabs of PropertyManager");
		K7::IPropertyTabPtr tab = tabs->Add(Kompas3D::Utf8ToCp1251(name).c_str());
		if (!tab) return nullptr;
		return std::make_unique<TabApi7>(tab);
	}

	void Update() override {
		if (!manager) throw Kompas3DException("Can not get PropertyManager");
		//manager->UpdateTabs(); // not working
		manager->HideTabs();
		manager->ShowTabs();
	}

	void Show(bool isShow) override {
		if (!manager) throw Kompas3DException("Can not get PropertyManager");
		if (isShow) {
			manager->ShowTabs();
		} else {
			manager->HideTabs();
		}
	}
};
