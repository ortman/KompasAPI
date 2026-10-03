#ifndef _ComTest_Sketch_h_
#define _ComTest_Sketch_h_

#include <unknwn.h>
#include <memory>
#include <vector>
#include "Plane.h"
#include "Face.h"
#include "../Graphic2D.h"
#include "../Annotation2D.h"


class Sketch : public Node {
public:
	// Система координат эскиза в модели
	struct Placement {
		Vertex::Point3D origin, x, y, z;
	};

	class SketchImpl : virtual public Node::NodeImpl {
	public:
		virtual std::optional<Placement> GetPlacement() { return std::nullopt; }
		// Все объекты эскиза; неподдерживаемые — с Kind::Other
		virtual std::vector<SketchItem> GetItems() { return {}; }
		// Удалить все объекты эскиза (эскиз остаётся открытым на редактирование)
		virtual void DeleteAll() {}
		// Ограничения и размеры (эскиз открывается на редактирование)
		virtual long LastObject() { return 0; }
		virtual bool Parametrize(const std::vector<long>& refs, const Parametrize2D& options) { return false; }
		virtual bool AddConstraint(Constraint2D type, long a, int pointA, long b, int pointB) { return false; }
		virtual DimensionInfo AddDimension(const Dimension2D& dimension) { return DimensionInfo(); }
		virtual ObjectDefinition GetObjectDefinition(long ref) { return ObjectDefinition::Unknown; }
		virtual std::vector<long> Project(const Node& modelObject) { return {}; }
		virtual long ProjectOrigin() { return 0; }
		virtual std::vector<long> ProjectSupport() { return {}; }
		virtual std::unique_ptr<Node::NodeImpl> GetSupportFace() { return nullptr; }
		virtual bool DeleteObject(long ref) { return false; }
		virtual SketchDefinition GetDefinition() { return SketchDefinition::Unknown; }
		virtual void SetPlane(const Plane& plane) = 0;
		virtual void SetAngle(double angle) = 0;
		virtual void SetLocation(double locX, double locY) = 0;
		virtual Plane::Point2D Projection(const Vertex::Point3D& point) = 0;
		virtual void BeginEdit() = 0;
		virtual void EndEdit() = 0;
		virtual bool IsEdit() = 0;
		virtual void Clear() = 0;
		virtual void Line(double x1, double y1, double x2, double y2, LineStyle style) = 0;
		virtual void LineTo(double x, double y, LineStyle style) = 0;
		virtual void Circle(double cx, double cy, double r, LineStyle style) = 0;
		virtual void Rect(double x, double y, double w, double h, double angle, LineStyle style) = 0;
		virtual void RegularPolygon(double cx, double cy, double r, int count, bool describe, double angle, LineStyle style) = 0;
		virtual void Point(double x, double y, LineStyle style) = 0;
		virtual void ArcByAngle(double cx, double cy, double r, double f1, double f2, bool cw, LineStyle style) = 0;
		virtual void ArcByPoint(double cx, double cy, double r, double x1, double y1, double x2, double y2, bool cw, LineStyle style) = 0;
		virtual void ArcBy3Points(double x1, double y1, double x2, double y2, double x3, double y3, LineStyle style) = 0;
		virtual void Ellipse(double cx, double cy, double a, double b, double angle, LineStyle style) = 0;
		virtual void EllipseArc(double cx, double cy, double a, double b, double a1, double a2, bool cw, double angle, LineStyle style) = 0;
	};
	static inline int TYPE = 5; /* o3d_sketch */
	Sketch() : Node(nullptr) {}
	// Перехват уже существующего узла
	Sketch(Node&& node) : Node(std::move(node)) {}
	Sketch(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {
		node->Create();
	}
	Sketch(std::unique_ptr<NodeImpl> p, const Plane& plane, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->SetPlane(plane);
		node->Create();
	}
	Sketch(std::unique_ptr<NodeImpl> p, const Plane& plane, double angle, double locX = 0.0, double locY = 0.0, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) {
			sketch->SetPlane(plane);
			sketch->SetAngle(angle);
			sketch->SetLocation(locX, locY);
		}
		node->Create();
	}
	Sketch(Node& node) : Node(std::move(node.node)) {}
	std::vector<SketchItem> GetItems() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->GetItems() : std::vector<SketchItem>();
	}
	Sketch& DeleteAll() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->DeleteAll();
		return *this;
	}
	// Нарисовать объект, прочитанный GetItems (например, чтобы вернуть прежний профиль)
	Sketch& Draw(const SketchItem& item) {
		const std::vector<double>& v = item.v;
		switch (item.kind) {
			case SketchItem::Kind::Line: return Line(v[0], v[1], v[2], v[3], item.style);
			case SketchItem::Kind::Circle: return Circle(v[0], v[1], v[2], item.style);
			case SketchItem::Kind::Arc: return ArcByPoint(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7] < 0, item.style);
			case SketchItem::Kind::Point: return Point(v[0], v[1], item.style);
			case SketchItem::Kind::Ellipse: return Ellipse(v[0], v[1], v[2], v[3], v[4], item.style);
			case SketchItem::Kind::EllipseArc: return EllipseArc(v[0], v[1], v[2], v[3], v[5], v[6], v[7] < 0, v[4], item.style);
			case SketchItem::Kind::Rect: return Rect(v[0], v[1], v[2], v[3], v[4], item.style);
			case SketchItem::Kind::Polygon: return RegularPolygon(v[0], v[1], v[2], (int)v[3], v[4] != 0, v[5], item.style);
			default: return *this;
		}
	}
	// Ссылка на последний нарисованный объект (Line, Circle, Rect…)
	long LastObject() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->LastObject() : 0;
	}
	// Параметризовать объекты (пустой список — все объекты эскиза): совпадение точек,
	// горизонталь, вертикаль, параллельность, перпендикулярность
	bool Parametrize(const std::vector<long>& refs = {}, const Parametrize2D& options = Parametrize2D()) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch && sketch->Parametrize(refs, options);
	}
	// Ограничение объекта a (точка pointA; -1 — объект целиком) относительно b (pointB)
	bool AddConstraint(Constraint2D type, long a, int pointA = -1, long b = 0, int pointB = -1) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch && sketch->AddConstraint(type, a, pointA, b, pointB);
	}
	// Размер, привязанный к геометрии эскиза (точки размера должны совпадать с её точками)
	DimensionInfo AddDimension(const Dimension2D& dimension) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->AddDimension(dimension) : DimensionInfo();
	}
	ObjectDefinition GetObjectDefinition(long ref) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->GetObjectDefinition(ref) : ObjectDefinition::Unknown;
	}
	// Спроецировать в эскиз ребро, грань или вершину модели; ссылки на созданные объекты
	std::vector<long> Project(const Node& modelObject) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->Project(modelObject) : std::vector<long>();
	}
	// Спроецировать в эскиз начало координат модели — неподвижная точка для размеров положения
	long ProjectOrigin() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->ProjectOrigin() : 0;
	}
	// Если эскиз лежит на грани — спроецировать её контур (опоры для привязок и размеров)
	std::vector<long> ProjectSupport() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->ProjectSupport() : std::vector<long>();
	}
	// Грань, на которой лежит эскиз (пусто, если эскиз на плоскости)
	Face GetSupportFace() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return Face(sketch ? sketch->GetSupportFace() : nullptr);
	}
	// Удалить объект эскиза; false — объект не найден (результат Delete у КОМПАСа ненадёжен)
	bool DeleteObject(long ref) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch && sketch->DeleteObject(ref);
	}
	// Определённость эскиза (по текущему сеансу редактирования)
	SketchDefinition GetDefinition() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->GetDefinition() : SketchDefinition::Unknown;
	}
	// Начало и оси эскиза в координатах модели (z — нормаль эскиза)
	std::optional<Placement> GetPlacement() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->GetPlacement() : std::nullopt;
	}
	Plane::Point2D Projection(const Vertex::Point3D& point) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->Projection(point) : Plane::Point2D{0., 0.};
	}
	Sketch& BeginEdit() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->BeginEdit();
		return *this;
	}
	void EndEdit() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->EndEdit();
	}
	bool IsEdit() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		return sketch ? sketch->IsEdit() : false;
	}
	Sketch& Clear() {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->Clear();
		return *this;
	}
	Sketch& Line(double x1, double y1, double x2, double y2, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->Line(x1, y1, x2, y2, style);
		return *this;
	}
	Sketch& LineTo(double x, double y, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->LineTo(x, y, style);
		return *this;
	}
	Sketch& Circle(double cx, double cy, double r, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->Circle(cx, cy, r, style);
		return *this;
	}
	Sketch& Rect(double x, double y, double w, double h, double angle = 0.0, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->Rect(x, y, w, h, angle, style);
		return *this;
	}
	Sketch& RegularPolygon(double cx, double cy, double r, int count, bool describe = true, double angle = 0.0, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->RegularPolygon(cx, cy, r, count, describe, angle, style);
		return *this;
	}
	Sketch& Point(double x, double y, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->Point(x, y, style);
		return *this;
	}
	Sketch& ArcByAngle(double cx, double cy, double r, double f1, double f2, bool cw, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->ArcByAngle(cx, cy, r, f1, f2, cw, style);
		return *this;
	}
	Sketch& ArcByPoint(double cx, double cy, double r, double x1, double y1, double x2, double y2, bool cw, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->ArcByPoint(cx, cy, r, x1, y1, x2, y2, cw, style);
		return *this;
	}
	Sketch& ArcBy3Points(double x1, double y1, double x2, double y2, double x3, double y3, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->ArcBy3Points(x1, y1, x2, y2, x3, y3, style);
		return *this;
	}
	Sketch& Ellipse(double cx, double cy, double a, double b, double angle = 0.0, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->Ellipse(cx, cy, a, b, angle, style);
		return *this;
	}
	Sketch& EllipseArc(double cx, double cy, double a, double b, double a1, double a2, bool cw, double angle = 0.0, LineStyle style = LineStyle::Main) {
		SketchImpl* sketch = dynamic_cast<SketchImpl*>(node.get());
		if (sketch) sketch->EllipseArc(cx, cy, a, b, a1, a2, cw, angle, style);
		return *this;
	}

	//ksLine
	//ksColouring
	//ksConicArc
	//ksEquidistant
	//ksHatch
	//ksInsertRaster
	//ksParEllipseArc
	//ksPointArraw
};

#endif
