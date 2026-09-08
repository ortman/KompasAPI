# KompasAPI

Обёртка над API КОМПАС-3D для C++. Прикладной код работает с обычными C++ классами
(`Doc3D`, `Part`, `Sketch`, `Panel`…) и не видит ни COM, ни `IUnknown`, ни `_variant_t`,
ни кодировки cp1251.

Пакет U++ (`KompasAPI.upp`), подключается как зависимость: `uses Kompas3DPrint/KompasAPI;`

## Зачем

Прямая работа с API КОМПАС из C++ — это ручной подсчёт ссылок, два несовместимых
поколения интерфейсов (API5 и API7), перевод строк в cp1251 и `VARIANT` на каждый чих.
Библиотека прячет это за фасадом:

```cpp
Doc3D doc = Kompas3D::GetActiveDocument3D();
if (!doc) return;
Part part = doc.GetTopPart();

Sketch sketch = part.Create<Sketch>(part.GetPlaneXOY(), "Эскиз шестерни");
sketch.Circle(0, 0, 25).Rect(-5, -5, 10, 10);

BaseExtrusion body = part.Create<BaseExtrusion>(sketch, 10.0);
```

## Структура

```
Include/     публичный API — только стандартный C++, без COM
Api7/        реализация поверх API5/API7 КОМПАСа (Windows, COM)
Ksapi/       место под реализацию для Linux (KsAPI); пока не реализовано
```

Прикладной код включает **только** `Include/Kompas3D.h` — он подтягивает всё остальное.
Заголовки из `Api7/` подключает единственная единица трансляции `Api7/ComKompas.cpp`;
напрямую их включать не нужно.

## Устройство: pimpl

Каждый публичный класс — тонкая обёртка над указателем на интерфейс реализации:

```cpp
class Face : public Node {
public:
    class FaceImpl : virtual public Node::NodeImpl {   // интерфейс, объявлен в Include/
    public:
        virtual bool IsPlanar() = 0;
        virtual bool IsCylinder() = 0;
    };

    bool IsPlanar() {                                   // обёртка
        FaceImpl* face = dynamic_cast<FaceImpl*>(node.get());
        return face ? face->IsPlanar() : false;
    }
};
```

а в `Api7/` живёт реализация:

```cpp
class FaceApi7 : public NodeApi7, public Face::FaceImpl {
    bool IsPlanar() override { K5::ksFaceDefinitionPtr face = def; return face && face->IsPlanar(); }
};
```

Реализацию нигде не создают вручную — её выдаёт фабрика: `Part::CreateImpl(type)` для
узлов, `Doc3DImpl` для документа, `Kompas3DImpl::CreatePanel()` для панели свойств.
Благодаря этому весь `Include/` компилируется без заголовков КОМПАСа, а прикладной код
можно проверять на подставных реализациях.

### Владение и перемещение

`Node`, `Part`, `Doc3D`, `MateConstraint` владеют своей реализацией через `unique_ptr`,
поэтому **копирование запрещено, перемещение разрешено**:

```cpp
NodeMacro edit;                        // пустой объект
edit = doc.GetEditMacroObject();       // перемещающее присваивание
edit = NodeMacro();                    // сброс, прежняя реализация освобождается
```

Отсюда же — способ типизировать узел, полученный из модели. Конструктор `Xxx(Node&&)`
перехватывает узел, забирая владение:

```cpp
for (Node& node : part.GetNodes()) {
    if (node.IsType(ThreadDesignation::TYPE)) {
        ThreadDesignation thread(std::move(node));   // node становится пустым
        double d = thread.GetDiameter();
    }
}
```

## Основные классы

### `Kompas3D` — точка входа

Статический фасад приложения.

| | |
|---|---|
| `GetActiveDocument3D()` | активный документ-модель |
| `Open3D(path, visible)` | открыть файл модели |
| `Message(txt)` / `Error(txt)` | сообщение пользователю |
| `SystemPath(type)`, `ConfigPath()` | системные пути КОМПАСа |
| `ComConnect(open, visible)` / `ComDisconnect()` | подключение к запущенному КОМПАСу через COM |
| `Cp1251ToUtf8` / `Utf8ToCp1251` | перекодировка (внутри API всё в UTF-8) |
| `WhenCreateDocument`, `WhenOpenDocument`, `WhenConnect` | события |

`RunCommand(id)` реализует **приложение**, а не библиотека — это диспетчер команд меню,
который вызывает КОМПАС.

