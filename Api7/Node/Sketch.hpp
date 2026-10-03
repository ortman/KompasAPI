#pragma once

#include "../Kompas3D.hpp"
#include "../../Include/Node/Sketch.h"
#include "Plane.hpp"
#include "Face.hpp"
#include "../Annotation2D.hpp"

class SketchApi7 : public NodeApi7, public Sketch::SketchImpl {
private:
	K5::ksDocument2DPtr doc2D = nullptr;
	double lastX = 0.0;
	double lastY = 0.0;
	long lastRef = 0;
	// Сеанс редактирования через API7: в нём верен признак определённости и доступны
	// размеры и ограничения; геометрию рисует API5-документ того же сеанса
	K7::ISketchPtr sketch7 = nullptr;
	K7::IViewPtr view7 = nullptr;

	Annotation2DApi7 Annotation() {
		BeginEdit();
		if (!view7) throw Kompas3DException("Ограничения и размеры эскиза требуют редактирования через API7");
		return Annotation2DApi7(doc2D, view7);
	}

public :
	SketchApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetPlane(const Plane& plane) override {
		K5::ksSketchDefinitionPtr d = def;
		NodeApi7* node = dynamic_cast<NodeApi7*>(plane.node.get());
		if (node) {
			K5::ksEntityPtr planeEntity = node->entity;
			if (!planeEntity) throw Kompas3DException("Не могу получить Plane для Эскиза");
			d->SetPlane(planeEntity);
		}
	}
	void SetAngle(double angle) override {
		K5::ksSketchDefinitionPtr d = def;
		d->angle = angle;
	}
	void SetLocation(double locX, double locY) override {
		K5::ksSketchDefinitionPtr d = def;
		d->SetLocation(locX, locY);
	}
	Plane::Point2D Projection(const Vertex::Point3D& point) override {
		K5::ksSketchDefinitionPtr d = def;
		K5::ksSurfacePtr surface = d->GetSurface();
		K5::ksPlaneParamPtr param = surface->GetSurfaceParam();
	  K5::ksPlacementPtr surfPl = param->GetPlacement();
	  Plane::Point2D res;
	  if (surfPl->PointProjection(point.x, point.y, point.z, &res.x, &res.y)) {
	    return res;
	  }
	  return {0., 0.};
	}
	std::optional<Sketch::Placement> GetPlacement() override {
		K5::ksSketchDefinitionPtr d = def;
		if (!d) return std::nullopt;
		K5::ksSurfacePtr surface = d->GetSurface();
		if (!surface) return std::nullopt;
		K5::ksPlaneParamPtr param = surface->GetSurfaceParam();
		if (!param) return std::nullopt;
		K5::ksPlacementPtr pl = param->GetPlacement();
		if (!pl) return std::nullopt;
		Sketch::Placement p{};
		pl->GetOrigin(&p.origin.x, &p.origin.y, &p.origin.z);
		pl->GetVector(0, &p.x.x, &p.x.y, &p.x.z);
		pl->GetVector(1, &p.y.x, &p.y.y, &p.y.z);
		pl->GetVector(2, &p.z.x, &p.z.y, &p.z.z);
		return p;
	}
	// Объекты эскиза: итератор по каждому поддерживаемому типу (тип объекта по ссылке API5 не
	// сообщает), затем общий подсчёт — остальные объекты помечаются Kind::Other
	std::vector<SketchItem> GetItems() override {
		BeginEdit();
		if (view7) return Annotation().Items();
		std::vector<SketchItem> items;
		K5::ksIteratorPtr it = ComEvent::kompas5->GetIterator();
		if (!it) return items;
		auto each = [&](long type, auto read) {
			if (!it->ksCreateIterator(type, 0)) return;
			for (long ref = it->ksMoveIterator("F"); ref; ref = it->ksMoveIterator("N")) {
				items.push_back(read(ref));
				items.back().ref = ref;
			}
		};
		auto param = [&](auto type) { return ComEvent::kompas5->GetParamStruct(type); };
		each(1, [&](long ref) {   // LINESEG_OBJ
			K5::ksLineSegParamPtr p = param(KConst::ko_LineSegParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Line, (LineStyle)p->style, {p->x1, p->y1, p->x2, p->y2}};
		});
		each(2, [&](long ref) {   // CIRCLE_OBJ
			K5::ksCircleParamPtr p = param(KConst::ko_CircleParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Circle, (LineStyle)p->style, {p->xc, p->yc, p->rad}};
		});
		each(3, [&](long ref) {   // ARC_OBJ
			K5::ksArcByPointParamPtr p = param(KConst::ko_ArcByPointParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Arc, (LineStyle)p->style,
			                  {p->xc, p->yc, p->rad, p->x1, p->y1, p->x2, p->y2, (double)p->dir}};
		});
		each(5, [&](long ref) {   // POINT_OBJ
			K5::ksPointParamPtr p = param(KConst::ko_PointParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Point, (LineStyle)p->style, {p->x, p->y}};
		});
		each(32, [&](long ref) {   // ELLIPSE_OBJ
			K5::ksEllipseParamPtr p = param(KConst::ko_EllipseParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Ellipse, (LineStyle)p->style, {p->xc, p->yc, p->A, p->B, p->angle}};
		});
		each(34, [&](long ref) {   // ELLIPSE_ARC_OBJ
			K5::ksEllipseArcParamPtr p = param(KConst::ko_EllipsArcParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::EllipseArc, (LineStyle)p->style,
			                  {p->xc, p->yc, p->A, p->B, p->angle, p->angleFirst, p->angleSecond, (double)p->direction}};
		});
		each(35, [&](long ref) {   // RECTANGLE_OBJ
			K5::ksRectangleParamPtr p = param(KConst::ko_RectangleParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Rect, (LineStyle)p->style, {p->x, p->y, p->width, p->height, p->ang}};
		});
		each(36, [&](long ref) {   // REGULARPOLYGON_OBJ
			K5::ksRegularPolygonParamPtr p = param(KConst::ko_RegularPolygonParam);
			doc2D->ksGetObjParam(ref, p, -1);
			return SketchItem{SketchItem::Kind::Polygon, (LineStyle)p->style,
			                  {p->xc, p->yc, p->radius, (double)p->count, p->describe ? 1.0 : 0.0, p->ang}};
		});
		size_t total = 0;
		if (it->ksCreateIterator(0 /*ALL_OBJ*/, 0)) {
			for (long ref = it->ksMoveIterator("F"); ref; ref = it->ksMoveIterator("N")) ++total;
		}
		for (size_t i = items.size(); i < total; ++i) items.push_back(SketchItem{});   // неподдерживаемые
		return items;
	}
	void DeleteAll() override {
		BeginEdit();
		std::vector<long> refs;
		K5::ksIteratorPtr it = ComEvent::kompas5->GetIterator();
		if (!it || !it->ksCreateIterator(0 /*ALL_OBJ*/, 0)) return;
		for (long ref = it->ksMoveIterator("F"); ref; ref = it->ksMoveIterator("N")) refs.push_back(ref);
		for (long ref : refs) doc2D->ksDeleteObj(ref);
	}
	long LastObject() override { return lastRef; }

