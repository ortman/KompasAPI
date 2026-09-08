#pragma once

#include "../Include/Process3D.h"
#include "../Include/Kompas3D.h"
#include "Node.hpp"
#include "Node/Face.hpp"
#include "Node/Edge.hpp"
#include "Part.hpp"
#include "Panel.hpp"

class MateConstraintApi7 : public MateConstraint::MateConstraintImpl {
private:
	K7::IMateConstraint3DPtr mate;

	// Сопрягаются объекты модели (грань, плоскость, ось), т.е. definition узла
	K7::IModelObjectPtr ToModelObject(const Node& node) {
		NodeApi7* n = dynamic_cast<NodeApi7*>(node.node.get());
		return n ? ToApi7<K7::IModelObjectPtr>(n->def) : nullptr;
	}

public:
	MateConstraintApi7(K7::IMateConstraint3DPtr m) : mate(m) {}

	void SetDir(MateDir dir) override {
		mate->Alignment = (KConst3D::ksMateConstraintAlignmentEnum)dir;
	}
	void SetFixed(MateFixed fixed) override {
		mate->Fixed = (KConst3D::ksMateFixedTypeEnum)fixed;
	}
	void SetValue(double value) override {
		mate->ParamValue = value;
	}
	void SetFirst(const Node& node) override {
		if (K7::IModelObjectPtr obj = ToModelObject(node)) mate->BaseObject1 = obj;
	}
	void SetSecond(const Node& node) override {
		if (K7::IModelObjectPtr obj = ToModelObject(node)) mate->BaseObject2 = obj;
	}
};

class Process3DNotifyLoc : public ComEvent {
private:
	KProcess3D* proc;

	// КОМПАС отдаёт объект модели API7; переводим его в узел (грань или ребро)
	static std::unique_ptr<Node::NodeImpl> ToNodeImpl(IDispatch* definition) {
		K7::IModelObjectPtr model = definition;
		if (!model) return nullptr;
		switch (model->ModelObjectType) {
			case KConst3D::o3d_face:
				if (K5::ksFaceDefinitionPtr face = ToApi5<K5::ksFaceDefinitionPtr>(definition)) {
					return std::make_unique<FaceApi7>(face->GetEntity(), face);
				}
				break;
			case KConst3D::o3d_edge:
				if (K5::ksEdgeDefinitionPtr edge = ToApi5<K5::ksEdgeDefinitionPtr>(definition)) {
					return std::make_unique<EdgeApi7>(edge->GetEntity(), edge);
				}
				break;
			default:
				break;
		}
		return nullptr;
	}

	bool CallHandler(DISPPARAMS* pDispParams, VARIANT* pVarResult, bool placement) {
		VARIANT& obj = pDispParams->rgvarg[pDispParams->cArgs - 1];
		if (obj.vt != VT_DISPATCH) return false;
		std::unique_ptr<Node::NodeImpl> impl = ToNodeImpl(obj.pdispVal);
		if (!impl) return false;
		Node node(std::move(impl));
		bool res = placement ? proc->OnPlacement(std::move(node)) : proc->OnFilter(std::move(node));
		VariantInit(pVarResult);
		pVarResult->vt = VT_BOOL;
		pVarResult->boolVal = res;
		return res;
	}

public:
	Process3DNotifyLoc(KProcess3D* p) : ComEvent(K7::DIID_ksProcess3DNotify), proc(p) {}

	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
	                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
	                    EXCEPINFO* pExcepInfo, UINT* puArgErr) override {
		switch ((int)dispIdMember) {
			case KConst::ksProcess3DFilterObjects:
				if (proc) CallHandler(pDispParams, pVarResult, false);
				break;
			case KConst::ksProcess3DPlacementChanged:
				if (proc) CallHandler(pDispParams, pVarResult, true);
				break;
		}
		return S_OK;
	}
};

class Process3DApi7 : public KProcess3D::Process3DImpl {
private:
	K5::ksDocument3DPtr doc;
	K7::IProcess3DPtr proc3D;
	K7::IProcessParamPtr procParam;
	Process3DNotifyLoc* procEvent = nullptr;
	PropertyManagerNotifyLoc* paramEvent = nullptr;

	K7::IProcessPtr Process() {
		K7::IProcessPtr proc = proc3D;
		if (!proc) throw Kompas3DException("Не могу получить базовый процесс от 3D");
		return proc;
	}

public:
	Process3DApi7(K5::ksDocument3DPtr d, K7::IProcess3DPtr p) : doc(d), proc3D(p) {}

