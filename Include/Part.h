#ifndef _ComTest_Part_h_
#define _ComTest_Part_h_

#include "Node.h"
#include "Node/Plane.h"
#include "Node/Axis.h"
#include <vector>
#include <utility>

class Part {
public:
	struct Variable {
		bool isExternal;
		double value;
		std::string name;
		std::string comment;
	};
	class PartImpl {
	public:
		virtual std::string Name() { return std::string(); }
		virtual std::unique_ptr<Node::NodeImpl> CreateImpl(int type) { return nullptr; }
		virtual Plane GetPlane(int type) = 0;
		virtual Axis GetAxis(int type) = 0;
		virtual std::vector<Node> GetNodes() = 0;
		virtual void Remove(const Node& node) = 0;
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
	//std::vector<Variable> GetVariables(bool isExternal = false);
	operator bool() const { return (bool)part; }
};

#endif
