#ifndef _KompasAPI_Drawing_h_
#define _KompasAPI_Drawing_h_

#include <array>

#include "Annotation2D.h"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Чертёж КОМПАС (.cdw): лист с основной надписью и ассоциативные виды 3D-модели.
// Координаты на листе — в миллиметрах, начало в левом нижнем углу листа.
class Drawing {
public:
	// Значения ksDocumentFormatEnum
	enum Format { A0 = 0, A1 = 1, A2 = 2, A3 = 3, A4 = 4, A5 = 5, UserFormat = 6 };

	struct Sheet {
		int format = A4;
		bool vertical = true;      // книжная ориентация
		double width = 0;          // размеры листа, мм
		double height = 0;
	};

	struct View {
		long number = 0;           // номер вида в чертеже (0 — системный)
		std::string name;
		bool associative = false;  // ассоциативный вид модели
		std::string source;        // файл модели
		std::string projection;    // имя проекции модели ("#Спереди")
		double x = 0, y = 0;       // точка привязки вида (проекция начала координат модели)
		double scale = 1;
		bool hasBox = false;
		std::array<double, 4> box{};   // габарит на листе: xmin, ymin, xmax, ymax
		bool hasAxes = false;
		// Строки матрицы проекции в осях модели: направление x листа, y листа, на наблюдателя
		std::array<double, 9> axes{};
	};

	struct ViewParams {
		std::string source;        // файл модели (должен быть сохранён)
		std::string projection;    // имя проекции из Doc3D::GetProjections
		double x = 0, y = 0;
		double scale = 1;
		std::string name;          // пусто — имя по умолчанию
		bool showLabel = false;    // надпись вида (имя/масштаб) над изображением
	};

	// Разрез или сечение: линия разреза на базовом виде и новый вид по ней
	struct SectionParams {
		long baseView = 0;
		std::vector<double> points;   // линия разреза в координатах базового вида: x0, y0, x1, y1…
		bool arrowsLeft = true;       // стрелки (направление взгляда) слева по ходу линии
		bool section = false;         // сечение (только фигура в секущей плоскости), иначе разрез
		double x = 0, y = 0;          // точка привязки нового вида на листе
		bool projectionLink = false;  // проекционная связь с базовым видом
		// Направление «вверх» нового вида в осях модели (например +Z): если вид получился
		// перевёрнутым, он строится с другой стороны (стрелки наоборот); пусто — как есть
		std::optional<std::array<double, 3>> up;
		bool flipped = false;         // результат: смотрим с другой стороны
	};

	// Неуказанная шероховатость (правый верхний угол листа)
	struct UnspecifiedRough {
		std::string text;          // «Ra 6,3»
		int sign = 1;              // ksRoughSignEnum, как у Rough2D
		bool addSign = true;       // знак в скобках «(√)» — остальные поверхности
	};

	class DrawingImpl {
	public:
		virtual std::string GetPath() { return std::string(); }
		virtual std::string GetName() { return std::string(); }
		virtual bool IsModified() { return false; }
		virtual Sheet GetSheet() { return Sheet(); }
		virtual bool SetSheet(int format, bool vertical) { return false; }
		virtual std::vector<View> GetViews() { return {}; }
		virtual View AddView(const ViewParams& params) { return View(); }
		virtual View MoveView(long number, double x, double y, double scale) { return View(); }
		virtual bool DeleteView(long number) { return false; }
		virtual View AddSection(SectionParams& params) { return View(); }
		virtual bool Update() { return false; }
		virtual std::vector<SketchItem> GetViewItems(long number) { return {}; }
		virtual DimensionInfo AddDimension(long number, const Dimension2D& dimension) { return DimensionInfo(); }
		virtual bool DeleteViewObject(long number, long ref) { return false; }
		virtual std::vector<DimensionRead> GetDimensions(long number) { return {}; }
		virtual bool SetTolerance(long number, long ref, const DimensionTolerance& t) { return false; }
		virtual long AddRough(long number, const Rough2D& rough) { return 0; }
		virtual std::map<int, std::string> GetStamp(const std::vector<int>& cells) { return {}; }
		virtual int SetStamp(const std::map<int, std::string>& cells) { return 0; }
		virtual std::vector<std::string> GetTechnicalRequirements() { return {}; }
		virtual bool SetTechnicalRequirements(const std::vector<std::string>& lines) { return false; }
		virtual std::vector<std::array<double, 4>> GetTechnicalRequirementsBlocks() { return {}; }
		virtual bool SetTechnicalRequirementsBlocks(const std::vector<std::array<double, 4>>& blocks) { return false; }
		virtual std::optional<UnspecifiedRough> GetUnspecifiedRough() { return std::nullopt; }
		virtual bool SetUnspecifiedRough(const std::optional<UnspecifiedRough>& rough) { return false; }
		virtual bool Save() { return false; }
		virtual bool SaveAs(const std::string& path) { return false; }
		virtual bool SaveImage(const std::string& path, int dpi) { return false; }
		virtual bool ExportPdf(const std::string& path) { return false; }
		virtual void Activate() {}
		virtual void Close() {}
		virtual ~DrawingImpl() = default;
	};

