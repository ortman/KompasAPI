#pragma once

#include <map>

#include "../Include/Annotation2D.h"
#include "Node.hpp"

#include <cmath>

// Ограничения и размеры в графическом документе: эскиз в режиме редактирования или вид
// чертежа. Геометрию рисует API5 (ksDocument2D), размеры и ограничения — API7 в виде view.
// Объект API7 по ссылке API5 — KompasObject::TransferReference.
//
// Проверено на КОМПАСе v23:
//  - управляющий размер = «фиксированный размер» (ksCFixedDim) + «размер с переменной»
//    (ksCDimWithVariable); одной переменной мало — геометрия не следует за значением;
//  - Ø и R привязываются через BaseObject (Associate() у них возвращает FALSE — связь уже есть);
//  - линейный размер привязывается Associate() по совпадающим точкам;
//  - признак определённости эскиза верен в сеансе, открытом через API7 (ISketch::BeginEdit).
class Annotation2DApi7 {
	K5::ksDocument2DPtr doc;
	K7::IViewPtr view;

	K7::ISymbols2DContainerPtr Symbols() {
		K7::ISymbols2DContainerPtr symbols = view;
		if (!symbols) throw Kompas3DException("Нет контейнера размеров в документе");
		return symbols;
	}

	static long ReferenceOf(IDispatch* object) {
		K7::IKompasAPIObjectPtr o = object;
		return o ? o->Reference : 0;
	}

	// Сделать размер управляющим: фиксированный размер + переменная
public:
	// Ограничения объекта: тип → объект ограничения
	static std::vector<std::pair<int, K7::IParametriticConstraintPtr>> Constraints(K7::IDrawingObject1Ptr obj) {
		std::vector<std::pair<int, K7::IParametriticConstraintPtr>> out;
		_variant_t list = obj ? obj->Constraints : _variant_t();
		auto take = [&](IDispatch* item) {
			K7::IParametriticConstraintPtr c = item;
			if (c) out.emplace_back((int)c->ConstraintType, c);
		};
		if (list.vt == VT_DISPATCH && list.pdispVal) {
			take(list.pdispVal);
		} else if ((list.vt & VT_ARRAY) && list.parray) {
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(list.parray, 1, &lo);
			SafeArrayGetUBound(list.parray, 1, &hi);
			for (LONG i = lo; i <= hi; ++i) {
				IDispatch* item = nullptr;
				if (SUCCEEDED(SafeArrayGetElement(list.parray, &i, &item)) && item) {
					take(item);
					item->Release();
				}
			}
		}
		return out;
	}

private:
	static bool Has(const std::vector<std::pair<int, K7::IParametriticConstraintPtr>>& list, int type) {
		for (const auto& c : list) {
			if (c.first == type) return true;
		}
		return false;
	}

	// Сделать размер управляющим: «размер с переменной», затем «фиксированный размер».
	// Коды возврата Create() у КОМПАСа v23 ненадёжны (FALSE и при созданном ограничении),
	// поэтому результат проверяется по списку ограничений размера
	void Drive(DimensionInfo& info, K7::IDrawingObject1Ptr obj) {
		if (!Has(Constraints(obj), KConst::ksCDimWithVariable)) {
			K7::IParametriticConstraintPtr variable = obj->NewConstraint();
			if (variable) {
				variable->ConstraintType = KConst::ksCDimWithVariable;
				variable->Create();
			}
		}
		if (!Has(Constraints(obj), KConst::ksCFixedDim)) {
			K7::IParametriticConstraintPtr fixed = obj->NewConstraint();
			if (fixed) {
				fixed->ConstraintType = KConst::ksCFixedDim;
				fixed->Create();
			}
		}
		const auto list = Constraints(obj);
		info.driving = Has(list, KConst::ksCDimWithVariable) && Has(list, KConst::ksCFixedDim);
		for (const auto& c : list) {
			if (c.first == KConst::ksCDimWithVariable) info.variable = BstrToUtf8(c.second->Variable);
		}
	}

