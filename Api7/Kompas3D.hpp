#if !defined(_API7_KOMPAS_HPP_) && defined(KAPI7)
#define _API7_KOMPAS_HPP_

#include "ComKompas.h"
#include "../Include/Kompas3D.h"
#include "Doc3D.hpp"

#include <stdint.h>

//#include <ksConstants.h>
//#include <ksConstants3D.h>

//#ifdef _WIN32
//	#include <windows.h>
//#endif

//IUnknown* pKompas = nullptr;
//IUnknown* pKompas7 = nullptr;
//bool comInit = false;
//
//class KompasObjectNotifyLoc : public ComEvent {
//public:
//	KompasObjectNotifyLoc() : ComEvent(K5::DIID_ksKompasObjectNotify) {}
//	
//	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
//	                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
//	                    EXCEPINFO* pExcepInfo, UINT* puArgErr) override {
//		switch((int)dispIdMember) {
//			case KConst::koOpenDocument: {
//				VARIANT& varDoc = pDispParams->rgvarg[pDispParams->cArgs - 1];
//				VARIANT& varType = pDispParams->rgvarg[pDispParams->cArgs - 2];
//				if (varDoc.vt == VT_DISPATCH && varType.vt == VT_I4 && Kompas3D::WhenOpenDocument) {
//					VariantInit(pVarResult);
//					pVarResult->vt = VT_BOOL;
//					Doc3D doc(varDoc.pdispVal);
//					pVarResult->boolVal = Kompas3D::WhenOpenDocument(doc, varType.lVal);
//				}
//				break;
//			}
//			case KConst::koCreateDocument: {
//				VARIANT& varDoc = pDispParams->rgvarg[pDispParams->cArgs - 1];
//				VARIANT& varType = pDispParams->rgvarg[pDispParams->cArgs - 2];
//				if (varDoc.vt == VT_DISPATCH && varType.vt == VT_I4 && Kompas3D::WhenCreateDocument) {
//					VariantInit(pVarResult);
//					pVarResult->vt = VT_BOOL;
//					Doc3D doc(varDoc.pdispVal);
//					pVarResult->boolVal = Kompas3D::WhenCreateDocument(doc, varType.lVal);
//				}
//				break;
//			}
//		}
//		return S_OK;
//	}
//};
//
//KompasObjectNotifyLoc kompasNotify;
//
//bool Kompas3D::Connect(bool open, bool visible) {
//	if (pKompas && pKompas7) return true;
//	HRESULT hr;
//	if (!comInit) {
//		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
//		comInit = SUCCEEDED(hr);
//	}
//	if (!comInit) return false;
//	K5::KompasObjectPtr kompas = nullptr;
//	hr = kompas.GetActiveObject(L"KOMPAS.Application.5");
//	if (FAILED(hr)) {
//		if (open) {
//			hr = kompas.CreateInstance(L"KOMPAS.Application.5");
//		} else {
//			return false;
//		}
//	}
//	if (SUCCEEDED(hr)) {
//		kompas->Visible = visible;
//		pKompas = kompas.Detach();
//		K7::IApplicationPtr kompas7 = nullptr;
//		hr = kompas7.GetActiveObject(L"KOMPAS.Application.7");
//		if (SUCCEEDED(hr)) {
//			pKompas7 = kompas7.Detach();
//			kompasNotify.Subscribe(pKompas);
//			return true;
//		} else {
//			pKompas->Release();
//		}
//	}
//	return false;
//}
//
//void Kompas3D::Disconnect() {
//	if (pKompas) {
//		kompasNotify.Unsubscribe(pKompas);
//		pKompas->Release();
//	}
//	if (pKompas7) pKompas7->Release();
//	if (comInit) CoUninitialize();
//}

//Doc3D Kompas3D::GetActiveDocument3D() {
//	if (!Connect()) return nullptr;
//	K5::KompasObjectPtr kompas(pKompas);
//	K5::ksDocument3DPtr doc = kompas->ActiveDocument3D();
//	return Doc3D(doc ? doc.GetInterfacePtr() : nullptr);
//}

