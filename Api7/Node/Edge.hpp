#pragma once

#include "../../Include/Node/Edge.h"
#include "../Node.hpp"
#include "Face.hpp"
#include "Vertex.hpp"

class EdgeApi7 : public NodeApi7, virtual public Edge::EdgeImpl {
public :
	EdgeApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	std::unique_ptr<Node::NodeImpl> GetAdjacentFace(bool left) override {
		K5::ksEdgeDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksEdgeDefinition");
		K5::ksFaceDefinitionPtr faceDef = d->GetAdjacentFace(left);
		if (!faceDef) throw Kompas3DException(left ? "Не могу получить левую грань ребра" : "Не могу получить правую грань ребра");
		K5::ksEntityPtr face = faceDef->GetEntity();
		return std::make_unique<FaceApi7>(face, faceDef);
	}
	std::unique_ptr<Node::NodeImpl> GetVertex(bool begin) override {
		K5::ksEdgeDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksEdgeDefinition");
		K5::ksVertexDefinitionPtr vertexDef = d->GetVertex(begin);
		if (!vertexDef) throw Kompas3DException("Не могу получить вершину ребра");
		return std::make_unique<VertexApi7>(nullptr, vertexDef);
	}
	Vertex::Point3D GetOrigin() override {
		K5::ksEdgeDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksEdgeDefinition");
		K5::ksCurve3DPtr curve = d->GetCurve3D();
		if (!curve) return {0., 0., 0.};
		Vertex::Point3D res;
		if (K5::ksArc3dParamPtr param = curve->GetCurveParam()) {
			K5::ksPlacementPtr pl = param->GetPlacement();
			if (pl && pl->GetOrigin(&res.x, &res.y, &res.z)) return res;
		}
		if (K5::ksCircle3dParamPtr param = curve->GetCurveParam()) {
			K5::ksPlacementPtr pl = param->GetPlacement();
			if (pl && pl->GetOrigin(&res.x, &res.y, &res.z)) return res;
		}
		if (K5::ksEllipse3dParamPtr param = curve->GetCurveParam()) {
			K5::ksPlacementPtr pl = param->GetPlacement();
			if (pl && pl->GetOrigin(&res.x, &res.y, &res.z)) return res;
		}
		return {0., 0., 0.};
	}
};
