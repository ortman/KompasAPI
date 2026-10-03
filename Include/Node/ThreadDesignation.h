#ifndef _Kompas3DPrint_ThreadDesignation_h_
#define _Kompas3DPrint_ThreadDesignation_h_

#include "Edge.h"
#include "Face.h"
#include "../ThreadSpec.h"

#include <optional>
#include <string>

class ThreadDesignation : public Node {
public:
	class ThreadDesignationImpl : virtual public Node::NodeImpl {
	public:
		virtual double GetLength() = 0;
		virtual double GetPitch() = 0;
		virtual double GetDiameter() = 0;
		virtual bool IsForwardDir() = 0;
		virtual bool IsOutside() = 0;
		virtual std::unique_ptr<Node::NodeImpl> GetEdge(bool begin) = 0;
		virtual std::unique_ptr<Node::NodeImpl> GetBaseFace() { return nullptr; }
		// Построение
		virtual void SetBase(const Node& face) = 0;     // цилиндрическая (коническая) грань
		virtual void SetStart(const Node& face) = 0;    // плоская грань, от которой идёт резьба
		virtual void SetDiameter(double diameter) = 0;  // 0 — номинальный диаметр по грани
		virtual void SetPitch(double pitch) = 0;        // 0 — оставить шаг КОМПАСа
		virtual void SetLength(double length) = 0;      // 0 — на всю длину грани
		virtual bool SetLeft(bool left) = 0;            // после создания
	};

	struct Params {
		double diameter = 0;   // номинальный, мм; 0 — КОМПАС подбирает по грани
		double pitch = 0;      // мм; 0 — шаг по умолчанию для диаметра
		double length = 0;     // мм; 0 — на всю длину грани
		bool left = false;
	};

	static inline int TYPE = 58; /* o3d_thread */
	ThreadDesignation(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	// Условное изображение резьбы на цилиндрической грани face; start — плоская грань-торец,
	// от которой отсчитывается длина (nullptr — выбирает КОМПАС)
	ThreadDesignation(std::unique_ptr<NodeImpl> p, const Node& face, const Node* start, const Params& params,
	                  const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		if (!thread) throw Kompas3DException("Не могу создать условное изображение резьбы");
		thread->SetBase(face);
		if (start) thread->SetStart(*start);
		thread->SetDiameter(params.diameter);
		thread->SetPitch(params.pitch);
		thread->SetLength(params.length);
		node->Create();
		if (params.left && !thread->SetLeft(true)) {
			throw Kompas3DException("КОМПАС не поддерживает левую резьбу через API");
		}
	}
	// Перехват уже существующего узла
	// Резьба по стандарту; имя по умолчанию — обозначение по стандарту (M8, M10×1,25LH,
	// G1/2, 1/4-20 UNC). length = 0 — на всю длину грани
	ThreadDesignation(std::unique_ptr<NodeImpl> p, const Node& face, const Node* start, const ThreadSpec& spec,
	                  double length = 0, const std::optional<std::string>& name = std::nullopt)
		: ThreadDesignation(std::move(p), face, start, Params{spec.diameter, spec.pitch, length, spec.left},
		                    name.value_or(spec.Designation())) {}
	ThreadDesignation(Node&& node) : Node(std::move(node)) {}
	ThreadDesignation(Node& node) : Node(std::move(node.node)) {}
	double GetLength() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return thread ? thread->GetLength() : 0.0;
	}
	double GetPitch() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return thread ? thread->GetPitch() : 0.0;
	}
	double GetDiameter() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return thread ? thread->GetDiameter() : 0.0;
	}
	bool IsForwardDir() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return thread ? thread->IsForwardDir() : true;
	}
	// Резьба из рядов стандартов по диаметру и шагу (иначе — нестандартная метрическая)
	ThreadSpec GetSpec() const { return ThreadSpec::FromMeasured(GetDiameter(), GetPitch(), !IsForwardDir()); }
	bool IsOutside() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return thread ? thread->IsOutside() : false;
	}
	// Цилиндрическая грань, на которой построена резьба (ось и радиус — Face::GetCylinder)
	Face GetBaseFace() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return Face(thread ? thread->GetBaseFace() : nullptr);
	}
	Edge GetBeginEdge() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return Edge(thread ? thread->GetEdge(true) : nullptr);
	}
	Edge GetEndEdge() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return Edge(thread ? thread->GetEdge(false) : nullptr);
	}
};

#endif