//Doc3D Kompas3D::Open3D(std::string path, bool visible) {
//	if (!Connect()) throw Kompas3DException("Нет подключения к Компас3D");
//	K5::KompasObjectPtr kompas(pKompas);
//	K5::ksDocument3DPtr doc = kompas->Document3D();
//	if (!doc->Open(Node::Utf8ToCp1251(path).c_str(), !visible)) {
//		throw Kompas3DException("Не могу открыть документ: " + path);
//	}
//	return Doc3D(doc.GetInterfacePtr());
//}
//
//template <typename T>
//T Kompas3D::GetParamStruct(int type) {
//	if (!Connect()) return nullptr;
//	K5::KompasObjectPtr kompas(pKompas);
//	return kompas->GetParamStruct(type);
//}
//
//template <typename T>
//T Kompas3D::ToApi7(IUnknown* k5) {
//	if (!Connect()) return nullptr;
//	K5::KompasObjectPtr kompas(pKompas);
//	return kompas->TransferInterface(k5, KConst::ksAPI7Dual, 0);
//}
//
//template <typename T>
//T Kompas3D::ToApi5(IUnknown* k7) {
//	if (!Connect()) return nullptr;
//	K5::KompasObjectPtr kompas(pKompas);
//	return kompas->TransferInterface(k7, KConst::ksAPI5Auto, 0);
//}
//
//IUnknown* Kompas3D::CreatePropertyManager() {
//	if (!Connect()) throw Kompas3DException("Kompas not connected");
//	K7::IApplicationPtr kompas7(pKompas7);
//	if (!pKompas7) throw Kompas3DException("Kompas not connected");
//	K7::IPropertyManagerPtr manager = kompas7->CreatePropertyManager(true);
//	if (!manager) throw Kompas3DException("Can not create PropertyManager");
//	manager.AddRef();
//	return manager.GetInterfacePtr();
//}
//
//IUnknown* Kompas3D::CreateProcessParam() {
//	if (!Connect()) throw Kompas3DException("Kompas not connected");
//	K7::IApplicationPtr kompas7(pKompas7);
//	if (!pKompas7) throw Kompas3DException("Kompas not connected");
//	K7::IProcessParamPtr param = kompas7->CreateProcessParam();
//	if (!param) throw Kompas3DException("Can not create ProcessParam");
//	param.AddRef();
//	return param.GetInterfacePtr();
//}
//
//void Kompas3D::Message(const std::string& txt) {
//	if (Connect()) {
//		K5::KompasObjectPtr kompas(pKompas);
//		kompas->ksMessage(Node::Utf8ToCp1251(txt).c_str());
//	}
//}
//
//void Kompas3D::Error(const std::string& txt) {
//	if (Connect()) {
//		K5::KompasObjectPtr kompas(pKompas);
//		kompas->ksError(Node::Utf8ToCp1251(txt).c_str());
//	}
//}
//
//std::string Kompas3D::SystemPath(long type) {
//	if (!Connect()) return std::string();
//	K5::KompasObjectPtr kompas(pKompas);
//	return Node::Cp1251ToUtf8(kompas->ksSystemPath(type));
//}

