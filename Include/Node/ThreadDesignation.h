#ifndef _Kompas3DPrint_ThreadDesignation_h_
#define _Kompas3DPrint_ThreadDesignation_h_

#include "Edge.h"

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
	};

	static inline int TYPE = 58; /* o3d_thread */
	ThreadDesignation(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	// Перехват уже существующего узла
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
	bool IsOutside() const {
		ThreadDesignationImpl* thread = dynamic_cast<ThreadDesignationImpl*>(node.get());
		return thread ? thread->IsOutside() : false;
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