	// Привязан ли размер к геометрии: ассоциация (Ø/R) или слияние его точек с точками объектов
	static bool Attached(K7::IDrawingObject1Ptr obj) {
		const auto list = Constraints(obj);
		return Has(list, KConst::ksCAssociation) || Has(list, KConst::ksCMergePoints);
	}

public:
	Annotation2DApi7(K5::ksDocument2DPtr d, K7::IViewPtr v) : doc(d), view(v) {
		if (!doc || !view) throw Kompas3DException("Нет графического документа для ограничений и размеров");
	}

	// Объект API7 по ссылке API5. Объект ищется среди объектов вида того же сеанса: объект,
	// полученный через TransferReference, в сеансе API7 эскиза для привязок не годится
	// Объекты вида по ссылкам — один перебор на много обращений; новые объекты (размеры,
	// построенная геометрия) находятся перечитыванием при промахе
	std::map<long, K7::IDrawingObject1Ptr> cache;

	void Reload() {
		cache.clear();
		K7::IDrawingContainerPtr container = view;
		if (!container) return;
		_variant_t all = container->GetObjects(_variant_t((long)KConst::ksAllObj));
		auto keep = [&](IDispatch* item) {
			K7::IKompasAPIObjectPtr o = item;
			if (o) cache[o->Reference] = K7::IDrawingObject1Ptr(item);
		};
		if (all.vt == VT_DISPATCH && all.pdispVal) {
			keep(all.pdispVal);
		} else if ((all.vt & VT_ARRAY) && all.parray) {
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(all.parray, 1, &lo);
			SafeArrayGetUBound(all.parray, 1, &hi);
			for (LONG i = lo; i <= hi; ++i) {
				IDispatch* item = nullptr;
				if (FAILED(SafeArrayGetElement(all.parray, &i, &item)) || !item) continue;
				keep(item);
				item->Release();
			}
		}
	}

	K7::IDrawingObject1Ptr Object(long ref) {
		if (!ref) return nullptr;
		auto it = cache.find(ref);
		if (it == cache.end()) {
			Reload();
			it = cache.find(ref);
		}
		if (it != cache.end() && it->second) return it->second;
		if (!ComEvent::kompas5) return nullptr;
		IUnknownPtr unknown = ComEvent::kompas5->TransferReference(ref, doc->reference);
		return K7::IDrawingObject1Ptr(unknown);
	}

	// Объект удалён — убрать из кеша
	void Forget(long ref) { cache.erase(ref); }

	// Ограничение; Create() у КОМПАСа v23 ненадёжен — созданное ищется в списках ограничений
	// объектов. Возвращает созданное ограничение (для отката) или nullptr
	K7::IParametriticConstraintPtr CreateConstraint(const ConstraintSpec& s) {
		K7::IDrawingObject1Ptr obj = Object(s.a);
		if (!obj) return nullptr;
		K7::IDrawingObject1Ptr partner = s.b ? Object(s.b) : nullptr;
		if (s.b && !partner) return nullptr;
		auto count = [&]() { return Constraints(obj).size() + (partner ? Constraints(partner).size() : 0); };
		const size_t before = count();
		K7::IParametriticConstraintPtr c = obj->NewConstraint();
		if (!c) return nullptr;
		c->ConstraintType = (KConst::ksConstraintTypeEnum)s.type;
		if (s.pa >= 0) c->Index = s.pa;
		if (partner) {
			c->Partner = _variant_t((IDispatch*)K7::IDrawingObjectPtr(partner), true);
			if (s.pb >= 0) c->PartnerIndex = s.pb;
		}
		if (s.axis) {
			K7::IDrawingObject1Ptr axis = Object(s.axis);
			if (!axis) return nullptr;
			c->Axis = K7::IDrawingObjectPtr(axis);
		}
		c->Create();
		return count() > before ? c : nullptr;
	}

