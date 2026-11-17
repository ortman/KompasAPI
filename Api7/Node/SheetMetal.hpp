#pragma once

#include "../../Include/Node/SheetMetal.h"
#include "Surfaces.hpp"

// Листовые операции создаются через ISheetMetalContainer API7; после Create() — сущность API5 из
// дерева (ModelObjectApi7). Прочитанные из дерева (EntityToNode) берут объект API7 у сущности
class SheetMetalApi7 : public ModelObjectApi7, public SheetMetalImpl {
	template <typename T>
	T As(const char* what) const {
		T p = object;
		if (!p) throw Kompas3DException(std::string("Не могу получить интерфейс листовой операции: ") + what);
		return p;
	}
	static K7::IModelObjectPtr SketchObject(Sketch& sketch) {
		K7::IModelObjectPtr s = ToApi7<K7::IModelObjectPtr>(Api7Features::EntityOf(sketch, "эскиз листовой операции"));
		if (!s) throw Kompas3DException("Не могу получить эскиз API7 листовой операции");
		return s;
	}

	// Линия сгиба по линии — не эскиз, а его отрезок как ребро модели (проверено на v24: эскиз
	// КОМПАС не принимает, Line остаётся пустым)
	static K7::IModelObjectPtr SketchLine(Sketch& sketch) {
		K7::IFeature7Ptr feature = SketchObject(sketch);
		if (!feature) throw Kompas3DException("Не могу получить эскиз линии сгиба");
		_variant_t edges = feature->GetModelObjects(_variant_t((long)7 /* o3d_edge */));
		K7::IModelObjectPtr line;
		if (edges.vt == VT_DISPATCH) {
			line = edges.pdispVal;
		} else if ((edges.vt & VT_ARRAY) && edges.parray) {
			long lo = 0, hi = -1;
			SafeArrayGetLBound(edges.parray, 1, &lo);
			SafeArrayGetUBound(edges.parray, 1, &hi);
			if (hi - lo != 0) throw Kompas3DException("В эскизе линии сгиба должен быть один отрезок");
			if ((edges.vt & VT_TYPEMASK) == VT_VARIANT) {
				_variant_t item;
				if (SUCCEEDED(SafeArrayGetElement(edges.parray, &lo, &item)) && item.vt == VT_DISPATCH) line = item.pdispVal;
			} else {
				IDispatch* item = nullptr;
				if (SUCCEEDED(SafeArrayGetElement(edges.parray, &lo, &item)) && item) {
					line = item;
					item->Release();
				}
			}
		}
		if (!line) throw Kompas3DException("В эскизе линии сгиба нет отрезка");
		return line;
	}

	// Расчёт развёртки — одинаковые свойства у тела и сгиба
	template <typename T>
	static void WriteAllowance(T p, const BendAllowance& a) {
		p->UnfoldType = (KConst3D::ksUnfoldTypeEnum)a.kind;
		if (a.kind == BendAllowance::Kind::Coefficient) p->BendCoefficient = a.value;
		if (a.kind == BendAllowance::Kind::Value) p->BendValue = a.value;
		if (a.kind == BendAllowance::Kind::Reduction) p->BendReduction = a.value;
	}
	template <typename T>
	static void ReadAllowance(T p, NodeParams& out) {
		out["unfold_type"] = (double)(int)p->UnfoldType;
		out["k_factor"] = p->BendCoefficient;
		out["bend_value"] = p->BendValue;
		out["bend_reduction"] = p->BendReduction;
	}
	template <typename T>
	static void ApplyAllowance(T p, const NodeParams& in) {
		auto it = in.find("unfold_type");
		if (it == in.end()) return;
		const int kind = (int)it->second;
		p->UnfoldType = (KConst3D::ksUnfoldTypeEnum)kind;
		if (kind == 0 && in.count("k_factor")) p->BendCoefficient = in.at("k_factor");
		if (kind == 1 && in.count("bend_value")) p->BendValue = in.at("bend_value");
		if (kind == 2 && in.count("bend_reduction")) p->BendReduction = in.at("bend_reduction");
	}

public:
	using ModelObjectApi7::ModelObjectApi7;
	// Операция из дерева модели
	SheetMetalApi7(K5::ksEntityPtr e) : ModelObjectApi7(nullptr, ToApi7<K7::IModelObjectPtr>(e), e ? (int)e->type : 0) {
		entity = e;
		def = e->GetDefinition();
	}