	std::unique_ptr<DrawingImpl> impl;

	Drawing() = default;
	Drawing(std::unique_ptr<DrawingImpl> p) : impl(std::move(p)) {}
	operator bool() const { return (bool)impl; }

	std::string GetPath() { return impl->GetPath(); }
	std::string GetName() { return impl->GetName(); }
	bool IsModified() { return impl->IsModified(); }
	Sheet GetSheet() { return impl->GetSheet(); }
	// Формат и ориентация первого листа
	bool SetSheet(int format, bool vertical) { return impl->SetSheet(format, vertical); }
	// Виды чертежа, кроме системного
	std::vector<View> GetViews() { return impl->GetViews(); }
	// Ассоциативный вид модели; возвращает вид с габаритом после перестроения
	View AddView(const ViewParams& params) { return impl->AddView(params); }
	View MoveView(long number, double x, double y, double scale) { return impl->MoveView(number, x, y, scale); }
	bool DeleteView(long number) { return impl->DeleteView(number); }
	// Разрез или сечение: новый вид с обозначением «А-А», линия разреза — на базовом виде
	View AddSection(SectionParams& params) { return impl->AddSection(params); }
	// Объекты вида (линии проекции и т. п.) в координатах вида: мм модели, без масштаба вида
	std::vector<SketchItem> GetViewItems(long number) { return impl->GetViewItems(number); }
	// Размер в виде (координаты вида), привязанный к его линиям; на чертеже — не управляющий
	DimensionInfo AddDimension(long number, const Dimension2D& dimension) { return impl->AddDimension(number, dimension); }
	// Удалить объект вида (например, размер); false — не найден
	bool DeleteViewObject(long number, long ref) { return impl->DeleteViewObject(number, ref); }
	// Размеры вида с номиналами и допусками
	std::vector<DimensionRead> GetDimensions(long number) { return impl->GetDimensions(number); }
	// Квалитет и/или предельные отклонения размера вида
	bool SetTolerance(long number, long ref, const DimensionTolerance& t) { return impl->SetTolerance(number, ref, t); }
	// Знак шероховатости на линии вида; ссылка на знак или 0
	long AddRough(long number, const Rough2D& rough) { return impl->AddRough(number, rough); }
	// Основная надпись: тексты ячеек по номерам КОМПАСа (1 — наименование, 2 — обозначение,
	// 3 — материал, 9 — организация, 110…115 — фамилии «Разраб.»…«Утв.», 130…135 — даты)
	std::map<int, std::string> GetStamp(const std::vector<int>& cells) { return impl->GetStamp(cells); }
	// Записать ячейки; число записанных (проверено чтением)
	int SetStamp(const std::map<int, std::string>& cells) { return impl->SetStamp(cells); }
	// Технические требования: пункты по порядку, КОМПАС нумерует и размещает над основной надписью
	std::vector<std::string> GetTechnicalRequirements() { return impl->GetTechnicalRequirements(); }
	// Пустой список — удалить технические требования
	bool SetTechnicalRequirements(const std::vector<std::string>& lines) { return impl->SetTechnicalRequirements(lines); }
	// Габариты блоков (страниц) технических требований на листе: xmin, ymin, xmax, ymax
	std::vector<std::array<double, 4>> GetTechnicalRequirementsBlocks() { return impl->GetTechnicalRequirementsBlocks(); }
	// Разместить блоки вручную (авторазмещение выключается); проверяется чтением
	bool SetTechnicalRequirementsBlocks(const std::vector<std::array<double, 4>>& blocks) { return impl->SetTechnicalRequirementsBlocks(blocks); }
	std::optional<UnspecifiedRough> GetUnspecifiedRough() { return impl->GetUnspecifiedRough(); }
	// nullopt — убрать
	bool SetUnspecifiedRough(const std::optional<UnspecifiedRough>& rough) { return impl->SetUnspecifiedRough(rough); }
	// Перестроить ассоциативные виды по текущим файлам моделей
	bool Update() { return impl->Update(); }
	bool Save() { return impl->Save(); }
	bool SaveAs(const std::string& path) { return impl->SaveAs(path); }
	// Растровый снимок листа (PNG)
	bool SaveImage(const std::string& path, int dpi = 100) { return impl->SaveImage(path, dpi); }
	// PDF через конвертер КОМПАСа; чертёж должен быть сохранён в файл
	bool ExportPdf(const std::string& path) { return impl->ExportPdf(path); }
	void Activate() { impl->Activate(); }
	void Close() { impl->Close(); }
};

#endif
