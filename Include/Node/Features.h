#ifndef _KompasAPI_Features_h_
#define _KompasAPI_Features_h_

#include "Sketch.h"

#include <vector>

// Оболочка, зеркальная копия, операции по траектории и по сечениям, ребро жёсткости

// ------------------------------------------------------------------ Shell --
class Shell : public Node {
public:
	class ShellImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetThickness(double thickness) = 0;
		virtual void SetOutward(bool outward) = 0;
		virtual void AddFace(const Node& face) = 0;   // удаляемая (вскрываемая) грань
	};

	static inline int TYPE = 43; /* o3d_shellOperation */
	// faces — грани, которые вскрываются; outward — стенка наружу от исходных граней
	Shell(std::unique_ptr<NodeImpl> p, const std::vector<const Node*>& faces, double thickness, bool outward = false,
	      const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		ShellImpl* shell = dynamic_cast<ShellImpl*>(node.get());
		if (shell) {
			shell->SetThickness(thickness);
			shell->SetOutward(outward);
			for (const Node* face : faces) shell->AddFace(*face);
		}
		node->Create();
	}
	Shell(Node&& node) : Node(std::move(node)) {}
	Shell() : Node(nullptr) {}
};

// ------------------------------------------------------------- MirrorCopy --
class MirrorCopy : public Node {
public:
	class MirrorCopyImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetPlane(const Node& planeOrFace) = 0;
		virtual void AddOperation(const Node& feature) = 0;
	};

	static inline int TYPE = 48; /* o3d_mirrorOperation */
	MirrorCopy(std::unique_ptr<NodeImpl> p, const Node& plane, const std::vector<Node>& features,
	           const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		MirrorCopyImpl* mirror = dynamic_cast<MirrorCopyImpl*>(node.get());
		if (mirror) {
			mirror->SetPlane(plane);
			for (const Node& feature : features) mirror->AddOperation(feature);
		}
		node->Create();
	}
	MirrorCopy(Node&& node) : Node(std::move(node)) {}
	MirrorCopy() : Node(nullptr) {}
};

// ------------------------------------------------- по траектории (Evolution) --
class EvolutionImpl : virtual public Node::NodeImpl {
public:
	virtual void SetSketch(Sketch& profile) = 0;
	virtual void AddPath(const Node& path) = 0;   // эскиз или ребро траектории
};

template <int Type>
class EvolutionNode : public Node {
public:
	static inline int TYPE = Type;
	EvolutionNode(std::unique_ptr<NodeImpl> p, Sketch& profile, const std::vector<const Node*>& path,
	              const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		profile.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		EvolutionImpl* evolution = dynamic_cast<EvolutionImpl*>(node.get());
		if (evolution) {
			evolution->SetSketch(profile);
			for (const Node* n : path) evolution->AddPath(*n);
		}
		node->Create();
	}
	EvolutionNode(Node&& node) : Node(std::move(node)) {}
	EvolutionNode() : Node(nullptr) {}
};

using BaseEvolution = EvolutionNode<45>;   /* o3d_baseEvolution */
using BossEvolution = EvolutionNode<46>;   /* o3d_bossEvolution */

// ------------------------------------------------------- по сечениям (Loft) --
class LoftImpl : virtual public Node::NodeImpl {
public:
	virtual void AddSection(Sketch& section) = 0;
	virtual void SetClosed(bool closed) = 0;
};

template <int Type>
class LoftNode : public Node {
public:
	static inline int TYPE = Type;
	// sections — эскизы сечений по порядку
	LoftNode(std::unique_ptr<NodeImpl> p, const std::vector<Sketch*>& sections, bool closed = false,
	         const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (name.has_value()) node->SetName(name.value());
		LoftImpl* loft = dynamic_cast<LoftImpl*>(node.get());
		if (loft) {
			loft->SetClosed(closed);
			for (Sketch* s : sections) {
				s->EndEdit();
				loft->AddSection(*s);
			}
		}
		node->Create();
	}
	LoftNode(Node&& node) : Node(std::move(node)) {}
	LoftNode() : Node(nullptr) {}
};

using BaseLoft = LoftNode<30>;   /* o3d_baseLoft */
using BossLoft = LoftNode<31>;   /* o3d_bossLoft */
using CutLoft = LoftNode<32>;    /* o3d_cutLoft */

// ------------------------------------------------------------------- Rib --
class Rib : public Node {
public:
	class RibImpl : virtual public Node::NodeImpl {
	public:
		virtual void SetSketch(Sketch& sketch) = 0;
		// side: 0 — в плоскости эскиза, 1 — перпендикулярно (ksRibDefinition::side)
		virtual void SetParams(double thickness, int side) = 0;
	};

	static inline int TYPE = 44; /* o3d_ribOperation */
	Rib(std::unique_ptr<NodeImpl> p, Sketch& sketch, double thickness, int side = 0,
	    const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		sketch.EndEdit();
		if (name.has_value()) node->SetName(name.value());
		RibImpl* rib = dynamic_cast<RibImpl*>(node.get());
		if (rib) {
			rib->SetParams(thickness, side);
			rib->SetSketch(sketch);
		}
		node->Create();
	}
	Rib(Node&& node) : Node(std::move(node)) {}
	Rib() : Node(nullptr) {}
};

#endif