	// Объекты вида (эскиза) через API7; ссылки действительны в текущем сеансе редактирования
	std::vector<SketchItem> Items() {
		std::vector<SketchItem> items;
		K7::IDrawingContainerPtr container = view;
		if (!container) return items;
		_variant_t all = container->GetObjects(_variant_t((long)KConst::ksAllObj));
		auto read = [&](IDispatch* item) {
			K7::IDrawingObjectPtr object = item;
			if (!object) return;
			SketchItem it;
			it.ref = object->Reference;
			auto style = [&](long st) { it.style = (LineStyle)st; };
			switch ((int)object->DrawingObjectType) {
				case 1: {   // ksDrLineSeg
					K7::ILineSegmentPtr o = item;
					it.kind = SketchItem::Kind::Line;
					it.v = {o->X1, o->Y1, o->X2, o->Y2};
					style(o->Style);
					break;
				}
				case 2: {   // ksDrCircle
					K7::ICirclePtr o = item;
					it.kind = SketchItem::Kind::Circle;
					it.v = {o->Xc, o->Yc, o->Radius};
					style(o->Style);
					break;
				}
				case 3: {   // ksDrArc
					K7::IArcPtr o = item;
					it.kind = SketchItem::Kind::Arc;
					// IArc::Direction: TRUE — по часовой (справка и опыт), у SketchItem 1 — против часовой
					it.v = {o->Xc, o->Yc, o->Radius, o->X1, o->Y1, o->X2, o->Y2, o->Direction ? -1.0 : 1.0};
					style(o->Style);
					break;
				}
				case 28: {   // ksDrLine — вспомогательная прямая (проекция оси модели): длинный отрезок через две её точки
					K7::ILinePtr o = item;
					it.kind = SketchItem::Kind::Line;
					const double dx = o->X2 - o->X1, dy = o->Y2 - o->Y1, len = std::hypot(dx, dy);
					const double ux = len > 0 ? dx / len : std::cos(o->Angle * 3.14159265358979323846 / 180);
					const double uy = len > 0 ? dy / len : std::sin(o->Angle * 3.14159265358979323846 / 180);
					it.v = {o->X1 - ux * 1000, o->Y1 - uy * 1000, o->X1 + ux * 1000, o->Y1 + uy * 1000};
					it.style = LineStyle::Auxiliary;
					break;
				}
				case 5: {   // ksDrPoint
					K7::IPointPtr o = item;
					it.kind = SketchItem::Kind::Point;
					it.v = {o->X, o->Y};
					style(o->Style);
					break;
				}
				case 32: {   // эллипс
					K7::IEllipsePtr o = item;
					it.kind = SketchItem::Kind::Ellipse;
					it.v = {o->Xc, o->Yc, o->SemiAxisA, o->SemiAxisB, o->Angle};
					style(o->Style);
					break;
				}
				case 34: {   // дуга эллипса
					K7::IEllipseArcPtr o = item;
					it.kind = SketchItem::Kind::EllipseArc;
					it.v = {o->Xc, o->Yc, o->SemiAxisA, o->SemiAxisB, o->Angle, o->Angle1, o->Angle2, o->Direction ? -1.0 : 1.0};
					style(o->Style);
					break;
				}
				case 35: {   // прямоугольник
					K7::IRectanglePtr o = item;
					it.kind = SketchItem::Kind::Rect;
					it.v = {o->X, o->Y, o->Width, o->Height, o->Angle};
					style(o->Style);
					break;
				}
				case 36: {   // правильный многоугольник
					K7::IRegularPolygonPtr o = item;
					it.kind = SketchItem::Kind::Polygon;
					it.v = {o->Xc, o->Yc, o->Radius, (double)o->Count, o->Describe ? 1.0 : 0.0, o->Angle};
					style(o->Style);
					break;
				}
				case 9: case 10: case 13: case 14: case 15: case 38: case 39: case 40: case 43:   // размеры
					it.kind = SketchItem::Kind::Dimension;
					break;
				default:
					break;
			}
			items.push_back(std::move(it));
		};
		if (all.vt == VT_DISPATCH && all.pdispVal) {
			read(all.pdispVal);
		} else if ((all.vt & VT_ARRAY) && all.parray) {
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(all.parray, 1, &lo);
			SafeArrayGetUBound(all.parray, 1, &hi);
			for (LONG i = lo; i <= hi; ++i) {
				IDispatch* item = nullptr;
				if (SUCCEEDED(SafeArrayGetElement(all.parray, &i, &item)) && item) {
					read(item);
					item->Release();
				}
			}
		}
		return items;
	}

