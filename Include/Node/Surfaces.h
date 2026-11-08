#ifndef _KompasAPI_Surfaces_h_
#define _KompasAPI_Surfaces_h_

#include "Sketch.h"
#include "Axis.h"
#include "Extrusion.h"
#include "Rotated.h"

#include <optional>
#include <string>
#include <vector>

// Каркас и поверхности: пространственные кривые, поверхности, перевод поверхностей в тело.
// Узлы создаются как остальные операции: part.Create<RuledSurface>(curve1, curve2, name).
// Ошибка построения — GetError() != 0 (узел остаётся в дереве, удаляет вызывающий).
//
// Проверено на КОМПАСе v24: поверхность — тело без объёма (ksBody::IsSolid() == FALSE);
// грани поверхностей входят в грани детали; сшивка замкнутого набора с CreateBody даёт тело.

// ------------------------------------------------------ кривые по точкам --
class CurveByPointsImpl : virtual public Node::NodeImpl {
public:
	virtual void SetPoints(const std::vector<Vertex::Point3D>& points, bool closed) = 0;
};

// Сплайн через точки (Spline3D) или ломаная (PolyLine3D) в координатах модели, мм
template <int Type>
class CurveByPoints : public Node {
public:
	static inline int TYPE = Type;
	CurveByPoints(std::unique_ptr<NodeImpl> p, const std::vector<Vertex::Point3D>& points, bool closed,
	              const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		if (CurveByPointsImpl* curve = dynamic_cast<CurveByPointsImpl*>(node.get())) curve->SetPoints(points, closed);
		node->Create();
	}
	CurveByPoints(Node&& node) : Node(std::move(node)) {}
};

using Spline3D = CurveByPoints<55>;     /* o3d_spline */
using PolyLine3D = CurveByPoints<53>;   /* o3d_polyline */

// ------------------------------------------------- поверхности по эскизам --
class SketchSurfaceImpl : virtual public Node::NodeImpl {
public:
	virtual void SetSketch(Sketch& sketch) {}
	virtual void SetExtrusion(const ExtrusionParams& params) {}
	virtual void AddPath(const Node& path) {}           // эскиз, кривая или ребро траектории
	virtual void AddSection(Sketch& section) {}
	virtual void SetClosed(bool closed) {}
};

// Поверхность выдавливания эскиза (профиль может быть незамкнутым)
class ExtrusionSurface : public Node {
public:
	static inline int TYPE = 60;   /* o3d_ExtrusionSurface */
	ExtrusionSurface(std::unique_ptr<NodeImpl> p, Sketch& sketch, const ExtrusionParams& params,
	                 const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		if (SketchSurfaceImpl* s = dynamic_cast<SketchSurfaceImpl*>(node.get())) {
			s->SetExtrusion(params);
			s->SetSketch(sketch);
		}
		node->Create();
	}
	ExtrusionSurface(Node&& node) : Node(std::move(node)) {}
};

// Поверхность вращения: как у тела вращения (ось — Axis или осевая линия эскиза)
using RotatedSurface = RotatedNode<61>;   /* o3d_RotatedSurface */

// Кинематическая поверхность: профиль вдоль траектории (эскиз, 3D-кривая, рёбра)
class EvolutionSurface : public Node {
public:
	static inline int TYPE = 59;   /* o3d_EvolutionSurface */
	EvolutionSurface(std::unique_ptr<NodeImpl> p, Sketch& profile, const std::vector<const Node*>& path,
	                 const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		profile.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		if (SketchSurfaceImpl* s = dynamic_cast<SketchSurfaceImpl*>(node.get())) {
			s->SetSketch(profile);
			for (const Node* n : path) s->AddPath(*n);
		}
		node->Create();
	}
	EvolutionSurface(Node&& node) : Node(std::move(node)) {}
};

// Поверхность по сечениям
class LoftSurface : public Node {
public:
	static inline int TYPE = 62;   /* o3d_LoftSurface */
	LoftSurface(std::unique_ptr<NodeImpl> p, const std::vector<Sketch*>& sections, bool closed,
	            const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		if (SketchSurfaceImpl* s = dynamic_cast<SketchSurfaceImpl*>(node.get())) {
			s->SetClosed(closed);
			for (Sketch* section : sections) {
				section->EndEdit();
				s->AddSection(*section);
			}
		}
		node->Create();
	}
	LoftSurface(Node&& node) : Node(std::move(node)) {}
};

// ------------------------------- поверхности и операции по объектам модели --
// Объекты — кривые, рёбра, грани, поверхности (узлы дерева или топологии)
class ModelObjectsImpl : virtual public Node::NodeImpl {
public:
	virtual void SetObjects(const std::vector<const Node*>& objects) = 0;
	virtual void SetObjects2(const std::vector<const Node*>& objects) {}
	virtual void SetValues(double value1, double value2, bool flag) {}
};

template <int Type>
class ModelObjectsNode : public Node {
public:
	static inline int TYPE = Type;
	ModelObjectsNode(std::unique_ptr<NodeImpl> p, const std::vector<const Node*>& objects,
	                 const std::vector<const Node*>& objects2, double value1, double value2, bool flag,
	                 const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		if (ModelObjectsImpl* m = dynamic_cast<ModelObjectsImpl*>(node.get())) {
			m->SetObjects(objects);
			m->SetObjects2(objects2);
			m->SetValues(value1, value2, flag);
		}
		node->Create();
	}
	ModelObjectsNode(Node&& node) : Node(std::move(node)) {}
};

// Линейчатая поверхность между двумя кривыми (рёбрами): objects — первая, objects2 — вторая
using RuledSurface = ModelObjectsNode<515>;         /* o3d_RuledSurface */
// Заплатка по замкнутому контуру рёбер или кривых
using SurfacePatch = ModelObjectsNode<94>;          /* o3d_SurfacePatch */
// Эквидистанта граней: value1 — расстояние, flag — в обратную сторону
using EquidistantSurface = ModelObjectsNode<514>;   /* o3d_EquidistantSurface */
// Сшивка поверхностей: flag — создать тело (набор должен быть замкнут), value1 — точность, мм
using SurfaceSewer = ModelObjectsNode<96>;          /* o3d_SurfaceSewer */
// Придание толщины граням: value1 — наружу (по нормали), value2 — внутрь, мм
using SurfaceThickening = ModelObjectsNode<518>;    /* o3d_SurfaceThickening */
// Сечение тела поверхностью (ICut): objects — секущая поверхность, flag — сторона
using SurfaceCut = ModelObjectsNode<50>;            /* o3d_cutByPlane: «Сечение» */
// Поверхность по сети кривых: objects — кривые U, objects2 — кривые V (концы сходятся)
using NurbsSurfaceByCurvesMesh = ModelObjectsNode<552>;   /* o3d_NurbsSurfaceByCurvesMesh */
// Усечение поверхности: objects — поверхность, objects2 — секущий объект (поверхность,
// плоскость, кривая), flag — какая часть остаётся
using TrimmedSurface = ModelObjectsNode<516>;       /* o3d_TrimmedSurface */
// Кривая пересечения поверхностей (граней): objects — первый набор, objects2 — второй
using SurfacesIntersectionCurve = ModelObjectsNode<98>;   /* o3d_SurfacesIntersectionCurve */
// Проекция кривых на грани: objects — кривые (эскизы, рёбра), objects2 — грани
using ProjectionCurve = ModelObjectsNode<567>;      /* o3d_ProjectionCurve */
// Контур (кривая) из цепочки рёбер или кривых
using Contour3D = ModelObjectsNode<568>;            /* o3d_Contour3D */

#endif
