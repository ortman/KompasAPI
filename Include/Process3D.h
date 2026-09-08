#ifndef _Kompas3DPrint_Process3D_h_
#define _Kompas3DPrint_Process3D_h_

#include <concepts>
#include <memory>
#include <utility>

#include "Node.h"
#include "Part.h"
#include "Panel.h"

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

class MateConstraint {
public:
	class MateConstraintImpl {
	public:
		// Тип сопряжения задаётся при создании и в КОМПАС только для чтения
		virtual void SetDir(MateDir dir) = 0;
		virtual void SetFixed(MateFixed fixed) = 0;
		virtual void SetValue(double value) = 0;
		virtual void SetFirst(const Node& node) = 0;
		virtual void SetSecond(const Node& node) = 0;
		virtual ~MateConstraintImpl() = default;
	};

private:
	std::unique_ptr<MateConstraintImpl> mate;
	MateType type;
	MateDir dir;
	MateFixed fixed;
	Node first;
	Node second;
	double value;

public:
	MateConstraint() : mate(nullptr), type(MateCoincidence), dir(MateDirUndefined),
	                   fixed(MateFixedNone), first(nullptr), second(nullptr), value(0.0) {}
	MateConstraint(std::unique_ptr<MateConstraintImpl> m, MateType type, MateDir dir, MateFixed fixed, double value)
		: mate(std::move(m)), type(type), dir(dir), fixed(fixed), first(nullptr), second(nullptr), value(value) {}
	// MateConstraint владеет реализацией и объектами сопряжения — только перемещение
	MateConstraint(const MateConstraint& m) = delete;
	MateConstraint& operator=(const MateConstraint& m) = delete;
	MateConstraint(MateConstraint&& m) noexcept = default;
	MateConstraint& operator=(MateConstraint&& m) noexcept = default;
	~MateConstraint() = default;

	MateConstraint& SetDir(MateDir d) {
		dir = d;
		if (mate) mate->SetDir(d);
		return *this;
	}
	MateConstraint& SetFixed(MateFixed f) {
		fixed = f;
		if (mate) mate->SetFixed(f);
		return *this;
	}
	MateConstraint& SetValue(double v) {
		value = v;
		if (mate) mate->SetValue(v);
		return *this;
	}
	MateConstraint& SetFirst(Node&& node) {
		if (mate && node) {
			mate->SetFirst(node);
			first = std::move(node);
		}
		return *this;
	}
	MateConstraint& SetSecond(Node&& node) {
		if (mate && node) {
			mate->SetSecond(node);
			second = std::move(node);
		}
		return *this;
	}
	Node& GetFirst() { return first; }
	Node& GetSecond() { return second; }
	MateType GetType() const { return type; }
	MateDir GetDir() const { return dir; }
	MateFixed GetFixed() const { return fixed; }
	double GetValue() const { return value; }
	operator bool() const { return (bool)mate; }
};

class Doc3D;
class KProcess3D : public Panel {
public:
	// Реализация процесса. Она же — реализация панели свойств процесса:
	// у процесса своя панель (IProcessParam), а не отдельный IPropertyManager
	class Process3DImpl : public Panel::PanelImpl {
	public:
		virtual void SetOwner(KProcess3D* owner) = 0;
		virtual bool Run(bool modal, bool postMessage) = 0;
		virtual bool Stop() = 0;
		virtual void SetPhantom(const Part& part) = 0;
		virtual Part GetPhantom() = 0;
		virtual void SetCaption(const std::string& caption) = 0;
		virtual std::unique_ptr<MateConstraint::MateConstraintImpl>
			AddMateConstraint(MateType type, MateDir dir, MateFixed fixed, double value) = 0;
	};

protected:
	bool hasPlacementChangeMethod = false;
	bool hasFilterObjectMethod = false;

	virtual bool OnPlacementChange(Node&& node) { return false; }
	virtual bool OnFilterObject(Node&& node) { return false; }

