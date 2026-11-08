#pragma once

#include "../../Include/Node/Surfaces.h"
#include "../Node.hpp"
#include "Features.hpp"
#include "Rotated.hpp"

#include <vector>

namespace Api7Surfaces {

// Объект модели API7 узла: у сущностей API5 — через TransferInterface
inline K7::IModelObjectPtr Model7(const Node& n, const char* what) {
	NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
	K7::IModelObjectPtr model = n7 && n7->entity ? ToApi7<K7::IModelObjectPtr>(n7->entity) : nullptr;
	if (!model) throw Kompas3DException(std::string("Не могу получить объект модели: ") + what);
	return model;
}

// VARIANT для свойств-списков API7: один объект — VT_DISPATCH, несколько — массив
inline _variant_t Objects(const std::vector<const Node*>& nodes, const char* what) {
	if (nodes.empty()) throw Kompas3DException(std::string("Не заданы объекты: ") + what);
	if (nodes.size() == 1) return _variant_t((IDispatch*)Model7(*nodes.front(), what), true);
	SAFEARRAY* array = SafeArrayCreateVector(VT_DISPATCH, 0, (ULONG)nodes.size());
	for (LONG i = 0; i < (LONG)nodes.size(); ++i) {
		K7::IModelObjectPtr model = Model7(*nodes[i], what);
		SafeArrayPutElement(array, &i, (IDispatch*)model);
	}
	_variant_t result;
	result.vt = VT_ARRAY | VT_DISPATCH;
	result.parray = array;
	return result;
}

} // namespace Api7Surfaces

// Объект, создаваемый через коллекции API7 (кривые, линейчатая поверхность, заплатка, сшивка,
// толщина, сечение). До Create() есть только объект API7; после — сущность API5 из дерева
// (по имени), чтобы имя, код ошибки, удаление, переменные и цвет работали как у операций API5.
// Проверено на v24: TransferInterface объекта API7 в API5 даёт определение, а не сущность
class ModelObjectApi7 : public NodeApi7 {
protected:
	K5::ksPartPtr part;
	K7::IModelObjectPtr object;
	int type;
	std::optional<std::string> pendingName;

	void Attach() {
		const std::string name = BstrToUtf8(object->Name);
		K5::ksFeaturePtr top = part ? part->GetFeature() : nullptr;
		K5::ksFeatureCollectionPtr features = top ? top->SubFeatureCollection(true, true) : nullptr;
		if (!features) return;
		for (long i = features->GetCount() - 1; i >= 0; --i) {
			K5::ksFeaturePtr feature = features->GetByIndex(i);
			K5::ksEntityPtr e = feature ? feature->GetObject() : nullptr;
			if (e && BstrToUtf8(e->name) == name) {
				entity = e;
				def = e->GetDefinition();
				return;
			}
		}
	}

public:
	ModelObjectApi7(K5::ksPartPtr p, K7::IModelObjectPtr o, int t) : NodeApi7(nullptr, nullptr), part(p), object(o), type(t) {
		if (!object) throw Kompas3DException("КОМПАС не создал объект каркаса или поверхности");
	}
	K7::IModelObjectPtr Object() const { return object; }
	int GetType() const override { return entity ? NodeApi7::GetType() : type; }
	std::string GetName() const override { return entity ? NodeApi7::GetName() : BstrToUtf8(object->Name); }
	void SetName(const std::string& name) override {
		if (entity) NodeApi7::SetName(name);
		else pendingName = name;
	}
	void Create() override {
		object->Update();
		Attach();
		if (pendingName && entity) {
			NodeApi7::SetName(*pendingName);
			entity->Update();
		}
	}
	void Update() override { object->Update(); }
	int GetError() const override {
		const int error = entity ? NodeApi7::GetError() : 0;
		if (error != 0) return error;
		return object->Valid != VARIANT_FALSE ? 0 : -1;
	}
	const void* GetIdentity() const override {
		if (entity) return NodeApi7::GetIdentity();
		IUnknownPtr unknown = object;
		return unknown.GetInterfacePtr();
	}
	// Удалить из модели, даже если сущность API5 не нашлась
	bool Delete() {
		K7::IFeature7Ptr feature = object->Owner;
		return feature && feature->Delete() != VARIANT_FALSE;
	}
};