### `Doc3D` — документ-модель

`GetPath`, `GetTopPart`, `GetEditMacroObject`, `SaveAs`, `Reopen`, `Close`,
исполнения (`GetEmbodimentsCount`, `GetEmbodimentName`, `GetEmbodiment`,
`SetCurrentEmbodiment`), сборка (`AddPart`, `AddMateConstraint`) и запуск процесса
(`CreatePorcess<T>()`).

`SaveAs` принимает `Doc3D::ExportParams` — формат (`Doc3D::Format`: STL, STEP, IGES,
ACIS, PARASOLID, VRLM, JT), единицы, точность триангуляции.

### `Part` — деталь или компонент сборки

```cpp
Sketch sketch = part.Create<Sketch>(part.GetPlaneXOY(), "Эскиз");
```

`Create<T>(args...)` создаёт узел типа `T::TYPE` и передаёт `args` в конструктор `T` —
там задаются параметры и вызывается `Create()`. Также: `GetNodes`, `Remove`,
`GetPlaneXOY/XOZ/YOZ`, `GetAxisOX/OY/OZ`, `GetVariables`.

### Узлы модели

Все наследуют `Node` (`GetType`, `IsType`, `GetName`, `SetName`, `Update`,
`operator bool`). У каждого есть `TYPE` — константа `o3d_*` из `ksConstants3D.h`.

| Класс | TYPE | Назначение |
|---|---|---|
| `Plane`, `ParallelPlane`, `EdgePointPlane` | 1, 20, 19 | плоскости; проекции точек, вектор осей |
| `Axis`, `ConeAxis` | 71, 11 | оси |
| `Face` | 6 | грань (`IsPlanar`, `IsCylinder`) |
| `Edge` | 7 | ребро (смежные грани, вершины, центр) |
| `Vertex` | 8 | вершина (→ `Point3D`) |
| `Sketch` | 5 | эскиз: примитивы 2D, проекция 3D-точки |
| `BaseExtrusion`, `CutExtrusion` | 24, 26 | выдавливание |
| `CutRotated`, `CutEvolution` | 29, 47 | вырезать вращением / кинематически |
| `MeshCopy`, `CircularCopy` | 35, 36 | массивы по сетке и концентрический |
| `CylindricSpiral` | 56 | цилиндрическая спираль |
| `ThreadDesignation` | 58 | условное изображение резьбы |
| `NodeMacro` | 63 | макрообъект (группировка операций, свои параметры) |

`Sketch` — цепочечный: `sketch.Line(...).Circle(...).Rect(...)`; редактирование
открывается лениво и закрывается `EndEdit()` (операции вроде `BaseExtrusion` делают это
сами).

### `Panel` — панель свойств

Панель описывается **декларативно**, вложенными структурами. Вкладки и элементы
регистрируются в конструкторах, поэтому порядок объявления = порядок на панели:

```cpp
struct : Panel {
    struct : Panel::Tab {
        PropertyD m     {"Модуль",     0.8};
        PropertyI count {"Количество", 20};
    } main{"Параметры"};
} panel{"Параметры шестерни"};

panel.WhenButtonClick = [=](int buttonId) { /* 1 = ОК */ return false; };
if (!panel.Create()) return;     // здесь панель реально создаётся в КОМПАСе
panel.Show();

double m = panel.main.m;         // чтение через operator double()
panel.main.count = 24;           // запись
```

Элементы: `PropertyI`, `PropertyD`, `PropertyList`, `PropertyButton` (у каждого
`WhenChange`, у кнопки `WhenClick`). Динамические элементы — `tab.Create<PropertyList>(name)`,
удаление всех — `tab.Clear()`.

`PropertyList` хранит добавленные значения у себя: `GetIndex()` возвращает номер
текущего значения **в порядке добавления**, независимо от того, как КОМПАС показывает
список.

### `Process3D` — процесс с фантомом

Процесс — это панель свойств плюс интерактивное размещение объекта в модели.
`KProcess3D` наследует `Panel`, поэтому вкладки описываются так же.

```cpp
class InsertProc : public Process3D<InsertProc> {
public:
    struct : Panel::Tab { PropertyButton model{"Модель"}; } mainTab{"Основные"};

    bool OnFilterObject(Node&& node) override {           // какие объекты подсвечивать
        if (node.GetType() != Face::TYPE) return false;
        return Face(std::move(node)).IsPlanar();
    }
    bool OnPlacementChange(Node&& node) override { ... }   // объект под курсором сменился
};

InsertProc& proc = doc.CreatePorcess<InsertProc>();
proc.SetCaption("Вставка");
proc.SetPhantom(part);
proc.Run(false, false);
```

