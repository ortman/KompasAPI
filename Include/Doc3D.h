#ifndef _ComTest_Document3D_h_
#define _ComTest_Document3D_h_

#include <concepts>
#include <string>
#include <memory>

#include "Node.h"
#include "Part.h"
#include "KompasEvent.h"
#include "Node/NodeMacro.h"
//#include "Panel.h"
//#include "Process3D.h"

#define SETTINGS_LINEAR_MAX 1.0
#define SETTINGS_LINEAR_MIN 0.001
#define SETTINGS_ANGLE_MAX  90.0
#define SETTINGS_ANGLE_MIN  0.1
#define SETTINGS_RIDGE_MIN  0.01
#define SETTINGS_RIDGE_MAX  100.0

enum MateType : int {
	MateCoincidence   = 0,  // совпадение объектов
	MateParallel      = 1,  // параллельность
	MatePerpendicular = 2,  // перпендикулярность
	MateTangency      = 3,  // касательность
	MateConcentric    = 4,  // концентричность
	MateDistance      = 5,  // постоянное расстояние между объектами
	MateAngle         = 6,  // постоянный угол между объектами
	MateInPlace       = 7,  // создание компонента "на месте"
	MateTransmission  = 9,  // Механическая передача
	MateCamGear       = 10, // Кулачковый механизм. Кулачек-толкатель
	MateSymmetric     = 11, // Симметрия
	MateDependent     = 14  // Зависимое положение
};

enum MateDir : int {
	MateDirUndefined  = 0,  // направление не учитывается
	MateDirSame       = 1,  // объекты однонаправленные
	MateDirOpposite   = -1  // объекты разнонаправленные
};

enum MateFixed : int {
	MateFixedNone     = 0,  // детали не фиксируются
	MateFixedFirst    = 1,  // фиксируется первая деталь
	MateFixedSecond   = 2   // фиксируется вторая деталь
};

class DocumentFileNotifyLoc;
class Doc3D {
public:
	struct Format {
		enum Value {
			ACIS = 1,
			PARASOLID = 2,
			STEP = 3,
			IGES = 4,
			VRLM = 5,
			STL  = 6,
			JT   = 8,
			STEP_AP203 = 203,
			STEP_AP214 = 214,
			STEP_AP242 = 242
		};
		
		Value value;
		
		Format() : value(STL) {}
		
		constexpr Format(Value v) : value(v) {}
		
		constexpr operator int() const { return value; }
		
		constexpr friend bool operator==(Format lhs, Format rhs) noexcept {
	        return lhs.value == rhs.value;
	    }
	    
		constexpr const char* Ext() const {
			switch (value) {
				case STL : return ".stl";
				case STEP_AP203:
				case STEP_AP214:
				case STEP_AP242:
				case STEP: return ".step";
				case VRLM: return ".vrlm";
				case IGES: return ".igs";
				case ACIS: return ".sat";
				case PARASOLID: return ".x_t";
				case JT  : return ".jt";
				default  : return "";
			}
		}

		Format(const std::string& v) : value(STL) {
			if (v == "STEP") value = STEP;
			else if (v == "VRLM") value = VRLM;
			else if (v == "IGES") value = IGES;
			else if (v == "ACIS") value = ACIS;
			else if (v == "JT")   value = JT;
			else if (v == "PARASOLID")  value = PARASOLID;
			else if (v == "STEP_AP203") value = STEP_AP203;
			else if (v == "STEP_AP214") value = STEP_AP214;
			else if (v == "STEP_AP242") value = STEP_AP242;
		}

		constexpr const char* Name() const {
			switch (value) {
				case STL : return "STL";
				case STEP: return "STEP";
				case VRLM: return "VRLM";
				case IGES: return "IGES";
				case ACIS: return "ACIS";
				case JT  : return "JT";
				case PARASOLID : return "PARASOLID";
				case STEP_AP203: return "STEP_AP203";
				case STEP_AP214: return "STEP_AP214";
				case STEP_AP242: return "STEP_AP242";
				default  : return "";
			}
		}
	};
	
	enum Unit {
		SM = 0,
		MM = 1,
		DM = 2,
		M = 3,
		Document = 4
	};
	
	struct ExportParams {
		Format format;
		bool objBody;
		bool objSurface;
		Unit units;
		bool formatBIN;
		bool isLinear;
		double linearVal;
		bool isAngle;
		double angleVal;
		bool isRidge;
		double ridgeVal;
	};
	
private:
	//IUnknown* pDoc;
	//DocumentFileNotifyLoc *comEvent = nullptr;
	//KProcess3D* proc3D = nullptr;

public:
	class Doc3DImpl {
	public:
		virtual std::string GetPath() { return std::string(); }
		virtual bool SaveAs(const ExportParams& params, const std::string& path) { return false; }
		virtual Part GetTopPart() { return Part(); }
		virtual std::unique_ptr<Node::NodeImpl> GetEditMacroObject() { return nullptr; }
		virtual bool Reopen() { return false; }
		virtual void Close() {}
		virtual int GetEmbodimentsCount() { return 0; }
		virtual std::string GetEmbodimentName(int i) { return std::string(); }
		virtual Part GetEmbodiment(int i) { return Part(); }
		virtual bool SetCurrentEmbodiment(int i) { return false; }
		virtual bool AddMateConstraint(MateType type, const Node& object1, const Node& object2,
		                               MateDir direction, MateFixed fixed, double value) { return false; }
		virtual Part AddPart(const Part& part, const std::optional<std::string>& filePath) { return Part(); }
		virtual ~Doc3DImpl() = default;
	};

	std::unique_ptr<Doc3DImpl> doc;
	KompasEvent<void()> WhenBeginCloseDocument;
	KompasEvent<void()> WhenCloseDocument;
	KompasEvent<void()> WhenBeginSaveDocument;
	KompasEvent<void()> WhenSaveDocument;
	KompasEvent<void()> WhenActiveDocument;
	
	Doc3D() : doc(nullptr) {} // No document
	Doc3D(std::nullptr_t) : doc(nullptr) {} // No document
	Doc3D(std::unique_ptr<Doc3DImpl> p) : doc(std::move(p)) {}
	std::string GetPath() { return doc->GetPath(); }
	Part GetTopPart() { return doc->GetTopPart(); }
	NodeMacro GetEditMacroObject() { return NodeMacro(doc->GetEditMacroObject()); }
	bool SaveAs(const ExportParams& params, const std::string& path) { return doc->SaveAs(params, path); }
	Doc3D& Reopen() { doc->Reopen(); return *this; }
	void Close() { doc->Close(); }
	int GetEmbodimentsCount() { return doc->GetEmbodimentsCount(); }
	std::string GetEmbodimentName(int i) { return doc->GetEmbodimentName(i); }
	Part GetEmbodiment(int i) { return doc->GetEmbodiment(i); }
	bool SetCurrentEmbodiment(int i) { return doc->SetCurrentEmbodiment(i); }
	operator bool() const { return (bool)doc; }
	bool AddMateConstraint(MateType type, const Node& object1, const Node& object2,
	                       MateDir direction, MateFixed fixed = MateFixedNone, double value = 0.0) {
		return doc->AddMateConstraint(type, object1, object2, direction, fixed, value);
	}
	Part AddPart(const Part& part, const std::optional<std::string>& filePath = std::nullopt) {
		return doc->AddPart(part, filePath);
	}
//	template <typename T>
//	T& CreatePorcess() {
//		if (proc3D) delete proc3D;
//		T* proc = new T();
//		proc->Init(this);
//		proc3D = proc;
//		return *proc;
//	}
//	friend class KProcess3D;
};

#endif
