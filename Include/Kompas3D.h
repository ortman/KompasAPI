#ifndef _KOMPAS3D_H_
#define _KOMPAS3D_H_

#include "Doc3D.h"
#include "Drawing.h"
#include "Node/Sketch.h"
#include "Node/BaseExtrusion.h"
#include "Node/BossExtrusion.h"
#include "Node/CutExtrusion.h"
#include "Node/CutEvolution.h"
#include "Node/CutRotated.h"
#include "Node/Rotated.h"
#include "Node/Fillet.h"
#include "Node/Features.h"
#include "Node/MeshCopy.h"
#include "Node/CircularCopy.h"
#include "ThreadSpec.h"
#include "Node/ThreadDesignation.h"
#include "Node/CylindricSpiral.h"

#include "Panel.h"

class Kompas3D {
public:
	class Kompas3DImpl {
	public:
		virtual Doc3D GetActiveDocument3D() { return Doc3D(); }
		virtual Doc3D Open3D(std::string path, bool visible) { return Doc3D(); }
		virtual Doc3D New3D(bool assembly, bool visible) { return Doc3D(); }
		virtual Drawing NewDrawing(bool visible) { return Drawing(); }
		virtual Drawing GetActiveDrawing() { return Drawing(); }
		virtual Drawing OpenDrawing(const std::string& path, bool visible) { return Drawing(); }
		virtual int ActiveDocumentType() { return 0; }
		virtual int SetHideMessage(int mode) { return 0; }
		virtual int CloseAll(bool save) { return 0; }
		virtual std::vector<std::pair<std::string, int>> ListDocuments() { return {}; }
		virtual void Message(const std::string& txt) {}
		virtual void Error(const std::string& txt) {}
		virtual std::string SystemPath(long type) { return std::string(); }
		virtual std::unique_ptr<Panel::PanelImpl> CreatePanel() { return nullptr; }
		virtual ~Kompas3DImpl() = default;
	};

	inline static KompasEvent<void()> WhenConnect;
	inline static KompasEvent<bool(Doc3D& doc, int docType)>  WhenCreateDocument;
	inline static KompasEvent<bool(Doc3D& doc, int docType)>  WhenOpenDocument;
	
	Kompas3D() = delete;
	static Doc3D GetActiveDocument3D() { return kompas->GetActiveDocument3D(); }
	static Doc3D Open3D(std::string path, bool visible = true) { return kompas->Open3D(path, visible); }
	// Новый документ: деталь (assembly = false) или сборка
	static Doc3D New3D(bool assembly = false, bool visible = true) { return kompas->New3D(assembly, visible); }
	// Новый чертёж (лист по умолчанию — A4 книжный с основной надписью)
	static Drawing NewDrawing(bool visible = true) { return kompas->NewDrawing(visible); }
	// Активный документ, если это чертёж; иначе пустой Drawing
	static Drawing GetActiveDrawing() { return kompas->GetActiveDrawing(); }
	// Открыть .cdw (уже открытый — активировать)
	static Drawing OpenDrawing(const std::string& path, bool visible = true) { return kompas->OpenDrawing(path, visible); }
	// Тип активного документа: значение DocumentTypeEnum (0 — нет, 1 — чертёж, 2 — фрагмент,
	// 3 — спецификация, 4 — деталь, 5 — сборка, 6 — текстовый)
	static int ActiveDocumentType() { return kompas->ActiveDocumentType(); }
	// Модальные вопросы КОМПАСа (ksHideMessageEnum): 0 — показывать, 1 — отвечать «Да» без
	// показа, 2 — «Нет». Возвращает прежний режим
	static int SetHideMessage(int mode) { return kompas->SetHideMessage(mode); }
	// Закрыть все документы (save = false — без сохранения); число закрытых
	static int CloseAll(bool save = false) { return kompas->CloseAll(save); }
	// Открытые документы: путь (или имя, если не сохранён) и тип (DocumentTypeEnum)
	static std::vector<std::pair<std::string, int>> ListDocuments() { return kompas->ListDocuments(); }
	static void Message(const std::string& txt) { kompas->Message(txt); }
	static void Error(const std::string& txt) { kompas->Error(txt); }
	static std::string SystemPath(long type) { return kompas->SystemPath(type); }
	static std::unique_ptr<Panel::PanelImpl> CreatePanel() { return kompas->CreatePanel(); }
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

// Определено здесь: панели нужна фабрика реализации из Kompas3D
inline bool Panel::Create() { return Build(Kompas3D::CreatePanel()); }

#endif