Обработчики получают `Node&&` — узел создаётся на каждый вызов и передаётся во владение,
поэтому его сразу типизируют: `Face face(std::move(node))`.

Сопряжения: `AddMateConstraint(тип, объект1, объект2, направление)` возвращает
`MateConstraint`, который держит оба объекта живыми (`GetFirst`, `GetSecond`) — их потом
можно передать в `Doc3D::AddMateConstraint` для настоящей сборки.

> **КОМПАС-3D Viewer запускает процесс, но панель свойств не показывает.**
> Проверять процессы только в полноценном КОМПАС-3D.

## Ошибки

Всё, что пошло не так, — `Kompas3DException` (наследник `std::exception`, сообщение
на русском в UTF-8). Библиотека исключения не глушит: решает вызывающий код.

```cpp
try {
    ...
} catch (const Kompas3DException& e) {
    Kompas3D::Error(e.what());
}
```

Обработчики событий (`WhenButtonClick` и прочие) вызываются из КОМПАСа, поэтому
исключение из них уйдёт в чужой стек — **оборачивайте тело обработчика в `try/catch`**.

## Сборка

Реализация выбирается макросами (см. `KompasAPI.upp`):

| Макрос | Что включает |
|---|---|
| `KAPI7` | реализация `Api7/` (Windows, COM) |
| `KAPI5` | доступ к интерфейсам API5 внутри `Api7/` |
| `KSAPI` | реализация `Ksapi/` (Linux) — заготовка |

Нужен установленный SDK КОМПАС-3D; пути прописаны в `KompasAPI.upp` (`C:/KSDK24`)
и подключаются библиотеки `kAPI7`, `kAPI5`, `kAPI3D5`, `kAPI2D5`.

Интерфейсы КОМПАСа импортируются в `Api7/ComKompas.h`: под MSVC — `#import` из `.tlb`
(пространства имён `K5`, `K7`, `KConst`, `KConst3D`), иначе — заранее сгенерированные
`.tlh`.

Библиотека собирается как DLL и подключается к КОМПАСу через `LibInterfaceNotifyEntry`
(КОМПАС передаёт указатель на приложение) и `LIBRARYENTRY` (вызов команды меню).
Альтернативно — внешнее подключение к запущенному КОМПАСу через `Kompas3D::ComConnect()`.

## Как добавить новый тип узла

1. `Include/Node/Xxx.h` — класс `Xxx : public Node`, внутри `class XxxImpl : virtual public Node::NodeImpl`
   с чисто виртуальными методами; публичные обёртки через `dynamic_cast`; `static inline int TYPE = <o3d_*>;`
   и конструктор `Xxx(Node&& node)`.
2. `Api7/Node/Xxx.hpp` — `class XxxApi7 : public NodeApi7, public Xxx::XxxImpl`.
   Доступны `entity` (`ksEntity`) и `def` (definition) из `NodeApi7`, а также
   `ToApi7<T>()` / `ToApi5<T>()` для перехода между поколениями интерфейсов.
3. Зарегистрировать тип в `PartApi7::EntityToNode` — иначе `Part::Create<Xxx>()` и
   `GetNodes()` не отдадут нужный класс.
4. Добавить оба файла в `KompasAPI.upp` и включить заголовок в `Include/Kompas3D.h`.

Если узел создаётся не через `Part::NewEntity`, а извлекается из другого объекта
(как грань из ребра), метод реализации возвращает `std::unique_ptr<Node::NodeImpl>`,
а обёртка заворачивает его в нужный класс.

## Известные ограничения

- `Ksapi/` (Linux) не реализован — файлы содержат старый код до перехода на pimpl.
- События документа (`Doc3D::WhenCloseDocument` и прочие) объявлены, но подписка на них
  не подключена: `Doc3D` перемещается, а COM-нотификатору нужен стабильный адрес.
  Чтобы заработало, события надо перенести в `Doc3DImpl`.
- `MateConstraint::SetType` отсутствует: `IMateConstraint3D::ConstraintType` в КОМПАСе
  доступен только для чтения, тип задаётся при создании сопряжения.
- `Doc3D::CreatePorcess` — опечатка в имени намеренная: `CreateProcess` занято макросом
  Win32. По той же причине фабрика в `Doc3DImpl` называется `CreateProcessImpl`.