	static bool IsDimensionType(int type) {
		switch (type) {
			case 9: case 10: case 13: case 14: case 15: case 38: case 39: case 40: case 43: return true;
			default: return false;
		}
	}

	// Размеры вида: номинал, значок, тексты и допуск
	std::vector<DimensionRead> Dimensions() {
		std::vector<DimensionRead> out;
		K7::IDrawingContainerPtr container = view;
		if (!container) return out;
		_variant_t all = container->GetObjects(_variant_t((long)KConst::ksAllObj));
		auto read = [&](IDispatch* item) {
			K7::IDrawingObjectPtr object = item;
			if (!object || !IsDimensionType((int)object->DrawingObjectType)) return;
			DimensionRead d;
			d.ref = object->Reference;
			d.type = (int)object->DrawingObjectType;
			if (K7::IDimensionTextPtr text = item) {
				d.value = text->NominalValue;
				d.sign = text->Sign;
				if (K7::ITextLinePtr p = text->Prefix) d.prefix = BstrToUtf8(p->Str);
				if (K7::ITextLinePtr p = text->Suffix) d.suffix = BstrToUtf8(p->Str);
				if (!text->AutoNominalValue) {
					if (K7::ITextLinePtr p = text->NominalText) d.text = BstrToUtf8(p->Str);
				}
				if (text->ToleranceOn) d.fit = BstrToUtf8(text->Tolerance);
				// DeviationOn в v23 читается FALSE и при показанных отклонениях — судим по их текстам
				if (K7::ITextLinePtr p = text->HighDeviation) d.upper = BstrToUtf8(p->Str);
				if (K7::ITextLinePtr p = text->LowDeviation) d.lower = BstrToUtf8(p->Str);
				d.deviations = text->DeviationOn == VARIANT_TRUE || !d.upper.empty() || !d.lower.empty();
			}
			if (K7::ILineDimensionPtr line = item) {
				d.orientation = (int)line->Orientation;
				d.points[0] = line->X1;
				d.points[1] = line->Y1;
				d.points[2] = line->X2;
				d.points[3] = line->Y2;
				d.points[4] = line->X3;
				d.points[5] = line->Y3;
			}
			if (K5::ksRectParamPtr rect = ComEvent::kompas5->GetParamStruct(KConst::ko_RectParam)) {
				if (doc->ksGetObjGabaritRect(d.ref, rect)) {
					K5::ksMathPointParamPtr bottom = rect->GetpBot(), top = rect->GetpTop();
					if (bottom && top) {
						d.box[0] = bottom->x;
						d.box[1] = bottom->y;
						d.box[2] = top->x;
						d.box[3] = top->y;
						d.hasBox = true;
					}
				}
			}
			out.push_back(std::move(d));
		};
		if (all.vt == VT_DISPATCH && all.pdispVal) {
			read(all.pdispVal);
		} else if ((all.vt & VT_ARRAY) && all.parray) {
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(all.parray, 1, &lo);
			SafeArrayGetUBound(all.parray, 1, &hi);
			for (LONG i = lo; i <= hi; ++i) {
				IDispatch* item = nullptr;
				if (SUCCEEDED(SafeArrayGetElement(all.parray, &i, &item)) && item) {
					read(item);
					item->Release();
				}
			}
		}
		return out;
	}

	// Допуск размера: квалитет и/или отклонения; проверяется чтением
	bool Tolerance(long ref, const DimensionTolerance& t) {
		K7::IDrawingObject1Ptr obj = Object(ref);
		K7::IDimensionTextPtr text = obj;
		if (!text) return false;
		if (!t.fit.empty()) {
			text->Tolerance = Utf8ToBstr(t.fit);
			text->ToleranceOn = t.showFit ? VARIANT_TRUE : VARIANT_FALSE;
		}
		if (t.deviations || (t.upper && t.lower)) {
			text->DeviationOn = VARIANT_TRUE;
			text->DeviationType = KConst::ksDimDeviation;
			if (t.upper && t.lower) text->InitDeviations(*t.upper, *t.lower);
		}
		K7::IDrawingObjectPtr(obj)->Update();
		return t.fit.empty() || BstrToUtf8(text->Tolerance) == t.fit;
	}

