#pragma once

#include "../../Include/Node/ThreadDesignation.h"
#include "../Node.hpp"
#include "Edge.hpp"

class ThreadDesignationApi7 : public NodeApi7, public ThreadDesignation::ThreadDesignationImpl {
private:
	K5::ksThreadDefinitionPtr ThreadDef() {
		K5::ksThreadDefinitionPtr d = def;
		if (!d) throw Kompas3DException("Не могу получить ksThreadDefinition");
		return d;
	}

public :
	ThreadDesignationApi7(K5::ksEntityPtr e, IDispatchPtr d) : NodeApi7(e, d) {}
	double GetLength() override { return ThreadDef()->length; }
	double GetPitch() override { return ThreadDef()->p; }
	double GetDiameter() override { return ThreadDef()->dr; }
	bool IsOutside() override { return ThreadDef()->outside; }
	bool IsForwardDir() override {
		ThreadDef();
		K7::IThreadPtr iThread = ToApi7<K7::IThreadPtr>(entity);
		return iThread ? !iThread->LeftThread : true;
	}
	std::unique_ptr<Node::NodeImpl> GetEdge(bool begin) override {
		K5::ksThreadDefinitionPtr d = ThreadDef();
		K5::ksEntityPtr edge = begin ? d->GetFaceBegin() : d->GetFaceEnd();
		if (!edge) throw Kompas3DException("Не могу получить ребро условного изображения резьбы");
		return std::make_unique<EdgeApi7>(edge, nullptr);
	}
};