	bool Parametrize(const std::vector<long>& refs, const Parametrize2D& options) override {
		BeginEdit();
		std::vector<long> all = refs;
		if (all.empty()) {
			K5::ksIteratorPtr it = ComEvent::kompas5->GetIterator();
			if (it && it->ksCreateIterator(0 /*ALL_OBJ*/, 0)) {
				for (long ref = it->ksMoveIterator("F"); ref; ref = it->ksMoveIterator("N")) all.push_back(ref);
			}
		}
		return Annotation().Parametrize(all, options);
	}

	bool AddConstraint(Constraint2D type, long a, int pointA, long b, int pointB) override {
		BeginEdit();
		return Annotation().Constraint(type, a, pointA, b, pointB);
	}

	DimensionInfo AddDimension(const Dimension2D& dimension) override {
		BeginEdit();
		return Annotation().Dimension(dimension);
	}

	ObjectDefinition GetObjectDefinition(long ref) override {
		BeginEdit();
		return Annotation().State(ref);
	}

	// ISketch::AddProjectionOf (КОМПАС v22+): объекты-проекции, ссылки — в документе эскиза
	std::vector<long> ProjectEntity(K5::ksEntityPtr object) {
		BeginEdit();
		K7::IModelObjectPtr model = ToApi7<K7::IModelObjectPtr>(object);
		if (!sketch7 || !model) throw Kompas3DException("Не могу получить эскиз или объект модели (API7)");
		std::vector<long> refs;
		_variant_t created = sketch7->AddProjectionOf(model);
		// Проекции — вспомогательные линии: иначе они входят в профиль операции эскиза
		auto take = [&](IDispatch* item) {
			K7::IKompasAPIObjectPtr o = item;
			if (!o) return;
			refs.push_back(o->Reference);
			const long aux = (long)LineStyle::Auxiliary;
			bool styled = true;
			if (K7::ILineSegmentPtr line = item) line->Style = aux;
			else if (K7::IArcPtr arc = item) arc->Style = aux;
			else if (K7::ICirclePtr circle = item) circle->Style = aux;
			else if (K7::IEllipsePtr ellipse = item) ellipse->Style = aux;
			else if (K7::IEllipseArcPtr ellipseArc = item) ellipseArc->Style = aux;
			else styled = false;   // точки и прочее в профиль не входят
			if (styled) K7::IDrawingObjectPtr(item)->Update();
		};
		if (created.vt == VT_DISPATCH && created.pdispVal) {
			take(created.pdispVal);
		} else if ((created.vt & VT_ARRAY) && created.parray) {
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(created.parray, 1, &lo);
			SafeArrayGetUBound(created.parray, 1, &hi);
			for (LONG i = lo; i <= hi; ++i) {
				IDispatch* item = nullptr;
				if (SUCCEEDED(SafeArrayGetElement(created.parray, &i, &item)) && item) {
					take(item);
					item->Release();
				}
			}
		}
		return refs;
	}