	void SetSketch(Sketch& sketch) override {
		switch (type) {
			case 74: As<K7::ISheetMetalBodyPtr>("тело")->Sketch = K7::ISketchPtr(SketchObject(sketch)); break;
			case 78: As<K7::ISheetMetalCutPtr>("вырез")->Sketch = K7::ISketchPtr(SketchObject(sketch)); break;
			case 76: As<K7::ISheetMetalLineBendPtr>("сгиб по линии")->Line = SketchLine(sketch); break;
			case 534: As<K7::ISheetMetalPlatePtr>("пластина")->Sketch = SketchObject(sketch); break;
			case 541:
			case 542: As<K7::ISheetMetalPressFormingPtr>("штамповка")->Sketch = SketchObject(sketch); break;
			case 543: As<K7::ISheetMetalShoulderPtr>("буртик")->Sketch = SketchObject(sketch); break;
			case 544: As<K7::ISheetMetalJalousiePtr>("жалюзи")->Sketch = SketchObject(sketch); break;
			default: throw Kompas3DException("Эта листовая операция не строится по эскизу");
		}
	}

	void SetObjects(const std::vector<const Node*>& objects) override {
		if (type == 75) {
			K7::ISheetMetalBendPtr b = As<K7::ISheetMetalBendPtr>("сгиб");
			if (objects.size() == 1) b->BendObject = Api7Surfaces::Model7(*objects.front(), "ребро сгиба");
			else b->BendObjects = Api7Surfaces::Objects(objects, "рёбра сгиба");
		} else if (type == 76) {
			As<K7::ISheetMetalLineBendPtr>("сгиб по линии")->Faces = Api7Surfaces::Objects(objects, "грань сгиба по линии");
		} else if (type == 539) {
			K7::ISheetMetalClosedCornerPtr c = As<K7::ISheetMetalClosedCornerPtr>("замыкание углов");
			for (const Node* n : objects) {
				if (!c->AddCornerObject(Api7Surfaces::Model7(*n, "угол"))) {
					throw Kompas3DException("КОМПАС не принял грань угла: нужен плоский торец зоны сгиба на углу");
				}
			}
		} else {
			throw Kompas3DException("Эта листовая операция не строится по объектам модели");
		}
	}

