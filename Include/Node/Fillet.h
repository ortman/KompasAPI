#ifndef _KompasAPI_Fillet_h_
#define _KompasAPI_Fillet_h_

#include "../Node.h"

#include <vector>

// Скругление и фаска по рёбрам (или граням: тогда обрабатываются все их рёбра)

class Fillet : public Node {
public:
	class FilletImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetRadius(double radius) = 0;
		virtual void SetTangent(bool tangent) = 0;
		virtual void Add(const Node& edgeOrFace) = 0;
	};

	static inline int TYPE = 34; /* o3d_fillet */
	// tangent — продолжать по касательным рёбрам
	Fillet(std::unique_ptr<NodeImpl> p, const std::vector<const Node*>& objects, double radius, bool tangent = true,
	       const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		FilletImpl* fillet = dynamic_cast<FilletImpl*>(node.get());
		if (fillet) {
			fillet->SetRadius(radius);
			fillet->SetTangent(tangent);
			for (const Node* object : objects) fillet->Add(*object);
		}
		node->Create();
	}
	Fillet(Node&& node) : Node(std::move(node)) {}
	Fillet() : Node(nullptr) {}
};

class Chamfer : public Node {
public:
	class ChamferImpl : virtual public Node::NodeImpl {
	public:
		// distance2 — второй катет; transfer — поменять катеты местами
		virtual void SetParams(double distance1, double distance2, bool transfer) = 0;
		virtual void SetTangent(bool tangent) = 0;
		virtual void Add(const Node& edgeOrFace) = 0;
	};

	static inline int TYPE = 33; /* o3d_chamfer */
	Chamfer(std::unique_ptr<NodeImpl> p, const std::vector<const Node*>& objects, double distance1, double distance2,
	        bool tangent = true, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		ChamferImpl* chamfer = dynamic_cast<ChamferImpl*>(node.get());
		if (chamfer) {
			chamfer->SetParams(distance1, distance2, false);
			chamfer->SetTangent(tangent);
			for (const Node* object : objects) chamfer->Add(*object);
		}
		node->Create();
	}
	Chamfer(Node&& node) : Node(std::move(node)) {}
	Chamfer() : Node(nullptr) {}
};

#endif
