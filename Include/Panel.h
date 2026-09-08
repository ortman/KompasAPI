#ifndef _KompasAPI_Panel_h_
#define _KompasAPI_Panel_h_

#include <vector>
#include <string>
#include <memory>
#include <variant>
#include <optional>
#include <cstdlib>
#include "Node.h"
#include "KompasEvent.h"

using PropertyVariant = std::variant<int, double, std::string>;

class Panel {
public:
	class Tab;
	class Property;

	// Реализация одного элемента управления
	class PropertyImpl {
	public:
		// val == std::nullopt — элемент без значения (например, кнопка)
		virtual void Create(const std::string& name, const std::optional<PropertyVariant>& val, int id) = 0;
		virtual void SetName(const std::string& name) = 0;
		virtual PropertyVariant GetValue() = 0;
		virtual void SetValue(const PropertyVariant& val) = 0;
		virtual void Add(const PropertyVariant& val) {}
		virtual void ClearList() {}
		virtual int Find(const PropertyVariant& val) { return -1; }
		virtual ~PropertyImpl() = default;
	};

	// Реализация вкладки
	class TabImpl {
	public:
		virtual std::unique_ptr<PropertyImpl> AddProperty(int type) = 0;
		virtual void Clear(int count) = 0;
		virtual ~TabImpl() = default;
	};

	// Реализация панели свойств
	class PanelImpl {
	public:
		// owner нужен реализации, чтобы вернуть события панели; Panel не копируется и не перемещается
		virtual bool Create(Panel* owner, const std::string& caption) = 0;
		virtual std::unique_ptr<TabImpl> AddTab(const std::string& name) = 0;
		virtual void Finish() {} // Вызывается после создания всех вкладок
		virtual void Update() = 0;
		virtual void Show(bool isShow) = 0;
		virtual ~PanelImpl() = default;
	};

protected:
	std::unique_ptr<PanelImpl> panel;
	std::string name;
	std::vector<Panel::Tab*> tabs;

	static inline Panel* currentPanel = nullptr;

	bool Build(std::unique_ptr<PanelImpl> impl);

public:
	KompasEvent<bool(int buttonId)> WhenButtonClick;

	Panel(const char* name) : panel(nullptr), name(name) {
		currentPanel = this;
	}
	Panel(const Panel& panel) = delete; // Конструктор копирования
  Panel& operator=(const Panel& panel) = delete; // Оператор копирующего присваивания
	Panel(Panel&& panel) noexcept = delete; // Конструктор перемещения
	Panel& operator=(Panel&& panel) noexcept = delete; // Оператор перемещающего присваивания
	~Panel() { currentPanel = nullptr; }
	bool Create(); // Определён в Kompas3D.h — нужен доступ к фабрике реализации
	void Update();
	void Show(bool isShow = true);
	void Hide() { Show(false); }
	operator bool() const { return (bool)panel; }

	// Вызывается реализацией при приходе событий от КОМПАС
	bool OnButtonClick(int buttonId) { return WhenButtonClick(buttonId); }
	void OnControlCommand(int id);
	void OnChangeControlValue(int id);
};

class Panel::Tab {
private:
	std::unique_ptr<TabImpl> tab;
	std::string name;
	std::vector<Panel::Property*> createdProps;
	void CreateProperty(Property* property);

public:
	static inline Tab* currentTab = nullptr;
	std::vector<Panel::Property*> props;

	Tab(const char* name) : tab(nullptr), name(name) {
		if (Panel::currentPanel) {
			Panel::currentPanel->tabs.push_back(this);
		}
		currentTab = this;
	}
	~Tab();
	Tab& Clear();
	template <typename T>
	T& Create(const char* name) {
		currentTab = this;
		T* p = new T(name);
		createdProps.push_back(p);
		CreateProperty(p);
		return *p;
	}
	//void Remove(Panel::Property& prop);

	friend class Panel;
	friend class KProcess3D;
};

class Panel::Property {
protected:
	std::unique_ptr<PropertyImpl> prop;
	std::string name;
	PropertyVariant defaultVal;
	int type;
	int id = 0;
	static inline int nextId = 1;
	Panel::Tab* tab = NULL;

	Property(const char* name, int type, PropertyVariant val) : prop(nullptr), name(name), type(type), defaultVal(val) {
		if (Panel::Tab::currentTab) {
			Panel::Tab::currentTab->props.push_back(this);
			tab = Panel::Tab::currentTab;
		}
	}

	virtual void Create(std::unique_ptr<PropertyImpl> p) {
		prop = std::move(p);
		id = ++nextId;
		prop->Create(name, defaultVal, id);
	}

public:
	KompasEvent<void(void)> WhenChange;

	virtual ~Property() = default;

	int GetId() const { return id; }
	void SetName(const std::string& name) {
		this->name = name;
		if (prop) prop->SetName(name);
	}
	operator bool() const { return (bool)prop; }

