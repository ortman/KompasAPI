#ifndef _KompasAPI_CircularCopy_h_
#define _KompasAPI_CircularCopy_h_

#include "../Node.h"
#include "Axis.h"
#include <vector>

class CircularCopy : public Node {
public:
	class CircularCopyImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetCircularParam(int count, double step, bool factor) = 0;
		virtual void SetRadialParam(int count, double step, bool factor) = 0;
		virtual void SetAxis(const Axis& axis) = 0;
		virtual void AddNode(const Node& node) = 0;
	};

	static inline int TYPE = 36; /* o3d_circularCopy */
	CircularCopy(std::unique_ptr<NodeImpl> p,
		int CCount, double CStep, bool CFactor,
		int RCount, double RStep, bool RFactor,
		const Axis& axis,
		const Node& n,
		const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		CircularCopyImpl* copy = dynamic_cast<CircularCopyImpl*>(node.get());
		if (copy) {
			copy->SetCircularParam(CCount, CStep, CFactor);
			copy->SetRadialParam(RCount, RStep, RFactor);
			copy->SetAxis(axis);
			copy->AddNode(n);
		}
		node->Create();
	}
	CircularCopy(std::unique_ptr<NodeImpl> p,
		int CCount, double CStep, bool CFactor,
		int RCount, double RStep, bool RFactor,
		const Axis& axis,
		const std::vector<Node>& nodes,
		const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		CircularCopyImpl* copy = dynamic_cast<CircularCopyImpl*>(node.get());
		if (copy) {
			copy->SetCircularParam(CCount, CStep, CFactor);
			copy->SetRadialParam(RCount, RStep, RFactor);
			copy->SetAxis(axis);
			for (const Node& n : nodes) copy->AddNode(n);
		}
		node->Create();
	}
	//CircularCopy(const Node& node) : Node(node) {}
	CircularCopy& SetCircularParam(int count, double step, bool factor) {
		CircularCopyImpl* copy = dynamic_cast<CircularCopyImpl*>(node.get());
		if (copy) copy->SetCircularParam(count, step, factor);
		return *this;
	}
	CircularCopy& SetRadialParam(int count, double step, bool factor) {
		CircularCopyImpl* copy = dynamic_cast<CircularCopyImpl*>(node.get());
		if (copy) copy->SetRadialParam(count, step, factor);
		return *this;
	}
};

#endif
