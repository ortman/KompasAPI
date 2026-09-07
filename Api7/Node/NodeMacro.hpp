#pragma once

#include "../../Include/Node/NodeMacro.h"
#include "../Node.hpp"

class NodeMacroApi7 : public NodeApi7, public NodeMacro::NodeMacroImpl {
public :
	NodeMacroApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void Show(bool show) override {
		K5::ksMacro3DDefinitionPtr d = def;
		d->StaffVisible = show;
	}
	void Add(const Node& n) override {
		K5::ksMacro3DDefinitionPtr d = def;
		NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
		if (n7) {
			K5::ksEntityPtr nodeEntity = n7->entity;
			d->Add(nodeEntity);
		}
	}
	std::vector<Node> GetNodes() override {
		std::vector<Node> nodes;
		K5::ksMacro3DDefinitionPtr d = def;
		if (!d) return nodes;
		K5::ksFeatureCollectionPtr subFeatures = d->FeatureCollection();
		if (!subFeatures) return nodes;
		int count = subFeatures->GetCount();
		for (int i = 0; i < count; ++i) {
			K5::ksFeaturePtr feature = subFeatures->GetByIndex(i);
			if (K5::ksEntityPtr e = feature->GetObject()) {
				//nodes.push_back(Node(e));
			}
		}
		return nodes;
	}
	bool SetUserParam(void* param, size_t size, int cmd) override {
		K5::ksUserParamPtr pParam = ComEvent::kompas5->GetParamStruct(KConst::ko_UserParam);
		if (pParam) {
			pParam->Init();
			if (K5::ksMacro3DDefinitionPtr d = def) {
				pParam->libName  = "Kompas 3D Print";
				pParam->fileName = "Kompas3DPrint.rtw";
				pParam->number = cmd;
				
				SAFEARRAY* pSafeArray = SafeArrayCreateVector(VT_UI1, 0, (ULONG)size);
				if (pSafeArray) {
					// 2. Копируем данные БЕЗ SafeArrayAccessData (для одномерных массивов pvData доступен напрямую)
					if (pSafeArray->pvData) {
						memcpy(pSafeArray->pvData, param, size);
					}
					VARIANT var;
					var.vt = VT_ARRAY | VT_UI1;
					var.parray = pSafeArray;
					pParam->UserParams = _variant_t(var, false);
					return d->SetUserParam(pParam);
				}
			}
		}
		return false;
	}
	bool GetUserParam(void* param, size_t size) override {
		if (!param || size == 0) return false;
		if (K5::ksUserParamPtr pParam = ComEvent::kompas5->GetParamStruct(KConst::ko_UserParam)) {
			pParam->Init();
			if (K5::ksMacro3DDefinitionPtr d = def) {
				// Инициализируем структуру теми же библиотечными данными для поиска
				pParam->libName  = "Kompas 3D Print";
				pParam->fileName = "Kompas3DPrint.rtw";
				
				// Запрашиваем параметры у макрообъекта
				if (d->GetUserParam(pParam)) {
					_variant_t var = pParam->UserParams;
					
					// Проверяем, что вернулся массив байт
					if ((var.vt & VT_ARRAY) && (var.vt & VT_UI1) && var.parray) {
						SAFEARRAY* pSafeArray = var.parray;
						
						// Вычисляем размер данных в SafeArray
						long lBound, uBound;
						SafeArrayGetLBound(pSafeArray, 1, &lBound);
						SafeArrayGetUBound(pSafeArray, 1, &uBound);
						size_t actualSize = (uBound - lBound + 1);
						
						// Проверяем, достаточно ли места в целевом буфере
						if (actualSize <= size && pSafeArray->pvData) {
							memcpy(param, pSafeArray->pvData, actualSize);
							return true;
						}
					}
				}
			}
		}
		return false;
	}
};
