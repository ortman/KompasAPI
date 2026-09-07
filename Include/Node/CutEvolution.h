#ifndef _Kompas3DPrint_CutEvolution_h_
#define _Kompas3DPrint_CutEvolution_h_

#include "Sketch.h"
#include <vector>

class CutEvolution : public Node {
public:
	class CutEvolutionImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetSketch(Sketch& sketch) = 0;
		virtual void AddPath(const Node& node) = 0;
	};

	static inline int TYPE = 47; /* o3d_cutEvolution */
	CutEvolution(std::unique_ptr<NodeImpl> p, Sketch& sketch, const Node& n, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		CutEvolutionImpl* evolution = dynamic_cast<CutEvolutionImpl*>(node.get());
		if (evolution) {
			evolution->SetSketch(sketch);
			evolution->AddPath(n);
		}
		node->Create();
	}
	CutEvolution(std::unique_ptr<NodeImpl> p, Sketch& sketch, const std::vector<Node>& nodes, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		CutEvolutionImpl* evolution = dynamic_cast<CutEvolutionImpl*>(node.get());
		if (evolution) {
			evolution->SetSketch(sketch);
			for (const Node& n : nodes) evolution->AddPath(n);
		}
		node->Create();
	}
	//CutEvolution(const Node& node) : Node(node) {}
};

#endif
