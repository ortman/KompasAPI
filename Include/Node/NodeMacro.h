#ifndef _ComTest_NodeMacro_h_
#define _ComTest_NodeMacro_h_

#include "../Node.h"
#include <vector>

class NodeMacro : public Node {
public:
	class NodeMacroImpl : virtual public Node::NodeImpl {
	public:
		virtual void Show(bool show) = 0;
		virtual ~NodeMacroImpl() = default;
	};
	
public:
	static inline int TYPE = 63; /* o3d_MacroObject */
	NodeMacro(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {
		node->Create();
	}
	NodeMacro(std::unique_ptr<NodeImpl> p, bool show = true, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		if (macro) macro->Show(show);
		node->Create();
	}
	//NodeMacro(const Node& node) : Node(node.pEntity, node.pDefinition) {}
	//NodeMacro(IUnknown* pEntity, IDispatch* pDefinition = NULL, bool show = true, const std::optional<std::string>& name = std::nullopt);
	//NodeMacro& Add(Node node);
	//std::vector<Node> GetNodes();
	//bool SetUserParam(void* param, size_t size, int cmd);
	//bool GetUserParam(void* param, size_t size);
	NodeMacro& Show(bool show = true) {
		NodeMacroImpl* macro = dynamic_cast<NodeMacroImpl*>(node.get());
		if (macro) macro->Show(show);
		return *this;
	}
	NodeMacro& Hide() { return Show(false); }
};

#endif
