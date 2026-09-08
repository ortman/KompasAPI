#if !defined(_API7_KOMPAS_HPP_) && defined(KAPI7)
#define _API7_KOMPAS_HPP_

#include "ComKompas.h"
#include "../Include/Kompas3D.h"
#include "Doc3D.hpp"
#include "Panel.hpp"

class KompasObjectNotifyLoc : public ComEvent {
public:
	KompasObjectNotifyLoc() : ComEvent(K5::DIID_ksKompasObjectNotify) {}
	
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
	                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
	                    EXCEPINFO* pExcepInfo, UINT* puArgErr) override {
		switch((int)dispIdMember) {
			case KConst::koOpenDocument: {
				VARIANT& varDoc = pDispParams->rgvarg[pDispParams->cArgs - 1];
				VARIANT& varType = pDispParams->rgvarg[pDispParams->cArgs - 2];
				if (varDoc.vt == VT_DISPATCH && varType.vt == VT_I4 && Kompas3D::WhenOpenDocument) {
					VariantInit(pVarResult);
					pVarResult->vt = VT_BOOL;
					Doc3D doc(std::make_unique<Doc3DApi7>(varDoc.pdispVal));
					pVarResult->boolVal = Kompas3D::WhenOpenDocument(doc, varType.lVal);
				}
				break;
			}
			case KConst::koCreateDocument: {
				VARIANT& varDoc = pDispParams->rgvarg[pDispParams->cArgs - 1];
				VARIANT& varType = pDispParams->rgvarg[pDispParams->cArgs - 2];
				if (varDoc.vt == VT_DISPATCH && varType.vt == VT_I4 && Kompas3D::WhenCreateDocument) {
					VariantInit(pVarResult);
					pVarResult->vt = VT_BOOL;
					Doc3D doc(std::make_unique<Doc3DApi7>(varDoc.pdispVal));
					pVarResult->boolVal = Kompas3D::WhenCreateDocument(doc, varType.lVal);
				}
				break;
			}
		}
		return S_OK;
	}
};

//IUnknown* Kompas3D::CreatePropertyManager() {
//	if (!Connect()) throw Kompas3DException("Kompas not connected");
//	K7::IApplicationPtr kompas7(pKompas7);
//	if (!pKompas7) throw Kompas3DException("Kompas not connected");
//	K7::IPropertyManagerPtr manager = kompas7->CreatePropertyManager(true);
//	if (!manager) throw Kompas3DException("Can not create PropertyManager");
//	manager.AddRef();
//	return manager.GetInterfacePtr();
//}

//IUnknown* Kompas3D::CreateProcessParam() {
//	if (!Connect()) throw Kompas3DException("Kompas not connected");
//	K7::IApplicationPtr kompas7(pKompas7);
//	if (!pKompas7) throw Kompas3DException("Kompas not connected");
//	K7::IProcessParamPtr param = kompas7->CreateProcessParam();
//	if (!param) throw Kompas3DException("Can not create ProcessParam");
//	param.AddRef();
//	return param.GetInterfacePtr();
//}

class Kompas3DApi7 : public Kompas3D::Kompas3DImpl {
private:
	bool comInit = false;
	KompasObjectNotifyLoc kompasNotify;

public:
	Kompas3DApi7(IDispatch *k5) {
		ComEvent::kompas5 = k5;
		ComEvent::kompas5.AddRef();
		ComEvent::kompas7 = ComEvent::kompas5->ksGetApplication7();
	}
	
	Kompas3DApi7(bool open, bool visible) {
		HRESULT hr;
		if (!comInit) {
			hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			comInit = SUCCEEDED(hr);
		}
		if (!comInit) return;
		hr = ComEvent::kompas5.GetActiveObject(L"KOMPAS.Application.5");
		if (FAILED(hr)) {
			if (open) {
				hr = ComEvent::kompas5.CreateInstance(L"KOMPAS.Application.5");
			} else {
				ComEvent::kompas5 = nullptr;
				return;
			}
		}
		if (SUCCEEDED(hr)) {
			ComEvent::kompas5->Visible = visible;
			hr = ComEvent::kompas7.GetActiveObject(L"KOMPAS.Application.7");
			if (SUCCEEDED(hr)) {
				kompasNotify.Subscribe(ComEvent::kompas5);
				return;
			}
		}
		ComEvent::kompas5 = nullptr;
		ComEvent::kompas7 = nullptr;
	}
	
	~Kompas3DApi7() {
		if (comInit) {
			//TODO: unsubscribe
			CoUninitialize();
		}
	}
	
	bool IsConnected() { return ComEvent::kompas5 && ComEvent::kompas7; }
	
	Doc3D GetActiveDocument3D() override {
		if (ComEvent::kompas5) {
			K5::ksDocument3DPtr doc = ComEvent::kompas5->ActiveDocument3D();
			if (doc) return Doc3D(std::make_unique<Doc3DApi7>(doc));
		}
		return Doc3D();
	}
	
	Doc3D Open3D(std::string path, bool visible) override {
		if (ComEvent::kompas5) {
			K5::ksDocument3DPtr doc = ComEvent::kompas5->Document3D();
			if (!doc) throw Kompas3DException("Не могу создать документ для: " + path);
			if (!doc->Open(Kompas3D::Utf8ToCp1251(path).c_str(), !visible)) {
				throw Kompas3DException("Не могу открыть документ: " + path);
			}
			return Doc3D(std::make_unique<Doc3DApi7>(doc));
		}
		return Doc3D();
	}
	
	void Message(const std::string& txt) override {
		if (ComEvent::kompas5) ComEvent::kompas5->ksMessage(Kompas3D::Utf8ToCp1251(txt).c_str());
	}
	
	void Error(const std::string& txt) override {
		if (ComEvent::kompas5) ComEvent::kompas5->ksError(Kompas3D::Utf8ToCp1251(txt).c_str());
	}
	
	std::string SystemPath(long type) override {
		return ComEvent::kompas5 ? Kompas3D::Cp1251ToUtf8(ComEvent::kompas5->ksSystemPath(type)) : std::string();
	}
	
	std::unique_ptr<Panel::PanelImpl> CreatePanel() override {
		if (!ComEvent::kompas7) return nullptr;
		return std::make_unique<PanelApi7>();
	}
};

#define DllExport extern "C" __declspec(dllexport)

DllExport void LIBRARYENTRY(unsigned int comm) {
	Kompas3D::RunCommand(comm);
}

DllExport int LibInterfaceNotifyEntry(IDispatch *application) {
	Kompas3D::SetKompas(std::move(std::make_unique<Kompas3DApi7>(application)));
	Kompas3D::WhenConnect();
	return 1;
}

bool Kompas3D::ComConnect(bool open, bool visible) {
	if (dynamic_cast<Kompas3DApi7*>(kompas.get())) return true;
	std::unique_ptr<Kompas3DApi7> comKompas = std::make_unique<Kompas3DApi7>(open, visible);
	bool isConnected = comKompas->IsConnected();
	if (isConnected) {
		Kompas3D::SetKompas(std::move(comKompas));
		Kompas3D::WhenConnect();
	}
	return isConnected;
}

void Kompas3D::ComDisconnect() {
//	if (pKompas) {
//		kompasNotify.Unsubscribe(pKompas);
//		pKompas->Release();
//	}
//	if (pKompas7) pKompas7->Release();
//	if (comInit) CoUninitialize();
}

#endif