	void SetParams(const SheetMetalParams& p) override {
		switch (type) {
			case 74: {
				K7::ISheetMetalBodyPtr b = As<K7::ISheetMetalBodyPtr>("тело");
				if (p.thickness > 0) b->Thickness = p.thickness;
				if (p.radius > 0) b->Radius = p.radius;
				b->ThicknessDirection = p.reverse ? VARIANT_FALSE : VARIANT_TRUE;
				if (p.depth > 0) {
					b->Direction = (KConst3D::ksDirectionTypeEnum)p.direction;
					b->SetSideParameters(VARIANT_TRUE, KConst3D::etBlind, p.depth, nullptr);
				}
				if (p.allowance) WriteAllowance(b, *p.allowance);
				break;
			}
			case 75: {
				K7::ISheetMetalBendPtr b = As<K7::ISheetMetalBendPtr>("сгиб");
				b->Length = p.length;
				b->Angle = p.angle;
				if (p.radius > 0) b->Radius = p.radius;
				b->Direction = p.reverse ? VARIANT_FALSE : VARIANT_TRUE;
				if (p.placement >= 0) b->OffsetType = (KConst3D::ksBendOffsetTypeEnum)p.placement;
				if (p.lengthType >= 0) b->LengthType = (KConst3D::ksBendLengthTypeEnum)p.lengthType;
				if (p.left.taper) {
					b->LeftSideType = KConst3D::ksBendSideByAngle;
					b->DeviationLeftSide = *p.left.taper;
				} else if (p.left.widen) {
					b->LeftSideType = KConst3D::ksBendSideByWidening;
					b->WideningLeftSide = *p.left.widen;
				}
				if (p.right.taper) {
					b->RightSideType = KConst3D::ksBendSideByAngle;
					b->DeviationRightSide = *p.right.taper;
				} else if (p.right.widen) {
					b->RightSideType = KConst3D::ksBendSideByWidening;
					b->WideningRightSide = *p.right.widen;
				}
				if (p.allowance) WriteAllowance(b, *p.allowance);
				break;
			}
			case 76: {
				K7::ISheetMetalLineBendPtr b = As<K7::ISheetMetalLineBendPtr>("сгиб по линии");
				b->Angle = p.angle;
				if (p.radius > 0) b->Radius = p.radius;
				b->Direction = p.reverse ? VARIANT_FALSE : VARIANT_TRUE;
				b->BendLeftSideFixed = p.fixOtherSide ? VARIANT_FALSE : VARIANT_TRUE;
				if (p.bendType >= 0) b->BendType = (KConst3D::ksBendTypeEnum)p.bendType;
				if (p.allowance) WriteAllowance(b, *p.allowance);
				break;
			}
			case 534: break;
			case 78: As<K7::ISheetMetalCutPtr>("вырез")->Cut = VARIANT_TRUE; break;
			case 539: {
				K7::ISheetMetalClosedCornerPtr c = As<K7::ISheetMetalClosedCornerPtr>("замыкание углов");
				for (long k = 0, n = c->CornersCount; k < n; ++k) {
					c->PutClosingClosedType(k, (KConst3D::ksClosingClosedTypeEnum)p.closing);
					c->PutClosingGapValue(k, p.gap);
				}
				break;
			}
			case 541:
			case 542: {
				K7::ISheetMetalPressFormingPtr f = As<K7::ISheetMetalPressFormingPtr>("штамповка");
				f->Height = p.height;
				f->Direction = p.reverse ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			}
			case 543: {
				K7::ISheetMetalShoulderPtr s = As<K7::ISheetMetalShoulderPtr>("буртик");
				if (p.height > 0) s->Height = p.height;
				if (p.width > 0) s->Width1 = p.width;
				s->Direction = p.reverse ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			}
			case 544: {
				K7::ISheetMetalJalousiePtr j = As<K7::ISheetMetalJalousiePtr>("жалюзи");
				j->Height = p.height;
				j->Width = p.width;
				j->Direction = p.reverse ? VARIANT_FALSE : VARIANT_TRUE;
				break;
			}
		}
	}

	// Эскиз листовой операции: определение API5 его не отдаёт, берём у объекта API7
	std::vector<std::string> GetSubFeatureNames() const override {
		std::vector<std::string> out = ModelObjectApi7::GetSubFeatureNames();
		K7::IModelObjectPtr sketch;
		try {
			switch (type) {
				case 74: sketch = As<K7::ISheetMetalBodyPtr>("тело")->Sketch; break;
				case 78: sketch = As<K7::ISheetMetalCutPtr>("вырез")->Sketch; break;
				case 76: {
					K7::IModelObjectPtr line = As<K7::ISheetMetalLineBendPtr>("сгиб по линии")->Line;
					if (line) sketch = line->Owner;
					break;
				}
				case 534: sketch = As<K7::ISheetMetalPlatePtr>("пластина")->Sketch; break;
				case 541:
				case 542: sketch = As<K7::ISheetMetalPressFormingPtr>("штамповка")->Sketch; break;
				case 543: sketch = As<K7::ISheetMetalShoulderPtr>("буртик")->Sketch; break;
				case 544: sketch = As<K7::ISheetMetalJalousiePtr>("жалюзи")->Sketch; break;
			}
		} catch (...) {
		}
		if (sketch) {
			const std::string name = BstrToUtf8(sketch->Name);
			if (!name.empty() && std::find(out.begin(), out.end(), name) == out.end()) out.push_back(name);
		}
		return out;
	}

