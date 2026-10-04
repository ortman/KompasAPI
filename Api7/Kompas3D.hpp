#if !defined(_API7_KOMPAS_HPP_) && defined(KAPI7)
#define _API7_KOMPAS_HPP_

#include "ComKompas.h"
#include <cctype>
#include "../Include/Kompas3D.h"
#include "Doc3D.hpp"
#include "Drawing.hpp"
#include "Specification.hpp"

// Сборка: компонент, чья грань видна в точке вида (IAssociationView::FindFace → IModelObject::Part)
inline std::vector<std::string> DrawingApi7::ComponentsAt(long number, const std::vector<std::array<double, 2>>& points, Doc3D& model3d) {
	std::vector<std::string> out(points.size());
	K7::IAssociationViewPtr assoc = FindView(number);
	auto* impl = dynamic_cast<Doc3DApi7*>(model3d.doc.get());
	K7::IKompasDocument3DPtr model = impl && impl->Document() ? ToApi7<K7::IKompasDocument3DPtr>(impl->Document()) : nullptr;
	if (!assoc || !model) return out;
	for (size_t k = 0; k < points.size(); ++k) {
		K7::IModelObjectPtr face = assoc->FindFace(points[k][0], points[k][1], model);
		if (!face) continue;
		K7::IPart7Ptr part = face->Part;
		if (part) out[k] = BstrToUtf8(part->FileName);
	}
	return out;
}
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

	// Отписаться от событий и отпустить КОМПАС: иначе закрытые документы, на которые
	// остались ссылки процесса, висят в Documents невидимыми до таймаута DCOM
	void Disconnect() {
		if (ComEvent::kompas5) kompasNotify.Unsubscribe(ComEvent::kompas5);
		ComEvent::kompas5 = nullptr;
		ComEvent::kompas7 = nullptr;
	}
	
	Doc3D GetActiveDocument3D() override {
		if (ComEvent::kompas5) {
			K5::ksDocument3DPtr doc = ComEvent::kompas5->ActiveDocument3D();
			if (doc) return Doc3D(std::make_unique<Doc3DApi7>(doc));
		}
		return Doc3D();
	}
	
	// Уже открытый документ с этим путём: активировать его вместо повторного открытия
	Doc3D ActivateOpened(const std::string& path) {
		if (!ComEvent::kompas7) return Doc3D();
		auto normalize = [](std::string p) {
			for (char& c : p) c = c == (char)92 ? '/' : (char)std::tolower((unsigned char)c);   // 92 — обратная косая
			return p;
		};
		const std::string wanted = normalize(path);
		K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
		long count = docs ? docs->Count : 0;
		for (long i = 0; i < count; ++i) {
			K7::IKompasDocumentPtr d = docs->GetItem(_variant_t(i));
			if (!d) continue;
			if (normalize(Kompas3D::Cp1251ToUtf8((const char*)d->PathName)) != wanted) continue;
			d->Active = VARIANT_TRUE;
			K5::ksDocument3DPtr doc = ComEvent::kompas5->ActiveDocument3D();
			if (doc) return Doc3D(std::make_unique<Doc3DApi7>(doc));
		}
		return Doc3D();
	}
	
	Doc3D Open3D(std::string path, bool visible) override {
		if (ComEvent::kompas5) {
			if (Doc3D opened = ActivateOpened(path)) return opened;
			K5::ksDocument3DPtr doc = ComEvent::kompas5->Document3D();
			if (!doc) throw Kompas3DException("Не могу создать документ для: " + path);
			if (!doc->Open(Kompas3D::Utf8ToCp1251(path).c_str(), !visible)) {
				throw Kompas3DException("Не могу открыть документ: " + path);
			}
			return Doc3D(std::make_unique<Doc3DApi7>(doc));
		}
		return Doc3D();
	}
	
	// Уже открытый документ с этим путём (без учёта регистра и вида косой черты)
	K7::IKompasDocumentPtr FindOpened(const std::string& path) {
		if (!ComEvent::kompas7) return nullptr;
		auto normalize = [](std::string p) {
			for (char& c : p) c = c == (char)92 ? '/' : (char)std::tolower((unsigned char)c);
			return p;
		};
		const std::string wanted = normalize(path);
		K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
		long count = docs ? docs->Count : 0;
		for (long i = 0; i < count; ++i) {
			K7::IKompasDocumentPtr d = docs->GetItem(_variant_t(i));
			if (d && normalize(BstrToUtf8(d->PathName)) == wanted) return d;
		}
		return nullptr;
	}

	Drawing NewDrawing(bool visible) override {
		if (!ComEvent::kompas7) return Drawing();
		K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
		K7::IKompasDocument2DPtr d = docs ? docs->Add(KConst::ksDocumentDrawing, visible ? VARIANT_TRUE : VARIANT_FALSE) : nullptr;
		if (!d) throw Kompas3DException("Не могу создать чертёж");
		return Drawing(std::make_unique<DrawingApi7>(d));
	}

	Specification NewSpecification(bool visible) override {
		if (!ComEvent::kompas7) return Specification();
		K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
		K7::IKompasDocumentPtr d = docs ? docs->Add(KConst::ksDocumentSpecification, visible ? VARIANT_TRUE : VARIANT_FALSE) : nullptr;
		if (!d) throw Kompas3DException("Не могу создать спецификацию");
		return Specification(std::make_unique<SpecificationApi7>(d));
	}

	Drawing GetActiveDrawing() override {
		if (!ComEvent::kompas7) return Drawing();
		K7::IKompasDocumentPtr d = ComEvent::kompas7->ActiveDocument;
		if (!d || d->DocumentType != KConst::ksDocumentDrawing) return Drawing();
		return Drawing(std::make_unique<DrawingApi7>(K7::IKompasDocument2DPtr(d)));
	}

	Drawing OpenDrawing(const std::string& path, bool visible) override {
		if (!ComEvent::kompas7) return Drawing();
		K7::IKompasDocumentPtr d = FindOpened(path);
		if (d) {
			d->Active = VARIANT_TRUE;
		} else {
			K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
			d = docs ? docs->Open(Utf8ToBstr(path), visible ? VARIANT_TRUE : VARIANT_FALSE, VARIANT_FALSE) : nullptr;
			if (!d) throw Kompas3DException("Не могу открыть чертёж: " + path);
		}
		if (d->DocumentType != KConst::ksDocumentDrawing) throw Kompas3DException("Файл не является чертежом: " + path);
		return Drawing(std::make_unique<DrawingApi7>(K7::IKompasDocument2DPtr(d)));
	}

	int CloseAll(bool save) override {
		if (!ComEvent::kompas7) return 0;
		K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
		int closed = 0;
		for (long i = docs ? docs->Count - 1 : -1; i >= 0; --i) {
			K7::IKompasDocumentPtr d = docs->GetItem(_variant_t(i));
			if (d && d->Close(save ? KConst::kdSaveChanges : KConst::kdDoNotSaveChanges)) ++closed;
		}
		return closed;
	}

	std::vector<std::pair<std::string, int>> ListDocuments() override {
		std::vector<std::pair<std::string, int>> out;
		if (!ComEvent::kompas7) return out;
		K7::IDocumentsPtr docs = ComEvent::kompas7->Documents;
		for (long i = 0; docs && i < docs->Count; ++i) {
			K7::IKompasDocumentPtr d = docs->GetItem(_variant_t(i));
			if (!d) continue;   // Count учитывает и уже закрытые документы — их элементы пусты
			std::string path = BstrToUtf8(d->PathName);
			out.emplace_back(path.empty() ? BstrToUtf8(d->Name) : path, (int)d->DocumentType);
		}
		return out;
	}

	int SetHideMessage(int mode) override {
		if (!ComEvent::kompas7) return 0;
		int old = (int)ComEvent::kompas7->HideMessage;
		ComEvent::kompas7->HideMessage = (KConst::ksHideMessageEnum)mode;
		return old;
	}

	int ActiveDocumentType() override {
		if (!ComEvent::kompas7) return 0;
		K7::IKompasDocumentPtr d = ComEvent::kompas7->ActiveDocument;
		return d ? (int)d->DocumentType : 0;
	}

	Doc3D New3D(bool assembly, bool visible) override {
		if (!ComEvent::kompas5) return Doc3D();
		K5::ksDocument3DPtr doc = ComEvent::kompas5->Document3D();
		if (!doc) throw Kompas3DException("Не могу создать документ-модель");
		if (!doc->Create(!visible, !assembly)) {
			throw Kompas3DException(assembly ? "Не могу создать сборку" : "Не могу создать деталь");
		}
		return Doc3D(std::make_unique<Doc3DApi7>(doc));
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
	if (Kompas3DApi7* api = dynamic_cast<Kompas3DApi7*>(kompas.get())) {
		api->Disconnect();
		kompas = std::make_unique<Kompas3DImpl>();
	}
//	if (pKompas) {
//		kompasNotify.Unsubscribe(pKompas);
//		pKompas->Release();
//	}
//	if (pKompas7) pKompas7->Release();
//	if (comInit) CoUninitialize();
}

#endif