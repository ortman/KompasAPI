#ifndef _ComTest_NodeMacro_h_
#define _ComTest_NodeMacro_h_

#include "../Node.h"
#include <vector>

class NodeMacro : public Node {
public:
	class NodeMacroImpl : virtual public Node::NodeImpl {
	public:
		virtual void Show(bool show) = 0;
		virtual void Add(const Node& n) = 0;
		virtual std::vector<Node> GetNodes() = 0;
		virtual bool SetUserParam(void* param, size_t size, int cmd) = 0;
		virtual bool GetUserParam(void* param, size_t size) = 0;
	};
	
public:
	static inline int TYPE = 63; /* o3d_MacroObject */
	NodeMacro() : Node(nullptr) {}
	// Обёртка уже существующего макрообъекта (например, редактируемого)
	NodeMacro(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	NodeMacro(NodeMacro&& other) noexcept = default;
	NodeMacro& operator=(NodeMacro&& other) noexcept = default;
	NodeMacro(std::unique_ptr<NodeImpl> p, bool show, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		if (macro) macro->Show(show);
		node->Create();
	}
	//NodeMacro(const Node& node) : Node(node.pEntity, node.pDefinition) {}
	NodeMacro& Add(const Node& n) {
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		if (macro) macro->Add(n);
		return *this;
	}
	std::vector<Node> GetNodes() {
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		if (macro) return macro->GetNodes();
		return std::vector<Node>();
	}
	bool SetUserParam(void* param, size_t size, int cmd) {
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		return macro && macro->SetUserParam(param, size, cmd);
	}
	bool GetUserParam(void* param, size_t size) {
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		return macro && macro->GetUserParam(param, size);
	}
	NodeMacro& Show(bool show = true) {
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		if (macro) macro->Show(show);
		return *this;
	}
	NodeMacro& Hide() { return Show(false); }
};

#endif