	// Параметры для правки (edit_feature): размеры и расчёт развёртки
	NodeParams GetParamMap() override {
		NodeParams p;
		switch (type) {
			case 74: {
				K7::ISheetMetalBodyPtr b = As<K7::ISheetMetalBodyPtr>("тело");
				p["thickness"] = b->Thickness;
				p["radius"] = b->Radius;
				p["depth"] = b->GetDepth(VARIANT_TRUE);
				ReadAllowance(b, p);
				break;
			}
			case 75: {
				K7::ISheetMetalBendPtr b = As<K7::ISheetMetalBendPtr>("сгиб");
				p["length"] = b->Length;
				p["angle"] = b->Angle;
				p["radius"] = b->Radius;
				ReadAllowance(b, p);
				break;
			}
			case 76: {
				K7::ISheetMetalLineBendPtr b = As<K7::ISheetMetalLineBendPtr>("сгиб по линии");
				p["angle"] = b->Angle;
				p["radius"] = b->Radius;
				ReadAllowance(b, p);
				break;
			}
			case 541:
			case 542: p["height"] = As<K7::ISheetMetalPressFormingPtr>("штамповка")->Height; break;
			case 543: {
				K7::ISheetMetalShoulderPtr s = As<K7::ISheetMetalShoulderPtr>("буртик");
				p["height"] = s->Height;
				p["width"] = s->Width1;
				break;
			}
			case 544: {
				K7::ISheetMetalJalousiePtr j = As<K7::ISheetMetalJalousiePtr>("жалюзи");
				p["height"] = j->Height;
				p["width"] = j->Width;
				break;
			}
		}
		return p;
	}
	void SetParamMap(const NodeParams& in) override {
		auto has = [&](const char* key) { return in.count(key) != 0; };
		switch (type) {
			case 74: {
				K7::ISheetMetalBodyPtr b = As<K7::ISheetMetalBodyPtr>("тело");
				if (has("thickness")) b->Thickness = in.at("thickness");
				if (has("radius")) b->Radius = in.at("radius");
				if (has("depth") && in.at("depth") > 0) b->PutDepth(VARIANT_TRUE, in.at("depth"));
				ApplyAllowance(b, in);
				break;
			}
			case 75: {
				K7::ISheetMetalBendPtr b = As<K7::ISheetMetalBendPtr>("сгиб");
				if (has("length")) b->Length = in.at("length");
				if (has("angle")) b->Angle = in.at("angle");
				if (has("radius")) b->Radius = in.at("radius");
				ApplyAllowance(b, in);
				break;
			}
			case 76: {
				K7::ISheetMetalLineBendPtr b = As<K7::ISheetMetalLineBendPtr>("сгиб по линии");
				if (has("angle")) b->Angle = in.at("angle");
				if (has("radius")) b->Radius = in.at("radius");
				ApplyAllowance(b, in);
				break;
			}
			case 541:
			case 542:
				if (has("height")) As<K7::ISheetMetalPressFormingPtr>("штамповка")->Height = in.at("height");
				break;
			case 543: {
				K7::ISheetMetalShoulderPtr s = As<K7::ISheetMetalShoulderPtr>("буртик");
				if (has("height")) s->Height = in.at("height");
				if (has("width")) s->Width1 = in.at("width");
				break;
			}
			case 544: {
				K7::ISheetMetalJalousiePtr j = As<K7::ISheetMetalJalousiePtr>("жалюзи");
				if (has("height")) j->Height = in.at("height");
				if (has("width")) j->Width = in.at("width");
				break;
			}
		}
	}
};
