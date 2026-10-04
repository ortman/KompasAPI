#ifndef _ComTest_Node_h_
#define _ComTest_Node_h_

#include <string>
#include <vector>
#include <exception>
#include <optional>
#include <map>
#include <memory>

class Kompas3DException : public std::exception {
protected:
	std::string message;

public:
	Kompas3DException(std::string msg) : message(std::move(msg)) {}
	const char* what() const noexcept override { return message.c_str(); }
};

// Числовые параметры операции по именам: depth1, angle1, radius, count…
using NodeParams = std::map<std::string, double>;

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
	operator bool() const { return node != nullptr; }
};

#endif