	Process3DImpl* Proc() const { return dynamic_cast<Process3DImpl*>(panel.get()); }
	void Init(std::unique_ptr<Process3DImpl> impl);

public:
	KProcess3D() : Panel("") {}
	KProcess3D(const KProcess3D& proc) = delete; // Конструктор копирования
  KProcess3D& operator=(const KProcess3D& proc) = delete; // Оператор копирующего присваивания
	KProcess3D(KProcess3D&& proc) noexcept = delete; // Конструктор перемещения
	KProcess3D& operator=(KProcess3D&& proc) noexcept = delete; // Оператор перемещающего присваивания
	virtual ~KProcess3D() = default;

	bool Run(bool prop, bool cmd);
	bool Stop();
	void SetPhantom(const Part& part);
	Part GetPhantom();
	MateConstraint AddMateConstraint(MateType type, Node&& object1, Node&& object2,
	                                 MateDir direction, MateFixed fixed = MateFixedNone, double value = 0.0);
	void SetCaption(const std::string& caption);
	bool Create() { return false; } // Панель процесса создаётся вместе с процессом
	void Update();
	void Show(bool isShow = true) {}
	void Hide() { Show(false); }

	// Вызывается реализацией при приходе событий от КОМПАС
	bool OnFilter(Node&& node) { return hasFilterObjectMethod && OnFilterObject(std::move(node)); }
	bool OnPlacement(Node&& node) { return hasPlacementChangeMethod && OnPlacementChange(std::move(node)); }

	friend class Doc3D;
};

inline void KProcess3D::Init(std::unique_ptr<Process3DImpl> impl) {
	if (!impl) throw Kompas3DException("Не могу создать процесс");
	impl->SetOwner(this);
	if (!Build(std::move(impl))) throw Kompas3DException("Не могу создать панель свойств процесса");
}

inline bool KProcess3D::Run(bool prop, bool cmd) {
	Process3DImpl* proc = Proc();
	return proc ? proc->Run(prop, cmd) : false;
}

inline bool KProcess3D::Stop() {
	Process3DImpl* proc = Proc();
	return proc ? proc->Stop() : false;
}

inline void KProcess3D::SetPhantom(const Part& part) {
	if (Process3DImpl* proc = Proc()) proc->SetPhantom(part);
}

inline Part KProcess3D::GetPhantom() {
	Process3DImpl* proc = Proc();
	return proc ? proc->GetPhantom() : Part();
}

inline void KProcess3D::SetCaption(const std::string& caption) {
	if (Process3DImpl* proc = Proc()) proc->SetCaption(caption);
}

inline void KProcess3D::Update() {
	if (Process3DImpl* proc = Proc()) proc->Update();
}

inline MateConstraint KProcess3D::AddMateConstraint(MateType type, Node&& object1, Node&& object2,
                                                    MateDir direction, MateFixed fixed, double value) {
	Process3DImpl* proc = Proc();
	if (!proc) return MateConstraint();
	std::unique_ptr<MateConstraint::MateConstraintImpl> impl = proc->AddMateConstraint(type, direction, fixed, value);
	if (!impl) return MateConstraint();
	MateConstraint mate(std::move(impl), type, direction, fixed, value);
	mate.SetFirst(std::move(object1));
	mate.SetSecond(std::move(object2));
	return mate;
}

template <typename T>
concept CheckOnPlacementChange = requires(T a, Node&& node) { { a.OnPlacementChange(std::move(node)) } -> std::same_as<bool>; };
template <typename T>
concept CheckOnFilterObject    = requires(T a, Node&& node) { { a.OnFilterObject(std::move(node))    } -> std::same_as<bool>; };

template <typename ProcClass>
class Process3D : public KProcess3D {
public:
	Process3D() {
		if constexpr (CheckOnPlacementChange<ProcClass>) hasPlacementChangeMethod = true;
		if constexpr (CheckOnFilterObject<ProcClass>) hasFilterObjectMethod = true;
	}
};

#endif