	std::vector<long> Project(const Node& modelObject) override {
		NodeApi7* object = dynamic_cast<NodeApi7*>(modelObject.node.get());
		if (!object || !object->entity) throw Kompas3DException("Не могу получить объект модели для проекции");
		return ProjectEntity(object->entity);
	}

	long ProjectOrigin() override {
		K5::ksPartPtr part = entity->GetParent();
		K5::ksEntityPtr origin = part ? part->GetDefaultEntity(4 /*o3d_pointCS*/) : nullptr;
		if (!origin) throw Kompas3DException("Не могу получить начало координат модели");
		std::vector<long> refs = ProjectEntity(origin);
		return refs.empty() ? 0 : refs.front();
	}

	std::vector<long> ProjectSupport() override {
		K5::ksSketchDefinitionPtr d = def;
		K5::ksEntityPtr support = d ? d->GetPlane() : nullptr;
		if (!support || support->type != 6 /*o3d_face*/) return {};
		return ProjectEntity(support);
	}

	std::unique_ptr<Node::NodeImpl> GetSupportFace() override {
		K5::ksSketchDefinitionPtr d = def;
		K5::ksEntityPtr support = d ? d->GetPlane() : nullptr;
		if (!support || support->type != 6 /*o3d_face*/) return nullptr;
		return std::make_unique<FaceApi7>(support, nullptr);
	}

	bool DeleteObject(long ref) override {
		K7::IDrawingObjectPtr object = Annotation().Object(ref);
		if (!object) return false;
		object->Delete();   // возвращает FALSE и при удалённом объекте
		return true;
	}

	SketchDefinition GetDefinition() override {
		BeginEdit();
		return sketch7 ? (SketchDefinition)(int)sketch7->ConstraintsState : SketchDefinition::Unknown;
	}

