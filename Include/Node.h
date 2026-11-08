#ifndef _ComTest_Node_h_
#define _ComTest_Node_h_

#include <string>
#include <vector>
#include <exception>
#include <optional>
#include <map>
#include <memory>
#include <vector>

class Kompas3DException : public std::exception {
protected:
	std::string message;

public:
	Kompas3DException(std::string msg) : message(std::move(msg)) {}
	const char* what() const noexcept override { return message.c_str(); }
};

// Числовые параметры операции по именам: depth1, angle1, radius, count…
using NodeParams = std::map<std::string, double>;

// Переменная модели или параметр операции (у операции и эскиза свои переменные: v20 —
// «Расстояние 1» выдавливания, v26 — линейный размер эскиза)
struct Variable {
	enum class Type { Unknown = 0, Bool = 1, Int = 2, Double = 3, Angle = 4, Interval = 5, Function = 6 };
	bool isExternal = false;
	double value = 0;
	std::string name;
	std::string comment;
	std::string expression;   // «L*2»; у переменной без связи — значение текстом («25»)
	std::string parameter;    // что задаёт переменная операции: «Расстояние 1», «Линейный размер»
	Type type = Type::Unknown;
	bool information = false; // информационная (справочный размер эскиза): только чтение
};

// Цвет и оптические свойства (как в КОМПАСе, все свойства 0…1)
struct Appearance {
	unsigned long color = 0x909090;   // 0xRRGGBB
	double ambient = 0.5;              // общий цвет
	double diffuse = 0.6;              // диффузия
	double specularity = 0.8;          // зеркальность
	double shininess = 0.8;            // блеск
	double opacity = 1.0;              // непрозрачность: 1 — непрозрачный (в API КОМПАСа — Transparency)
	double emission = 0.5;             // излучение
	bool own = true;                   // свой цвет объекта; false — наследуется от владельца
};

class Node {
public:
	class NodeImpl {
	public:
		virtual int GetType() const = 0;
		virtual std::string GetName() const = 0;
		virtual void SetName(const std::string& name) = 0;
		virtual void Create() = 0;
		virtual void Update() = 0;
		// Код ошибки построения операции (0 — без ошибок)
		virtual int GetError() const { return 0; }
		// Опорные объекты операции: эскиз выдавливания, плоскость эскиза — имена
		virtual std::vector<std::string> GetSubFeatureNames() const { return {}; }
		// Параметры, которые можно прочитать и изменить у готовой операции
		virtual NodeParams GetParamMap() { return {}; }
		// Идентичность объекта КОМПАСа: одинакова у двух обёрток одного объекта
		virtual const void* GetIdentity() const { return nullptr; }
		virtual void SetParamMap(const NodeParams& params) {}
		// Переменные операции (параметры, размеры эскиза)
		virtual std::vector<Variable> GetVariables() { return {}; }
		virtual bool SetVariableExpression(const std::string& name, const std::string& expression) { return false; }
		// Цвет операции или грани; nullopt — объект не поддерживает цвет
		virtual std::optional<Appearance> GetAppearance() { return std::nullopt; }
		// nullopt — снять свой цвет (наследовать от владельца)
		virtual bool SetAppearance(const std::optional<Appearance>& appearance) { return false; }
		// Скрыть объект в окне модели (поверхность после сшивки или отсечения)
		virtual bool SetHidden(bool hidden) { return false; }
		virtual ~NodeImpl() = default;
	};

public:
	std::unique_ptr<NodeImpl> node;
	static int TYPE;
	Node(std::unique_ptr<NodeImpl> p) : node(std::move(p)) {}
	// Node владеет реализацией, поэтому копирование запрещено, а перемещение разрешено
	Node(const Node& other) = delete;
	Node& operator=(const Node& other) = delete;
	Node(Node&& other) noexcept = default;
	Node& operator=(Node&& other) noexcept = default;
	virtual ~Node() {}
	int GetType() const { return node->GetType(); }
	bool IsType(int type) const { return node->GetType() == type; }
	std::string GetName() const { return node->GetName(); }
	Node& SetName(const std::string& name) { node->SetName(name); return *this; }
	Node& Update() { node->Update(); return *this; }
	int GetError() const { return node->GetError(); }
	// Опорные объекты: эскиз операции (выдавливание, вырез, вращение, ребро…), плоскость эскиза
	std::vector<std::string> GetSubFeatureNames() const { return node->GetSubFeatureNames(); }
	// Параметры операции; после SetParamMap нужен Update() и перестроение модели
	NodeParams GetParamMap() { return node->GetParamMap(); }
	// Указатель для сравнения: тот же объект КОМПАСа — тот же указатель (nullptr — неизвестно)
	const void* GetIdentity() const { return node ? node->GetIdentity() : nullptr; }
	Node& SetParamMap(const NodeParams& params) { node->SetParamMap(params); return *this; }
	// Переменные операции: параметры (глубина, угол…) и управляющие размеры эскиза.
	// Выражение связывает параметр с переменными модели («L*2»); после — перестроение
	std::vector<Variable> GetVariables() { return node->GetVariables(); }
	bool SetVariableExpression(const std::string& name, const std::string& expression) {
		return node->SetVariableExpression(name, expression);
	}
	std::optional<Appearance> GetAppearance() { return node->GetAppearance(); }
	bool SetAppearance(const std::optional<Appearance>& appearance) { return node->SetAppearance(appearance); }
	bool SetHidden(bool hidden) { return node->SetHidden(hidden); }
	operator bool() const { return node != nullptr; }
};

#endif
