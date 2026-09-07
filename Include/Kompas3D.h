#ifndef _KOMPAS3D_H_
#define _KOMPAS3D_H_

#include "Doc3D.h"
#include "Node/Sketch.h"
//#include "Panel.h"

class Kompas3D {
public:
	class Kompas3DImpl {
	public:
		virtual Doc3D GetActiveDocument3D() { return Doc3D(); }
		virtual void Message(const std::string& txt) {}
		virtual void Error(const std::string& txt) {}
		virtual std::string SystemPath(long type) { return std::string(); }
		virtual ~Kompas3DImpl() = default;
	};

	inline static KompasEvent<bool(Doc3D& doc, int docType)>  WhenCreateDocument;
	inline static KompasEvent<bool(Doc3D& doc, int docType)>  WhenOpenDocument;
	
	Kompas3D() = delete;
	static Doc3D GetActiveDocument3D() { return kompas->GetActiveDocument3D(); }
	static Doc3D Open3D(std::string path, bool visible = true);
/*
 * 	template <typename T>
 * 	static T GetParamStruct(int type);
 * 	template <typename T>
 * 	static T ToApi7(IUnknown* k5);
 * 	template <typename T>
 * 	static T ToApi5(IUnknown* k7);
 */
	static void Message(const std::string& txt) { kompas->Message(txt); }
	static void Error(const std::string& txt) { kompas->Error(txt); }
	static std::string SystemPath(long type) { return kompas->SystemPath(type); }
	static std::string ConfigPath() { return SystemPath(3 /*ksConfigurations*/); }
	static void RunCommand(uint32_t comm);
	static std::string Cp1251ToUtf8(const char* cp1251Str);
	static std::string Utf8ToCp1251(const std::string& utf8Str);
	static void SetKompas(std::unique_ptr<Kompas3DImpl> app) { kompas = std::move(app); }
	static bool ComConnect(bool open = false, bool visible = true);
	static void ComDisconnect();
	
//	friend class Panel;
//	friend class KProcess3D;

private:
	inline static std::unique_ptr<Kompas3DImpl> kompas = std::make_unique<Kompas3DImpl>();
//	static IUnknown* CreatePropertyManager();
//	static IUnknown* CreateProcessParam();
};

#endif