//KompasEvent<bool(Doc3D&, int)> Kompas3D::WhenCreateDocument;
//KompasEvent<bool(Doc3D&, int)> Kompas3D::WhenOpenDocument;
//
//#define KOMPAS_PARAM(p) template K5::p##Ptr Kompas3D::GetParamStruct(int);
//KOMPAS_PARAM(ksRectangleParam)
//KOMPAS_PARAM(ksRegularPolygonParam)
//KOMPAS_PARAM(ksEllipseParam)
//KOMPAS_PARAM(ksEllipseArcParam)
//KOMPAS_PARAM(ksEllipseArcParam1)
//KOMPAS_PARAM(ksUserParam)
//
//#define KOMPAS_API7(p) template K7::p##Ptr Kompas3D::ToApi7(IUnknown*);
//KOMPAS_API7(IEmbodimentsManager)
//KOMPAS_API7(IThread)
//KOMPAS_API7(IAxis3D)
//KOMPAS_API7(ICutRotated)
//KOMPAS_API7(IKompasDocument3D1)
//KOMPAS_API7(IPart7)
//KOMPAS_API7(IModelObject)
//
//#define KOMPAS_API5(p) template K5::p##Ptr Kompas3D::ToApi5(IUnknown*);
//KOMPAS_API5(ksPart)
//KOMPAS_API5(ksFaceDefinition)
//KOMPAS_API5(ksEdgeDefinition)

class K3D_API7_Kompas3D : public Kompas3D::K3D_Kompas3D {
private:
	K7::IApplicationPtr kompas7 = nullptr;
	K5::KompasObjectPtr kompas5 = nullptr;
	bool comInit = false;

public:
	K3D_API7_Kompas3D(IDispatch *k5) {
		kompas5 = k5;
		kompas5.AddRef();
	}
	
	K3D_API7_Kompas3D(bool open, bool visible) {
		HRESULT hr;
		if (!comInit) {
			hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			comInit = SUCCEEDED(hr);
		}
		if (!comInit) return;
		hr = kompas5.GetActiveObject(L"KOMPAS.Application.5");
		if (FAILED(hr)) {
			if (open) {
				hr = kompas5.CreateInstance(L"KOMPAS.Application.5");
			} else {
				kompas5 = nullptr;
				return;
			}
		}
		if (SUCCEEDED(hr)) {
			kompas5->Visible = visible;
			hr = kompas7.GetActiveObject(L"KOMPAS.Application.7");
			if (SUCCEEDED(hr)) {
				//kompasNotify.Subscribe(pKompas);
				return;
			}
		}
		kompas5 = nullptr;
		kompas7 = nullptr;
	}
	
	~K3D_API7_Kompas3D() {
		if (comInit) {
			//TODO: unsubscribe
			CoUninitialize();
		}
	}
	
	bool IsConnected() { return kompas5 && kompas7; }
	
	Doc3D GetActiveDocument3D() override {
		if (kompas5) {
			K5::ksDocument3DPtr doc = kompas5->ActiveDocument3D();
			if (doc) return Doc3D(std::move(std::make_unique<K3D_API7_Doc3D>(doc)));
		}
		return Doc3D();
	}
	
	void Message(const std::string& txt) override {
		if (kompas5) kompas5->ksMessage(Kompas3D::Utf8ToCp1251(txt).c_str());
	}
	
	void Error(const std::string& txt) override {
		if (kompas5) kompas5->ksError(Kompas3D::Utf8ToCp1251(txt).c_str());
	}
	
	std::string SystemPath(long type) override {
		return kompas5 ? Kompas3D::Cp1251ToUtf8(kompas5->ksSystemPath(type)) : std::string();
	}
};

#define DllExport extern "C" __declspec(dllexport)

DllExport void LIBRARYENTRY(unsigned int comm) {
	Kompas3D::RunCommand(comm);
}

DllExport int LibInterfaceNotifyEntry(IDispatch *application) {
	Kompas3D::SetKompas(std::move(std::make_unique<K3D_API7_Kompas3D>(application)));
	return 1;
}

bool Kompas3D::ComConnect(bool open, bool visible) {
	if (dynamic_cast<K3D_API7_Kompas3D*>(kompas.get())) return true;
	std::unique_ptr<K3D_API7_Kompas3D> comKompas = std::make_unique<K3D_API7_Kompas3D>(open, visible);
	bool isConnected = comKompas->IsConnected();
	if (isConnected) Kompas3D::SetKompas(std::move(comKompas));
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