	// Знак шероховатости на линии вида или на полке выноски от неё.
	// Выноску КОМПАС строит по углу и длине (по умолчанию 90° и 10 мм), а не по точке полки, и
	// задать их можно только у созданного знака. Точка на поверхности при этом может сдвинуться
	// (на вертикальной линии — на длину выноски по умолчанию): тогда знак создаётся заново с
	// поправкой на тот же сдвиг.
	long Rough(const Rough2D& r) {
		K7::IDrawingObject1Ptr base = Object(r.object);
		if (!base) throw Kompas3DException("Нет линии вида для шероховатости");
		K7::IRoughsPtr roughs = Symbols()->Roughs;
		if (!roughs) throw Kompas3DException("Не могу получить знаки шероховатости вида");
		auto create = [&](double bx, double by) -> K7::IRoughPtr {
			K7::IRoughPtr rough = roughs->Add();
			if (!rough) throw Kompas3DException("КОМПАС не создал знак шероховатости");
			rough->BaseObject = K7::IDrawingObjectPtr(base);
			rough->BranchX0 = bx;
			rough->BranchY0 = by;
			rough->ShelfX = r.leader ? bx : r.shelfX;
			rough->ShelfY = r.leader ? by + 10 : r.shelfY;
			if (K7::IRoughParamsPtr params = rough) {
				params->SignType = (KConst::ksRoughSignEnum)r.sign;
				if (r.leader) params->ShelfDirection = r.shelfX >= r.x ? KConst::ksLSRight : KConst::ksLSLeft;
				if (K7::ITextPtr text = params->RoughParamText) text->Str = Utf8ToBstr(r.text);
			}
			K7::IDrawingObjectPtr object = rough;
			if (!object->Update()) return nullptr;
			if (r.leader) {
				K7::IRoughParamsPtr params = rough;
				const double dx = r.shelfX - r.x, dy = r.shelfY - r.y;
				params->LeaderAngle = std::atan2(dy, dx) * 180 / 3.14159265358979323846;
				params->LeaderLength = std::hypot(dx, dy);
				object->Update();
			}
			return rough;
		};
		K7::IRoughPtr rough = create(r.x, r.y);
		if (!rough) return 0;
		const double sx = rough->BranchX0 - r.x, sy = rough->BranchY0 - r.y;
		if (r.leader && std::hypot(sx, sy) > 0.05) {
			K7::IDrawingObjectPtr(rough)->Delete();
			rough = create(r.x - sx, r.y - sy);
			if (!rough) return 0;
		}
		return K7::IDrawingObjectPtr(rough)->Reference;
	}

	bool Parametrize(const std::vector<long>& refs, const Parametrize2D& o) {
		if (refs.empty()) return true;
		K5::ksParametrizationParamPtr par = ComEvent::kompas5->GetParamStruct(KConst::ko_ParametrisationParam);
		if (!par) throw Kompas3DException("Не могу получить параметры параметризации");
		par->Init();
		par->nearestPoints = o.mergePoints ? VARIANT_TRUE : VARIANT_FALSE;
		par->pointsLimit = o.pointsLimit;
		par->horizontal = o.horizontal ? VARIANT_TRUE : VARIANT_FALSE;
		par->vertical = o.vertical ? VARIANT_TRUE : VARIANT_FALSE;
		par->parallel = o.parallel ? VARIANT_TRUE : VARIANT_FALSE;
		par->perpendicular = o.perpendicular ? VARIANT_TRUE : VARIANT_FALSE;
		par->angleLimit = o.angleLimit;
		// Модельная группа — для уже существующих объектов; очистка группы объекты не удаляет
		const long group = doc->ksNewGroup(0);
		doc->ksEndGroup();
		if (!group) return false;
		for (long ref : refs) doc->ksAddObjGroup(group, ref);
		const bool ok = doc->ksParametrizeObjects(group, par) != 0;
		doc->ksClearGroup(group, VARIANT_FALSE);
		return ok;
	}

