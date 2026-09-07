#ifndef _ComTest_Axis_h_
#define _ComTest_Axis_h_

#include "Face.h"
#include "Vertex.h"

class Axis : public Node {
private:
	class AxisImpl : virtual public Node::NodeImpl {
	public:
		virtual Vertex::Point3D GetFirstPoint() = 0;
		virtual Vertex::Point3D GetLastPoint() = 0;
	};

public:
	static int TYPE;
	//Axis(const Node& node) : Node(node) {}
	Axis(std::unique_ptr<AxisImpl> p) : Node(std::move(p)) {}
	Vertex::Point3D GetFirstPoint() {
		AxisImpl* vertex = dynamic_cast<AxisImpl*>(node.get());
		return vertex ? vertex->GetFirstPoint() : Vertex::Point3D{0, 0, 0};
	}
	Vertex::Point3D GetLastPoint() {
		AxisImpl* vertex = dynamic_cast<AxisImpl*>(node.get());
		return vertex ? vertex->GetLastPoint() : Vertex::Point3D{0, 0, 0};
	}
};

/*
class ConeAxis : public Axis {
public:
	static int TYPE;
	ConeAxis(const Node& node) : Axis(node.pEntity, node.pDefinition) {}
	ConeAxis(IUnknown* pEntity, IDispatch* pDefinition, const Face& coneFace, bool show = false);
};
*/

#endif
