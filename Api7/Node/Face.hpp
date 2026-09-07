#pragma once

#include "../../Include/Node/Face.h"
#include "../Node.hpp"

class FaceApi7 : public NodeApi7, public Face::FaceImpl {
public :
	FaceApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}

	bool IsPlanar() {
		K5::ksFaceDefinitionPtr face = def;
		return face ? face->IsPlanar() : false;
	}

	bool IsCylinder() {
		K5::ksFaceDefinitionPtr face = def;
		return face ? face->IsCylinder() : false;
	}
};