	// Ограничение на объект a (точка pa, -1 — объект целиком) относительно объекта b (pb)
	bool Constraint(Constraint2D type, long a, int pa, long b, int pb) {
		K7::IDrawingObject1Ptr obj = Object(a);
		if (!obj) return false;
		K7::IParametriticConstraintPtr c = obj->NewConstraint();
		if (!c) return false;
		c->ConstraintType = (KConst::ksConstraintTypeEnum)type;
		if (pa >= 0) c->Index = pa;
		if (b) {
			K7::IDrawingObjectPtr partner = Object(b);
			if (!partner) return false;
			c->Partner = _variant_t((IDispatch*)partner, true);
			if (pb >= 0) c->PartnerIndex = pb;
		}
		return c->Create() == VARIANT_TRUE;
	}

	// Свой текст размера: приставка и/или замена значения (обозначение резьбы — без знака Ø)
	static void ApplyText(IDispatch* dimension, const Dimension2D& d) {
		if (d.text.empty() && d.prefix.empty() && d.suffix.empty() && d.under.empty() && d.sign < 0) return;
		K7::IDimensionTextPtr text = dimension;
		if (!text) return;
		if (!d.prefix.empty()) {
			K7::ITextLinePtr prefix = text->Prefix;
			if (prefix) prefix->Str = Utf8ToBstr(d.prefix);
		}
		if (!d.suffix.empty()) {
			K7::ITextLinePtr suffix = text->Suffix;
			if (suffix) suffix->Str = Utf8ToBstr(d.suffix);
		}
		if (!d.under.empty()) {
			// Шрифт ГОСТ читает строку в CP1251: Ø и × — спецзнаками (2 и 4), иначе «Ш», «Ч»
			K7::ITextPtr under = text->TextUnder;
			K7::ITextLinePtr line = under ? (under->Clear(), under->Add()) : nullptr;
			if (line) {
				static const std::pair<std::string, long> kSymbols[] = {{"Ø", 2}, {"×", 4}};
				std::string part;
				auto flush = [&]() {
					if (part.empty()) return;
					K7::ITextItemPtr item = line->Add();
					item->ItemType = KConst::ksTItString;
					item->Str = Utf8ToBstr(part);
					item->Update();
					part.clear();
				};
				for (size_t pos = 0; pos < d.under.size();) {
					bool symbol = false;
					for (const auto& [sym, number] : kSymbols) {
						if (d.under.compare(pos, sym.size(), sym) != 0) continue;
						flush();
						K7::ITextItemPtr item = line->Add();
						item->ItemType = KConst::ksTItSpecialSymbol;
						item->Number = number;
						item->Update();
						pos += sym.size();
						symbol = true;
						break;
					}
					if (!symbol) part += d.under[pos++];
				}
				flush();
			}
		}
		if (!d.text.empty()) {
			text->AutoNominalValue = VARIANT_FALSE;
			text->Sign = 0;
			K7::ITextLinePtr nominal = text->NominalText;
			if (nominal) nominal->Str = Utf8ToBstr(d.text);
		}
		if (d.sign >= 0) text->Sign = d.sign;   // знак Ø — значком, текстом шрифт ГОСТ его не покажет
	}

	// Надпись Ø/R на горизонтальной полке
	static void ApplyShelf(IDispatch* dimension, int shelf) {
		if (!shelf) return;
		K7::IDimensionParamsPtr params = dimension;
		if (params) params->ShelfDirection = shelf > 0 ? KConst::ksLSRight : KConst::ksLSLeft;
	}