	~Process3DApi7() override {
		if (paramEvent) {
			if (procParam) paramEvent->Unsubscribe(procParam);
			delete paramEvent;
		}
		if (procEvent) {
			if (proc3D) procEvent->Unsubscribe(proc3D);
			delete procEvent;
		}
		if (proc3D) {
			K7::IProcessPtr proc = proc3D;
			if (proc) proc->Stop();
		}
	}

	void SetOwner(KProcess3D* owner) override {
		procEvent = new Process3DNotifyLoc(owner);
		if (FAILED(procEvent->Subscribe(proc3D))) throw Kompas3DException("Не могу подписать события на процесс");
	}

	// Panel::PanelImpl
	bool Create(Panel* owner, const std::string& caption) override {
		if (!ComEvent::kompas7) return false;
		Process()->Dynamic = true;
		procParam = ComEvent::kompas7->CreateProcessParam();
		if (!procParam) throw Kompas3DException("Can not create ProcessParam");
		procParam->AutoReduce = false;
		procParam->SpecToolbar = KConst::pnEnterEscHelp;
		if (!caption.empty()) procParam->Caption = Kompas3D::Utf8ToCp1251(caption).c_str();
		paramEvent = new PropertyManagerNotifyLoc(owner);
		paramEvent->Subscribe(procParam);
		return true;
	}

	std::unique_ptr<Panel::TabImpl> AddTab(const std::string& name) override {
		K7::IPropertyTabsPtr tabs = procParam ? procParam->PropertyTabs : nullptr;
		if (!tabs) throw Kompas3DException("Can not get PropertyTabs of Process3D");
		K7::IPropertyTabPtr tab = tabs->Add(Kompas3D::Utf8ToCp1251(name).c_str());
		if (!tab) return nullptr;
		return std::make_unique<TabApi7>(tab);
	}

	// ProcessParam назначается процессу только после того, как в нём созданы все вкладки
	void Finish() override {
		if (procParam) Process()->ProcessParam = procParam;
	}

	void Update() override {
		Process()->Update();
	}

	void Show(bool isShow) override {} // Панель процесса показывается через Run/Stop

	// KProcess3D::Process3DImpl
	bool Run(bool modal, bool postMessage) override {
		return Process()->Run(modal, postMessage);
	}

	bool Stop() override {
		return Process()->Stop();
	}

	void SetPhantom(const Part& part) override {
		PartApi7* p7 = dynamic_cast<PartApi7*>(part.part.get());
		if (!p7 || !p7->part) return;
		K7::IPart7Ptr part7 = ToApi7<K7::IPart7Ptr>(p7->part);
		if (part7) {
			part7.AddRef(); //TODO: do remove?
			proc3D->PhantomObject = part7;
		}
	}

	Part GetPhantom() override {
		if (!proc3D) return Part();
		K7::IModelObjectPtr phModel = proc3D->PhantomObject;
		if (phModel && phModel->ModelObjectType == KConst3D::o3d_part) {
			phModel.AddRef(); // TODO:
			K5::ksPartPtr part = ToApi5<K5::ksPartPtr>(phModel);
			if (part) return Part(std::make_unique<PartApi7>(doc, part));
		}
		return Part();
	}

	void SetCaption(const std::string& caption) override {
		K7::IProcessPtr proc = Process();
		if (procParam) {
			procParam->Caption = Kompas3D::Utf8ToCp1251(caption).c_str();
		} else {
			proc->Caption = Kompas3D::Utf8ToCp1251(caption).c_str(); //TODO: unused!
		}
	}

	std::unique_ptr<MateConstraint::MateConstraintImpl>
	AddMateConstraint(MateType type, MateDir dir, MateFixed fixed, double value) override {
		if (!proc3D) return nullptr;
		K7::IMateConstraints3DPtr mates = proc3D->MateConstraints;
		if (!mates) throw Kompas3DException("Не могу получить MateConstraints процесса");
		K7::IMateConstraint3DPtr mate = mates->Add((KConst3D::MateConstraintType)type);
		if (!mate) return nullptr;
		mate->Alignment = (KConst3D::ksMateConstraintAlignmentEnum)dir;
		mate->Fixed = (KConst3D::ksMateFixedTypeEnum)fixed;
		mate->ParamValue = value;
		return std::make_unique<MateConstraintApi7>(mate);
	}
};
