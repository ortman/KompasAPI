#ifndef _ComTest_Part_h_
#define _ComTest_Part_h_

#include "Node.h"
#include "Node/Plane.h"
#include "Node/Axis.h"
#include "Node/Edge.h"
#include <vector>
#include <utility>
#include <optional>
#include <string>
#include <variant>

class Part {
public:
	struct Variable {
		bool isExternal;
		double value;
		std::string name;
		std::string comment;
	};
	struct BoundingBox {
		double x1, y1, z1, x2, y2, z2;   // мм
	};
	struct MassProperties {
		double mass;       // кг
		double volume;     // мм³
		double area;       // площадь поверхности, мм²
		double xc, yc, zc; // центр масс, мм
		double density;    // плотность материала, г/см³
		std::string material;
	};
	// Результат измерения между двумя объектами (грани, рёбра, вершины)
	struct Measurement {
		double distance;                // минимальное расстояние, мм
		std::optional<double> angle;    // угол, если определён, градусы
		Vertex::Point3D point1, point2; // ближайшие точки
	};
	// Положение компонента в сборке
	struct Placement {
		Vertex::Point3D origin, x, y, z;
	};
	class PartImpl {
	public:
		virtual std::string Name() { return std::string(); }
		virtual std::unique_ptr<Node::NodeImpl> CreateImpl(int type) { return nullptr; }
		virtual Plane GetPlane(int type) = 0;
		virtual Axis GetAxis(int type) = 0;
		virtual std::vector<Node> GetNodes() = 0;
		virtual std::vector<Variable> GetVariables(bool isExternal) = 0;
		virtual void Remove(const Node& node) = 0;
		virtual std::optional<BoundingBox> GetBoundingBox() { return std::nullopt; }
		virtual int CheckIntersection(Part& other, bool tangent) { return -1; }
		virtual std::optional<MassProperties> GetMassProperties() { return std::nullopt; }
		virtual int GetBodiesCount() { return 0; }
		virtual bool SetVariable(const std::string& name, double value) { return false; }
		virtual bool SetVariableExpression(const std::string& name, const std::string& expression) { return false; }
		virtual bool Rebuild() { return false; }
		virtual std::vector<Face> GetFaces() { return {}; }
		virtual std::optional<Measurement> Measure(const Node& a, const Node& b) { return std::nullopt; }
		virtual std::string GetFileName() { return std::string(); }
		virtual std::string GetDesignation() { return std::string(); }
		virtual bool SetTitle(const std::string& name, const std::string& designation) { return false; }
		virtual std::optional<std::string> GetSystemProperty(int id) { return std::nullopt; }
		virtual bool SetSystemProperty(int id, const std::variant<bool, double, std::string>& value) { return false; }
		virtual bool IsStandard() { return false; }
		virtual std::optional<std::pair<double, double>> GetHatch() { return std::nullopt; }
		virtual bool SetHatch(double angle, double step) { return false; }
		virtual bool IsFixed() { return false; }
		virtual void SetFixed(bool fixed) {}
		virtual std::optional<Placement> GetPlacement() { return std::nullopt; }
		virtual bool SetPlacement(const Placement& placement) { return false; }
		virtual std::vector<Edge> GetEdges() { return {}; }
		virtual ~PartImpl() = default;
	};