	DimensionInfo Dimension(const Dimension2D& d) {
		DimensionInfo info;
		using Kind = Dimension2D::Kind;
		K7::IDrawingObject1Ptr obj;
		if (d.kind == Kind::Angle) {
			K7::IDrawingObjectPtr first = Object(d.object), second = Object(d.object2);
			if (!first || !second) throw Kompas3DException("Для углового размера нужны два отрезка (object, object2)");
			K7::IAngleDimensionPtr dim = Symbols()->AngleDimensions->Add(KConst::ksDrADimension);
			dim->BaseObject1 = first;
			dim->BaseObject2 = second;
			dim->Xc = d.x1;
			dim->Yc = d.y1;
			dim->Radius = d.offset;
			ApplyText(dim, d);
			if (!dim->Update()) return info;
			obj = dim;
			obj->Associate();
			info.associated = Attached(obj);
			info.ref = ReferenceOf(obj);
			if (d.driving && info.associated) Drive(info, obj);
			return info;
		}
		if (d.kind == Kind::Diameter || d.kind == Kind::Radius) {
			K7::IDrawingObjectPtr base = Object(d.object);
			if (!base) throw Kompas3DException("Для размера Ø/R нужна окружность или дуга (object)");
			if (d.kind == Kind::Diameter) {
				K7::IDiametralDimensionPtr dim = Symbols()->DiametralDimensions->Add();
				dim->BaseObject = base;
				dim->Angle = d.angle;
				ApplyText(dim, d);
				ApplyShelf(dim, d.shelf);
				if (!dim->Update()) return info;
				obj = dim;
			} else {
				K7::IRadialDimensionPtr dim = Symbols()->RadialDimensions->Add();
				dim->BaseObject = base;
				dim->Angle = d.angle;
				ApplyText(dim, d);
				ApplyShelf(dim, d.shelf);
				if (!dim->Update()) return info;
				obj = dim;
			}
			if (d.textAt) {
				K7::IDimension2DPtr dim2 = obj;
				if (dim2 && dim2->SetTextPosition(d.textX, d.textY)) K7::IDrawingObjectPtr(obj)->Update();
			}
			obj->Associate();   // FALSE и при уже существующей связи — проверяем по ограничениям
			info.associated = Attached(obj);
		} else {
			K7::ILineDimensionPtr dim = Symbols()->LineDimensions->Add();
			dim->X1 = d.x1;
			dim->Y1 = d.y1;
			dim->X2 = d.x2;
			dim->Y2 = d.y2;
			// Точка на размерной линии: от первой точки на offset поперёк измерения
			if (d.kind == Kind::Horizontal) {
				dim->X3 = (d.x1 + d.x2) / 2;
				dim->Y3 = d.y1 + d.offset;
				dim->Orientation = KConst::ksLinDHorizontal;
			} else if (d.kind == Kind::Vertical) {
				dim->X3 = d.x1 + d.offset;
				dim->Y3 = (d.y1 + d.y2) / 2;
				dim->Orientation = KConst::ksLinDVertical;
			} else {
				const double lx = d.x2 - d.x1, ly = d.y2 - d.y1, len = std::hypot(lx, ly);
				dim->X3 = (d.x1 + d.x2) / 2 + (len > 0 ? -ly / len * d.offset : 0);
				dim->Y3 = (d.y1 + d.y2) / 2 + (len > 0 ? lx / len * d.offset : d.offset);
				dim->Orientation = KConst::ksLinDParallel;
			}
			ApplyText(dim, d);
			if (!dim->Update()) return info;
			obj = dim;
			if (d.textAt) {
				K7::IDimension2DPtr dim2 = obj;
				if (dim2 && dim2->SetTextPosition(d.textX, d.textY)) K7::IDrawingObjectPtr(obj)->Update();
			}
			obj->Associate();   // при совпадающих точках связи появляются уже в Update()
			info.associated = Attached(obj);
		}
		info.ref = ReferenceOf(obj);
		if (K7::IDimension2DPtr dim2 = obj) dim2->GetTextPosition(&info.textX, &info.textY);
		if (d.driving && info.associated) Drive(info, obj);
		return info;
	}

	ObjectDefinition State(long ref) {
		K7::IDrawingObject1Ptr obj = Object(ref);
		return obj ? (ObjectDefinition)(int)obj->ConstraintsState : ObjectDefinition::Unknown;
	}
};
