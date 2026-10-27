#pragma once

#include "../../Include/Node/ThreadDesignation.h"
#include "../Node.hpp"
#include "Edge.hpp"
#include "Face.hpp"

class ThreadDesignationApi7 : public NodeApi7, public ThreadDesignation::ThreadDesignationImpl {
private:
	static K5::ksEntityPtr EntityOf(const Node& n, const char* what) {
		NodeApi7* n7 = dynamic_cast<NodeApi7*>(n.node.get());
		if (!n7 || !n7->entity) throw Kompas3DException(std::string("Не могу получить объект: ") + what);
		return n7->entity;
	}

	K5::ksThreadDefinitionPtr ThreadDef() {
		K5::ksThreadDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksThreadDefinition");
		return d;
	}

public :
	ThreadDesignationApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	double GetLength() override { return ThreadDef()->length; }
	double GetPitch() override { return ThreadDef()->p; }
	double GetDiameter() override { return ThreadDef()->dr; }
	bool IsOutside() override { return ThreadDef()->outside; }
	// LeftThread читается по имени: в конце IThread, при SDK новее КОМПАСа слота vtable может не быть
	bool IsForwardDir() override {
		K7::IThreadPtr t = ToApi7<K7::IThreadPtr>(entity);
		return t ? !t->LeftThread : true;
	}
	void SetBase(const Node& face) override { ThreadDef()->SetBaseObject(EntityOf(face, "грань резьбы")); }
	void SetStart(const Node& face) override { ThreadDef()->SetFaceBegin(EntityOf(face, "начальная грань резьбы")); }
	void SetDiameter(double diameter) override {
		K5::ksThreadDefinitionPtr d = ThreadDef();
		d->autoDefinDr = diameter <= 0 ? VARIANT_TRUE : VARIANT_FALSE;
		if (diameter > 0) d->dr = diameter;
	}
	void SetPitch(double pitch) override {
		if (pitch > 0) ThreadDef()->p = pitch;
	}
	void SetLength(double length) override {
		K5::ksThreadDefinitionPtr d = ThreadDef();
		d->allLength = length <= 0 ? VARIANT_TRUE : VARIANT_FALSE;
		if (length > 0) d->length = length;
	}
	bool SetLeft(bool left) override {
		K7::IThreadPtr thread = ToApi7<K7::IThreadPtr>(entity);
		if (!thread) return false;
		thread->LeftThread = left ? VARIANT_TRUE : VARIANT_FALSE;
		thread->Update();
		return (thread->LeftThread != VARIANT_FALSE) == left;
	}
	NodeParams GetParamMap() override {
		K5::ksThreadDefinitionPtr d = ThreadDef();
		return {{"length", d->length}, {"pitch", d->p}, {"diameter", d->dr},
		        {"all_length", d->allLength ? 1.0 : 0.0}};
	}
	void SetParamMap(const NodeParams& params) override {
		K5::ksThreadDefinitionPtr d = ThreadDef();
		if (auto it = params.find("all_length"); it != params.end()) d->allLength = it->second != 0 ? VARIANT_TRUE : VARIANT_FALSE;
		if (auto it = params.find("length"); it != params.end() && !d->allLength) d->length = it->second;
		if (auto it = params.find("pitch"); it != params.end()) d->p = it->second;
	}
	std::unique_ptr<Node::NodeImpl> GetBaseFace() override {
		K5::ksEntityPtr face = ThreadDef()->GetBaseObject();
		return face ? std::make_unique<FaceApi7>(face, nullptr) : nullptr;
	}
	// Начальная и конечная границы резьбы — обычно грани-торцы (не рёбра): у торца берём
	// ребро, общее с цилиндрической гранью резьбы. nullptr — такого ребра нет
	std::unique_ptr<Node::NodeImpl> GetEdge(bool begin) override {
		K5::ksThreadDefinitionPtr d = ThreadDef();
		K5::ksEntityPtr bound = begin ? d->GetFaceBegin() : d->GetFaceEnd();
		if (!bound) throw Kompas3DException("Не могу получить ребро условного изображения резьбы");
		IDispatchPtr boundDef = bound->GetDefinition();
		if (K5::ksEdgeDefinitionPtr edge = boundDef) return std::make_unique<EdgeApi7>(bound, edge);
		K5::ksFaceDefinitionPtr face = boundDef;
		if (!face) return nullptr;
		std::optional<Face::Cylinder> cylinder;
		if (K5::ksEntityPtr base = d->GetBaseObject()) cylinder = FaceApi7(base, nullptr).GetCylinder();
		K5::ksEdgeCollectionPtr edges = face->EdgeCollection();
		int count = edges ? edges->GetCount() : 0;
		for (int i = 0; i < count; ++i) {
			K5::ksEdgeDefinitionPtr edge = edges->GetByIndex(i);
			if (!edge || !(edge->IsCircle() || edge->IsArc())) continue;
			K5::ksFaceDefinitionPtr left = edge->GetAdjacentFace(true);
			K5::ksFaceDefinitionPtr right = edge->GetAdjacentFace(false);
			if (!(left && left->IsCylinder()) && !(right && right->IsCylinder())) continue;
			auto result = std::make_unique<EdgeApi7>(edge->GetEntity(), edge);
			if (cylinder && !OnCylinder(*result, *cylinder)) continue;
			return result;
		}
		return nullptr;
	}

private:
	// Окружность ребра лежит на цилиндре: тот же радиус и центр на оси
	static bool OnCylinder(EdgeApi7& edge, const Face::Cylinder& c) {
		const double tol = 1e-3;
		std::optional<double> r = edge.GetRadius();
		if (!r || std::abs(*r - c.radius) > tol) return false;
		Vertex::Point3D o = edge.GetOrigin();
		Vertex::Point3D v{o.x - c.origin.x, o.y - c.origin.y, o.z - c.origin.z};
		Vertex::Point3D a = c.axis;
		double cx = v.y * a.z - v.z * a.y, cy = v.z * a.x - v.x * a.z, cz = v.x * a.y - v.y * a.x;
		return std::sqrt(cx * cx + cy * cy + cz * cz) <= tol;
	}
};