	std::unique_ptr<PartImpl> part;

private:
	//IUnknown* pDoc;
	//IUnknown* CreateEntity(int type);
	//IUnknown* GetDefaultEntity(int type);
	
public:
	Part() : part(nullptr) {}
	Part(std::unique_ptr<PartImpl> p) : part(std::move(p)) {}
	//IUnknown* pPart;
	//Part(IUnknown* pDoc, IUnknown* pPart);
	//Part() : Part(nullptr, nullptr) {}
	//Part(const Part& part);                // Конструктор копирования
	//Part& operator=(const Part& part);     // Оператор копирующего присваивания
	//Part(Part&& part) noexcept;            // Конструктор перемещения
	//Part& operator=(Part&& part) noexcept; // Оператор перемещающего присваивания
	//~Part();
	std::vector<Node> GetNodes() { return part->GetNodes(); }
	template <typename T, typename... Args>
	T Create(Args&&... args) {
		static_assert(std::is_base_of<Node, T>::value, "T must be derived from Node");
		std::unique_ptr<Node::NodeImpl> node = std::move(part->CreateImpl(T::TYPE));
		if (!node) throw Kompas3DException(std::string("Не могу создать объект ") + typeid(T).name());
		return T(std::move(node), std::forward<Args>(args)...);
	}
	std::string Name() { return part->Name(); }
	Part& Remove(const Node& node) { part->Remove(node); return *this; }
	Plane GetPlaneXOY() { return part->GetPlane(1); }
	Plane GetPlaneXOZ() { return part->GetPlane(2); }
	Plane GetPlaneYOZ() { return part->GetPlane(3); }
	Axis GetAxisOX() { return part->GetAxis(71); }
	Axis GetAxisOY() { return part->GetAxis(72); }
	Axis GetAxisOZ() { return part->GetAxis(73); }
	std::vector<Variable> GetVariables(bool isExternal = false) { return part->GetVariables(isExternal); }
	// Габарит тел детали; nullopt, если тел нет
	std::optional<BoundingBox> GetBoundingBox() { return part->GetBoundingBox(); }
	// Пересечение тел двух компонентов сборки (IBody7::CheckIntersectionWithBody, КОМПАС v23):
	// 0 — нет; 1 — касание в точке, 2 — по линии, 3 — по поверхности (при tangent); 4 — общий объём;
	// -1 — проверить нельзя
	int CheckIntersection(Part& other, bool tangent = false) { return part->CheckIntersection(other, tangent); }
	// МЦХ: масса в кг, длины в мм
	std::optional<MassProperties> GetMassProperties() { return part->GetMassProperties(); }
	int GetBodiesCount() { return part->GetBodiesCount(); }
	// Значение или выражение переменной; false — переменная не найдена
	bool SetVariable(const std::string& name, double value) { return part->SetVariable(name, value); }
	bool SetVariableExpression(const std::string& name, const std::string& expression) { return part->SetVariableExpression(name, expression); }
	bool Rebuild() { return part->Rebuild(); }
	// Все грани и рёбра тел детали (без повторов). Порядок стабилен, пока модель не меняется
	std::vector<Face> GetFaces() { return part->GetFaces(); }
	std::vector<Edge> GetEdges() { return part->GetEdges(); }
	// Расстояние и угол между гранями, рёбрами или вершинами этой детали/сборки
	std::optional<Measurement> Measure(const Node& a, const Node& b) { return part->Measure(a, b); }
	// Компонент сборки: файл, фиксация, положение (x, y — оси компонента в сборке)
	std::string GetFileName() { return part->GetFileName(); }
	// Обозначение детали (свойство «Обозначение»); попадает в основную надпись чертежа
	std::string GetDesignation() { return part->GetDesignation(); }
	// Наименование и обозначение детали; пустая строка — не менять. Проверяется чтением
	bool SetTitle(const std::string& name, const std::string& designation) { return part->SetTitle(name, designation); }
	bool IsFixed() { return part->IsFixed(); }
	// Системные свойства компонента (SystemPropertyType): 4 — обозначение, 5 — наименование,
	// 15 — позиция, 50 — «Рассекать на разрезах»… Значение — строкой; задать — с проверкой чтением
	std::optional<std::string> GetSystemProperty(int id) { return part->GetSystemProperty(id); }
	bool SetSystemProperty(int id, const std::variant<bool, double, std::string>& value) { return part->SetSystemProperty(id, value); }
	// Стандартное изделие (из библиотеки)
	bool IsStandard() { return part->IsStandard(); }
	// Штриховка компонента в разрезах чертежа: угол наклона и шаг, мм
	std::optional<std::pair<double, double>> GetHatch() { return part->GetHatch(); }
	bool SetHatch(double angle, double step) { return part->SetHatch(angle, step); }
	Part& SetFixed(bool fixed) { part->SetFixed(fixed); return *this; }
	std::optional<Placement> GetPlacement() { return part->GetPlacement(); }
	bool SetPlacement(const Placement& placement) { return part->SetPlacement(placement); }
	operator bool() const { return (bool)part; }
};

#endif
