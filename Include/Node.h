#ifndef _ComTest_Node_h_
#define _ComTest_Node_h_

#include <string>
#include <exception>
#include <optional>

class Kompas3DException : public std::exception {
protected:
	std::string message;

public:
	Kompas3DException(std::string msg) : message(std::move(msg)) {}
	const char* what() const noexcept override { return message.c_str(); }
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
		virtual ~NodeImpl() = default;
	};

public:
	std::unique_ptr<NodeImpl> node;
	static int TYPE;
	Node(std::unique_ptr<NodeImpl> p) : node(std::move(p)) {}
	//Node(const Node& node) : Node(node.p) {}
	virtual ~Node() {}
	int GetType() const { return node->GetType(); }
	bool IsType(int type) const { return node->GetType() == type; }
	std::string GetName() const { return node->GetName(); }
	Node& SetName(const std::string& name) { node->SetName(name); return *this; }
	Node& Update() { node->Update(); return *this; }
	operator bool() const { return node != nullptr; }
	Node& operator=(const Node& other);
};

#endif
