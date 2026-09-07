#pragma once

#include "../Kompas3D.hpp"
#include "../../Include/Node/Sketch.h"
#include "Plane.hpp"

class SketchApi7 : public NodeApi7, public Sketch::SketchImpl {
private:
	K5::ksDocument2DPtr doc2D = nullptr;
	double lastX = 0.0;
	double lastY = 0.0;

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
	void BeginEdit() override {
		if (doc2D) return;
		K5::ksSketchDefinitionPtr d = def;
		doc2D = d->BeginEdit();
		if (!doc2D) throw Kompas3DException("Не могу открыть эскиз!");
	}
	void EndEdit() override {
		if (doc2D) {
			K5::ksSketchDefinitionPtr d = def;
			d->EndEdit();
			doc2D = nullptr;
		}
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
		doc2D->ksLineSeg(x1, y1, x2, y2, (int)style);
		lastX = x2;
		lastY = y2;
	}
	void LineTo(double x, double y, LineStyle style) override {
		Line(lastX, lastY, x, y, style);
	}
	void Circle(double cx, double cy, double r, LineStyle style) override {
		BeginEdit();
		doc2D->ksCircle(cx, cy, r, (int)style);
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
		doc2D->ksRectangle(param, 0);
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
		param->style = (int)style;
		doc2D->ksRegularPolygon(param, 0);
		lastX = cx;
		lastY = cy;
	}
	void Point(double x, double y, LineStyle style) override {
		BeginEdit();
		doc2D->ksPoint(x, y, (int)style);
		lastX = x;
		lastY = y;
	}
	void ArcByAngle(double cx, double cy, double r, double f1, double f2, bool cw, LineStyle style) override {
		BeginEdit();
		doc2D->ksArcByAngle(cx, cy, r, f1, f2, cw ? -1 : 1, (int)style);
		lastX = cx;
		lastY = cy;
	}
	void ArcByPoint(double cx, double cy, double r, double x1, double y1, double x2, double y2, bool cw, LineStyle style) override {
		BeginEdit();
		doc2D->ksArcByPoint(cx, cy, r, x1, y1, x2, y2, cw ? -1 : 1, (int)style);
		lastX = cx;
		lastY = cy;
	}
	void ArcBy3Points(double x1, double y1, double x2, double y2, double x3, double y3, LineStyle style) override {
		BeginEdit();
		doc2D->ksArcBy3Points(x1, y1, x2, y2, x3, y3, (int)style);
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
		doc2D->ksEllipse(param);
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
		doc2D->ksEllipseArc(param);
		lastX = cx;
		lastY = cy;
	}
};
