#ifndef _Kompas3DPrint_CylindricSpiral_h_
#define _Kompas3DPrint_CylindricSpiral_h_

#include "Plane.h"

class CylindricSpiral : public Node {
public:
	enum Mode {
		ModeStepHeight = 1
	};

	class CylindricSpiralImpl : virtual public Node::NodeImpl {
	public:
		virtual void Show(bool show) = 0;
		virtual void SetDiam(double d) = 0;
		virtual void SetStep(double step) = 0;
		virtual void SetHeight(double height) = 0;
		virtual void TurnDir(bool dir) = 0;
		virtual void BuildDir(bool dir) = 0;
		virtual void BuildMode(Mode mode) = 0;
		virtual void SetPlane(const Plane& plane) = 0;
		virtual void SetLocation(double x, double y) = 0;
		virtual std::unique_ptr<Node::NodeImpl> GetBeginVertex() = 0;
	};

	static inline int TYPE = 56; /* o3d_cylindricSpiral */
	// Create() вызывается явно, после настройки параметров
	CylindricSpiral(std::unique_ptr<NodeImpl> p) : Node(std::move(p)) {}
	//CylindricSpiral(const Node& node) : Node(node) {}
	CylindricSpiral& SetDiam(double d) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->SetDiam(d);
		return *this;
	}
	CylindricSpiral& SetStep(double step) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->SetStep(step);
		return *this;
	}
	CylindricSpiral& SetHeight(double height) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->SetHeight(height);
		return *this;
	}
	CylindricSpiral& TurnDir(bool dir) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->TurnDir(dir);
		return *this;
	}
	CylindricSpiral& BuildDir(bool dir) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->BuildDir(dir);
		return *this;
	}
	CylindricSpiral& BuildMode(Mode mode) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->BuildMode(mode);
		return *this;
	}
	CylindricSpiral& SetPlane(const Plane& plane) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->SetPlane(plane);
		return *this;
	}
	CylindricSpiral& SetLocation(double x, double y) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->SetLocation(x, y);
		return *this;
	}
	CylindricSpiral& Show(bool show = true) {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		if (spiral) spiral->Show(show);
		return *this;
	}
	CylindricSpiral& Hide() { return Show(false); }
	CylindricSpiral& Create(bool show = false) {
		Show(show);
		node->Create();
		return *this;
	}
	Vertex GetBeginVertex() const {
		CylindricSpiralImpl* spiral = dynamic_cast<CylindricSpiralImpl*>(node.get());
		return Vertex(spiral ? spiral->GetBeginVertex() : nullptr);
	}
};

#endif