	friend class Panel;
	friend class Panel::Tab;
};

class PropertyI : public Panel::Property {
public:
	PropertyI(const char* name, int val) : Property(name, 2, val) {}
	operator int() const;
	int operator=(int);
};

class PropertyD : public Panel::Property {
public:
	PropertyD(const char* name, double val) : Property(name, 3, val) {}
	operator double() const;
	double operator=(double);
};

class PropertyList : public Panel::Property {
public:
	PropertyList(const char* name) : Property(name, 7, "") {}
	PropertyList& Add(PropertyVariant val);
	PropertyList& Clear();
	int Find(PropertyVariant val);
	operator PropertyVariant() const;
	PropertyVariant operator=(PropertyVariant);
};

class PropertyButton : public Panel::Property {
public:
	KompasEvent<void(void)> WhenClick;
	PropertyButton(const char* name) : Property(name, 13, "") {}

protected:
	void Create(std::unique_ptr<Panel::PropertyImpl> p) override {
		prop = std::move(p);
		id = ++nextId;
		prop->Create(name, std::nullopt, id); // у кнопки нет значения
	}
};

// ---- Panel ----

inline bool Panel::Build(std::unique_ptr<PanelImpl> impl) {
	currentPanel = nullptr;
	if (!impl) return false;
	try {
		if (!impl->Create(this, name)) return false;
		panel = std::move(impl);
		for (Tab* t : tabs) {
			std::unique_ptr<TabImpl> tabImpl = panel->AddTab(t->name);
			if (!tabImpl) continue;
			t->tab = std::move(tabImpl);
			for (Property* p : t->props) t->CreateProperty(p);
		}
		panel->Finish();
	} catch (const Kompas3DException&) {
		panel = nullptr;
		return false;
	}
	return true;
}

inline void Panel::Update() {
	if (!panel) throw Kompas3DException("Панель свойств не создана");
	panel->Update();
}

inline void Panel::Show(bool isShow) {
	if (!panel) throw Kompas3DException("Панель свойств не создана");
	panel->Show(isShow);
}

inline void Panel::OnControlCommand(int id) {
	for (Tab* t : tabs) {
		for (Property* p : t->props) {
			if (p->GetId() == id) {
				if (PropertyButton* b = dynamic_cast<PropertyButton*>(p)) b->WhenClick();
				return;
			}
		}
	}
}

inline void Panel::OnChangeControlValue(int id) {
	for (Tab* t : tabs) {
		for (Property* p : t->props) {
			if (p->GetId() == id) {
				p->WhenChange();
				return;
			}
		}
	}
}

// ---- Panel::Tab ----

inline Panel::Tab::~Tab() {
	for (Property* p : createdProps) delete p;
	currentTab = nullptr;
}

inline Panel::Tab& Panel::Tab::Clear() {
	if (tab) tab->Clear((int)props.size());
	props.clear();
	for (Property* p : createdProps) delete p;
	createdProps.clear();
	return *this;
}

inline void Panel::Tab::CreateProperty(Panel::Property* property) {
	if (!tab || !property) return;
	std::unique_ptr<PropertyImpl> impl = tab->AddProperty(property->type);
	if (impl) property->Create(std::move(impl));
}

// ---- Свойства ----

inline PropertyI::operator int() const {
	if (!prop) return std::get<int>(defaultVal);
	return std::visit([](const auto& v) -> int {
		using T = std::decay_t<decltype(v)>;
		if constexpr (std::is_same_v<T, std::string>) return std::atoi(v.c_str());
		else return (int)v;
	}, prop->GetValue());
}

inline int PropertyI::operator=(int val) {
	if (prop) prop->SetValue(val);
	defaultVal = val;
	return val;
}

inline PropertyD::operator double() const {
	if (!prop) return std::get<double>(defaultVal);
	return std::visit([](const auto& v) -> double {
		using T = std::decay_t<decltype(v)>;
		if constexpr (std::is_same_v<T, std::string>) return std::atof(v.c_str());
		else return (double)v;
	}, prop->GetValue());
}

inline double PropertyD::operator=(double val) {
	if (prop) prop->SetValue(val);
	defaultVal = val;
	return val;
}

inline PropertyList& PropertyList::Add(PropertyVariant val) {
	if (prop) prop->Add(val);
	return *this;
}

inline PropertyList& PropertyList::Clear() {
	if (prop) prop->ClearList();
	return *this;
}

inline int PropertyList::Find(PropertyVariant val) {
	return prop ? prop->Find(val) : -1;
}

inline PropertyList::operator PropertyVariant() const {
	return prop ? prop->GetValue() : defaultVal;
}

inline PropertyVariant PropertyList::operator=(PropertyVariant val) {
	if (prop) prop->SetValue(val);
	defaultVal = val;
	return val;
}

#endif
