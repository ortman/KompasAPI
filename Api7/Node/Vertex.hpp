#pragma once

#include "../../Include/Node/Vertex.h"
#include "../Node.hpp"

class VertexApi7 : public NodeApi7, public Vertex::VertexImpl {
public :
	VertexApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}

	Vertex::Point3D ToPoint() override {
		K5::ksVertexDefinitionPtr vertex = def;
		double x, y, z;
		if (vertex->GetPoint(&x, &y, &z)) {
			return Vertex::Point3D{x, y, z};
		} else {
			return Vertex::Point3D{0, 0, 0};
		}
	}
};