// ------------------------------------------------------ кривые по точкам --
template <typename Curve7Ptr>
class CurveByPointsApi7 : public ModelObjectApi7, public CurveByPointsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetPoints(const std::vector<Vertex::Point3D>& points, bool closed) override {
		Curve7Ptr curve = object;
		if (!curve) throw Kompas3DException("Не могу получить кривую API7");
		for (long i = 0; i < (long)points.size(); ++i) {
			K7::ICurveVertexParamPtr vertex = curve->AddVertex(i);
			if (!vertex) throw Kompas3DException("КОМПАС не добавил вершину кривой");
			vertex->SetParamVertex(points[i].x, points[i].y, points[i].z, 0.0);
		}
		curve->Closed = closed ? VARIANT_TRUE : VARIANT_FALSE;
	}
};

using Spline3DApi7 = CurveByPointsApi7<K7::ISpline3DPtr>;
using PolyLine3DApi7 = CurveByPointsApi7<K7::IPolyLinePtr>;

// ------------------------------------------------- поверхности по эскизам --
class ExtrusionSurfaceApi7 : public NodeApi7, public SketchSurfaceImpl {
	K5::ksExtrusionSurfaceDefinitionPtr Def() {
		K5::ksExtrusionSurfaceDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksExtrusionSurfaceDefinition");
		return d;
	}
public:
	ExtrusionSurfaceApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& sketch) override { Def()->SetSketch(Api7Features::EntityOf(sketch, "эскиз поверхности")); }
	void SetExtrusion(const ExtrusionParams& p) override {
		K5::ksExtrusionSurfaceDefinitionPtr d = Def();
		d->directionType = (short)p.direction;
		d->SetSideParam(VARIANT_TRUE, (short)p.end1, p.depth1, 0, VARIANT_FALSE);
		if (p.direction == ExtrusionDirection::Both) d->SetSideParam(VARIANT_FALSE, (short)p.end2, p.depth2, 0, VARIANT_FALSE);
	}
};

class EvolutionSurfaceApi7 : public NodeApi7, public SketchSurfaceImpl {
	K5::ksEvolutionSurfaceDefinitionPtr Def() {
		K5::ksEvolutionSurfaceDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksEvolutionSurfaceDefinition");
		return d;
	}
public:
	EvolutionSurfaceApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void SetSketch(Sketch& sketch) override { Def()->SetSketch(Api7Features::EntityOf(sketch, "эскиз сечения")); }
	void AddPath(const Node& path) override { Api7Features::Add(Def()->PathPartArray(), path, "траектория"); }
};

class LoftSurfaceApi7 : public NodeApi7, public SketchSurfaceImpl {
	K5::ksLoftSurfaceDefinitionPtr Def() {
		K5::ksLoftSurfaceDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksLoftSurfaceDefinition");
		return d;
	}
public:
	LoftSurfaceApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	void AddSection(Sketch& section) override { Api7Features::Add(Def()->Sketchs(), section, "эскиз сечения"); }
	void SetClosed(bool closed) override { Def()->SetLoftParam(closed, VARIANT_FALSE, VARIANT_TRUE); }
};

using RotatedSurfaceApi7 = RotatedNodeApi7<61, K5::ksRotatedSurfaceDefinitionPtr>;

// ------------------------------- поверхности и операции по объектам модели --
class RuledSurfaceApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::IRuledSurfacePtr(object)->Curves1 = Api7Surfaces::Objects(objects, "первая кривая");
	}
	void SetObjects2(const std::vector<const Node*>& objects) override {
		K7::IRuledSurfacePtr(object)->Curves2 = Api7Surfaces::Objects(objects, "вторая кривая");
	}
};

class SurfacePatchApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::ISurfacePatchPtr(object)->Edges = Api7Surfaces::Objects(objects, "контур заплатки");
	}
};

class EquidistantSurfaceApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::IEquidistantSurfacePtr(object)->BaseSurface = Api7Surfaces::Objects(objects, "грани эквидистанты");
	}
	void SetValues(double distance, double, bool reverse) override {
		K7::IEquidistantSurfacePtr s = object;
		s->Distance = distance;
		s->Direction = reverse ? VARIANT_TRUE : VARIANT_FALSE;
	}
};

class SurfaceSewerApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::ISurfaceSewerPtr(object)->Shells = Api7Surfaces::Objects(objects, "сшиваемые поверхности");
	}
	void SetValues(double precision, double, bool createBody) override {
		K7::ISurfaceSewerPtr s = object;
		if (precision > 0) s->Precision = precision;
		s->CreateBody = createBody ? VARIANT_TRUE : VARIANT_FALSE;
	}
};

// Проверено на v24: индекс Thickness — «наружу» (TRUE) / «внутрь» (FALSE); толщина в
// направлении ThinType должна быть ненулевой, иначе операция невалидна
class SurfaceThickeningApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::ISurfaceThickeningPtr(object)->Faces = Api7Surfaces::Objects(objects, "грани для толщины");
	}
	void SetValues(double outward, double inward, bool) override {
		K7::IThinParametersPtr thin = object;
		if (!thin) throw Kompas3DException("Не могу получить IThinParameters");
		thin->Thin = VARIANT_TRUE;
		thin->ThinType = outward > 0 && inward > 0 ? KConst3D::dtBoth : inward > 0 ? KConst3D::dtReverse : KConst3D::dtNormal;
		thin->Thickness[VARIANT_TRUE] = outward;
		thin->Thickness[VARIANT_FALSE] = inward;
	}
};

class SurfaceCutApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		if (objects.size() != 1) throw Kompas3DException("Сечение задаётся одной поверхностью");
		K7::ICutPtr(object)->CutObject = Api7Surfaces::Model7(*objects.front(), "секущая поверхность");
	}
	void SetValues(double, double, bool direction) override {
		K7::ICutPtr(object)->Direction = direction ? VARIANT_TRUE : VARIANT_FALSE;
	}
};

class NurbsSurfaceByCurvesMeshApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::INurbsSurfaceByCurvesMeshPtr(object)->UCurves = Api7Surfaces::Objects(objects, "кривые U");
	}
	void SetObjects2(const std::vector<const Node*>& objects) override {
		K7::INurbsSurfaceByCurvesMeshPtr(object)->VCurves = Api7Surfaces::Objects(objects, "кривые V");
	}
};

class TrimmedSurfaceApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::ITrimmedSurfacePtr(object)->Surface = Api7Surfaces::Objects(objects, "усекаемая поверхность");
	}
	void SetObjects2(const std::vector<const Node*>& objects) override {
		K7::ITrimmedSurfacePtr(object)->CutObject = Api7Surfaces::Objects(objects, "секущий объект");
	}
	void SetValues(double, double, bool sense) override {
		K7::ITrimmedSurfacePtr(object)->Sense = sense ? VARIANT_TRUE : VARIANT_FALSE;
	}
};

// Наборы задаются одним вызовом SetObjects(набор1, набор2): первый запоминаем до второго
class SurfacesIntersectionCurveApi7 : public ModelObjectApi7, public ModelObjectsImpl {
	_variant_t first;
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override { first = Api7Surfaces::Objects(objects, "первые поверхности"); }
	void SetObjects2(const std::vector<const Node*>& objects) override {
		K7::ISurfacesIntersectionCurvePtr(object)->SetObjects(first, Api7Surfaces::Objects(objects, "вторые поверхности"));
	}
};

class ProjectionCurveApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::IProjectionCurvePtr(object)->Curves = Api7Surfaces::Objects(objects, "проецируемые кривые");
	}
	void SetObjects2(const std::vector<const Node*>& objects) override {
		K7::IProjectionCurvePtr p = object;
		p->Faces = Api7Surfaces::Objects(objects, "грани проекции");
		p->ProjectionType = KConst3D::ksCPNearest;   // по нормали к граням
	}
};

class Contour3DApi7 : public ModelObjectApi7, public ModelObjectsImpl {
public:
	using ModelObjectApi7::ModelObjectApi7;
	void SetObjects(const std::vector<const Node*>& objects) override {
		K7::IContour3DPtr c = object;
		c->Contour3DType = KConst3D::ksCTAuto;
		for (const Node* n : objects) {
			if (!c->AddEdge(Api7Surfaces::Model7(*n, "ребро контура"))) throw Kompas3DException("КОМПАС не добавил ребро в контур");
		}
	}
};
