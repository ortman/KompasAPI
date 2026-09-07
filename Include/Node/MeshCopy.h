#ifndef _ComTest_MeshCopy_h_
#define _ComTest_MeshCopy_h_

#include "../Node.h"
#include <vector>

class MeshCopy : public Node {
public:
	class MeshCopyImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetParam1(int count, double step) = 0;
		virtual void SetParam2(int count, double step) = 0;
		virtual void AddNode(const Node& node) = 0;
	};

	static inline int TYPE = 35; /* o3d_meshCopy */
	MeshCopy(std::unique_ptr<NodeImpl> p, int count1, double step1, int count2, double step2, const Node& n, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		MeshCopyImpl* mesh = dynamic_cast<MeshCopyImpl*>(node.get());
		if (mesh) {
			mesh->SetParam1(count1, step1);
			mesh->SetParam2(count2, step2);
			mesh->AddNode(n);
		}
		node->Create();
	}
	MeshCopy(std::unique_ptr<NodeImpl> p, int count1, double step1, int count2, double step2, const std::vector<Node>& nodes, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		MeshCopyImpl* mesh = dynamic_cast<MeshCopyImpl*>(node.get());
		if (mesh) {
			mesh->SetParam1(count1, step1);
			mesh->SetParam2(count2, step2);
			for (const Node& n : nodes) mesh->AddNode(n);
		}
		node->Create();
	}
	//MeshCopy(const Node& node) : Node(node.pEntity, node.pDefinition) {}
	MeshCopy& SetParam1(int count, double step) {
		MeshCopyImpl* mesh = dynamic_cast<MeshCopyImpl*>(node.get());
		if (mesh) mesh->SetParam1(count, step);
		return *this;
	}
	MeshCopy& SetParam2(int count, double step) {
		MeshCopyImpl* mesh = dynamic_cast<MeshCopyImpl*>(node.get());
		if (mesh) mesh->SetParam2(count, step);
		return *this;
	}
};

#endif