	void BeginEdit() override {
		if (doc2D) return;
		sketch7 = ToApi7<K7::ISketchPtr>(entity);
		if (sketch7) {
			K7::IKompasDocument2DPtr fragment = sketch7->BeginEdit();
			if (fragment) {
				view7 = fragment->ViewsAndLayersManager->Views->ActiveView;
				doc2D = ToApi5<K5::ksDocument2DPtr>(fragment);
			}
			if (!doc2D) {
				sketch7->EndEdit();
				view7 = nullptr;
			}
		}
		if (!doc2D) {   // запасной путь — API5
			sketch7 = nullptr;
			K5::ksSketchDefinitionPtr d = def;
			doc2D = d->BeginEdit();
		}
		if (!doc2D) throw Kompas3DException("Не могу открыть эскиз!");
	}
	void EndEdit() override {
		if (!doc2D) return;
		if (view7 && sketch7) {
			sketch7->EndEdit();
		} else {
			K5::ksSketchDefinitionPtr d = def;
			d->EndEdit();
		}
		doc2D = nullptr;
		view7 = nullptr;
	}
	bool IsEdit() override { return (bool) doc2D; }
	void Clear() override {
		BeginEdit();
		doc2D->ksSelectGroup(0, 2, -1, -1, 1, 1);
		doc2D->ksDeleteObj(0);
		doc2D->ksSelectGroup(0, 3, -2, -2, 2, 2);
		doc2D->ksDeleteObj(0);
	}
	void Line(double x1, double y1, double x2, double y2, LineStyle style) override {
		BeginEdit();
		lastRef = doc2D->ksLineSeg(x1, y1, x2, y2, (int)style);
		lastX = x2;
		lastY = y2;
	}
	void LineTo(double x, double y, LineStyle style) override {
		Line(lastX, lastY, x, y, style);
	}
	void Circle(double cx, double cy, double r, LineStyle style) override {
		BeginEdit();
		lastRef = doc2D->ksCircle(cx, cy, r, (int)style);
		lastX = cx;
		lastY = cy;
	}
	void Rect(double x, double y, double w, double h, double angle, LineStyle style) override {
		K5::ksRectangleParamPtr param = ComEvent::kompas5->GetParamStruct(KConst::ko_RectangleParam);
		if (!param) return;
		BeginEdit();
		param->Init();
		param->x = x;
		param->y = y;
		param->width = w;
		param->height = h;
		param->style = (int)style;
		param->ang = angle;
		lastRef = doc2D->ksRectangle(param, 0);
		lastX = x + w;
		lastY = y + h;
	}
	void RegularPolygon(double cx, double cy, double r, int count, bool describe, double angle, LineStyle style) override {
		K5::ksRegularPolygonParamPtr param = ComEvent::kompas5->GetParamStruct(KConst::ko_RegularPolygonParam);
		if (!param) return;
		BeginEdit();
		param->Init();
		param->xc = cx;
		param->yc = cy;
		param->radius = r;
		param->count = count;
		param->describe = describe;
		param->ang = angle;
		param->style = (int)style;
		lastRef = doc2D->ksRegularPolygon(param, 0);
		lastX = cx;
		lastY = cy;
	}
	void Point(double x, double y, LineStyle style) override {
		BeginEdit();
		lastRef = doc2D->ksPoint(x, y, (int)style);
		lastX = x;
		lastY = y;
	}
	void ArcByAngle(double cx, double cy, double r, double f1, double f2, bool cw, LineStyle style) override {
		BeginEdit();
		lastRef = doc2D->ksArcByAngle(cx, cy, r, f1, f2, cw ? -1 : 1, (int)style);
		lastX = cx;
		lastY = cy;
	}
	void ArcByPoint(double cx, double cy, double r, double x1, double y1, double x2, double y2, bool cw, LineStyle style) override {
		BeginEdit();
		lastRef = doc2D->ksArcByPoint(cx, cy, r, x1, y1, x2, y2, cw ? -1 : 1, (int)style);
		lastX = cx;
		lastY = cy;
	}
	void ArcBy3Points(double x1, double y1, double x2, double y2, double x3, double y3, LineStyle style) override {
		BeginEdit();
		lastRef = doc2D->ksArcBy3Points(x1, y1, x2, y2, x3, y3, (int)style);
		lastX = x3;
		lastY = y3;
	}
	void Ellipse(double cx, double cy, double a, double b, double angle, LineStyle style) override {
		K5::ksEllipseParamPtr param = ComEvent::kompas5->GetParamStruct(KConst::ko_EllipseParam);
		if (!param) return;
		BeginEdit();
		param->Init();
		param->xc = cx;
		param->yc = cy;
		param->A = a;
		param->B = b;
		param->angle = angle;
		param->style = (int)style;
		lastRef = doc2D->ksEllipse(param);
		lastX = cx;
		lastY = cy;
	}
	void EllipseArc(double cx, double cy, double a, double b, double a1, double a2, bool cw, double angle, LineStyle style) override {
		K5::ksEllipseArcParamPtr param = ComEvent::kompas5->GetParamStruct(KConst::ko_EllipsArcParam);
		if (!param) return;
		BeginEdit();
		param->Init();
		param->xc = cx;
		param->yc = cy;
		param->A = a;
		param->B = b;
		param->angle = angle;
		param->angleFirst = a1;
		param->angleSecond = a2;
		param->direction = cw ? -1 : 1;
		param->style = (int)style;
		lastRef = doc2D->ksEllipseArc(param);
		lastX = cx;
		lastY = cy;
	}
};
