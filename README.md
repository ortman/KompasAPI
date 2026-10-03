# KompasAPI

Обёртка над API КОМПАС-3D для C++. Прикладной код работает с обычными C++ классами
(`Doc3D`, `Part`, `Sketch`, `Panel`…) и не видит ни COM, ни `IUnknown`, ни `_variant_t`,
ни кодировки cp1251, ни сторонних библиотек.

Проект оформлен как CMake или U++ пакет (`KompasAPI.upp`), подключается как зависимость: `uses Kompas3DPrint/KompasAPI;`

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
| `Open3D(path, visible)` | открыть файл модели; уже открытый файл просто активируется |
| `New3D(assembly, visible)` | создать новую деталь или сборку |
| `NewDrawing(visible)`, `OpenDrawing(path, visible)`, `GetActiveDrawing()` | чертежи |
| `ActiveDocumentType()` | тип активного документа (`DocumentTypeEnum`: 1 — чертёж, 4 — деталь, 5 — сборка) |
| `SetHideMessage(mode)` | автоответ на модальные вопросы КОМПАСа (0 — показывать, 1 — «Да», 2 — «Нет») |
| `Message(txt)` / `Error(txt)` | сообщение пользователю |
| `SystemPath(type)`, `ConfigPath()` | системные пути КОМПАСа |
| `ComConnect(open, visible)` / `ComDisconnect()` | подключение к запущенному КОМПАСу через COM |
| `Cp1251ToUtf8` / `Utf8ToCp1251` | перекодировка (внутри API всё в UTF-8) |
| `WhenCreateDocument`, `WhenOpenDocument`, `WhenConnect` | события |

`RunCommand(id)` реализует **приложение**, а не библиотека — это диспетчер команд меню,
который вызывает КОМПАС.

### `Doc3D` — документ-модель

`GetPath`, `IsPart`, `GetTopPart`, `GetEditMacroObject`, `Save`, `SaveAs`, `Reopen`, `Close`,
`Rebuild`, `SaveImage` (PNG-снимок окна модели: ориентация `Doc3D::View`, dpi,
скрытие вспомогательной геометрии),
исполнения (`GetEmbodimentsCount`, `GetEmbodimentName`, `GetEmbodiment`,
`SetCurrentEmbodiment`), сборка (`AddPart`, `AddMateConstraint`, `GetComponents`) и запуск
процесса (`CreatePorcess<T>()`).

`SaveAs(path)` сохраняет в родном формате, `SaveAs(params, path)` — экспорт:
принимает `Doc3D::ExportParams` — формат (`Doc3D::Format`: STL, STEP, IGES,
ACIS, PARASOLID, VRLM, JT), единицы, точность триангуляции.

`IsModified` — есть несохранённые изменения; `GetProjections` — проекции модели
(тип `Doc3D::View` и имя, например `#Спереди`) для видов чертежа.

### `Drawing` — чертёж

Лист и ассоциативные виды 3D-модели. Координаты на листе — мм от левого нижнего угла.

| | |
|---|---|
| `GetSheet` / `SetSheet(format, vertical)` | формат (`Drawing::Format`, A0…A5) и ориентация первого листа |
| `AddView(ViewParams)` | ассоциативный вид: файл модели, имя проекции, точка привязки, масштаб, подпись |
| `GetViews` | виды: номер, имя, точка привязки, масштаб, габарит на листе, оси листа в осях модели |
| `MoveView`, `DeleteView` | перемещение, масштаб, удаление вида |
| `Update` | перестроить ассоциативные виды по файлам моделей |
| `GetViewItems`, `AddDimension` | объекты вида и размеры в координатах вида, привязанные к линиям проекции |
| `Save`, `SaveAs`, `SaveImage(path, dpi)`, `ExportPdf(path)` | сохранение, PNG листа, PDF (конвертер `Bin/Pdf2d.dll`) |

Вид читает модель из файла: перед построением видов модель нужно сохранить.

### `ThreadSpec` — резьбы по стандартам

Заголовок без COM (`Include/ThreadSpec.h`): разбор обозначения, номинальный диаметр и
шаг в мм, диаметр сверла под резьбу, обозначение по стандарту.

```cpp
ThreadSpec m = *ThreadSpec::Parse("M12x1.25LH-6g");   // M12×1,25LH-6g
ThreadSpec g = *ThreadSpec::Parse("G1/2");            // Ø20,955, шаг 25,4/14
ThreadSpec u = *ThreadSpec::Parse("1/4-20 UNC-2B");   // Ø6,35, шаг 1,27
m.TapDrill();                                         // 10,75
ThreadSpec::ForTapDrill(6.8);                         // M8
ThreadSpec::FromMeasured(20.955, 25.4 / 14, false);   // G1/2
```

Метрическая (ГОСТ 24705, 8724, поле допуска ГОСТ 16093), трапецеидальная (ГОСТ 24738),
трубная цилиндрическая (ГОСТ 6357), дюймовая UNC/UNF/UNEF (ASME B1.1); нестандартные —
метрическая с любыми диаметром и шагом и дюймовая UNS (`standard == false`).

`part.Create<ThreadDesignation>(face, &startFace, spec, length)` строит условное
изображение резьбы и по умолчанию называет операцию обозначением (`M12×1,25LH-6g`);
`ThreadDesignation::GetSpec()` восстанавливает стандартную резьбу по диаметру и шагу.

### `Part` — деталь или компонент сборки

```cpp
Sketch sketch = part.Create<Sketch>(part.GetPlaneXOY(), "Эскиз");
```

`Create<T>(args...)` создаёт узел типа `T::TYPE` и передаёт `args` в конструктор `T` —
там задаются параметры и вызывается `Create()`.

| | |
|---|---|
| `GetNodes`, `Remove`, `Name` | дерево операций |
| `GetPlaneXOY/XOZ/YOZ`, `GetAxisOX/OY/OZ` | системные плоскости и оси |
| `GetVariables`, `SetVariable`, `SetVariableExpression`, `Rebuild` | переменные и перестроение |
| `GetBoundingBox`, `GetMassProperties`, `GetBodiesCount` | габарит, масса/объём/площадь/центр масс (мм, кг), число тел; у сборки — по компонентам |
| `GetFaces`, `GetEdges` | грани и рёбра тел |
| `Measure(a, b)` | расстояние и угол между гранями, рёбрами, плоскостями (`ksMeasurer`), ближайшие точки |
| `GetFileName`, `IsFixed`/`SetFixed`, `GetPlacement`/`SetPlacement` | компонент сборки: файл, фиксация, положение |

Оси стандартных плоскостей в эскизе (x, y → нормаль): XOY: +X, +Y → +Z;
XOZ: +X, **−Z** → +Y; YOZ: **+Z**, +Y → **−X**.

### Сборки

```cpp
Doc3D assembly = Kompas3D::New3D(true);
Part top = assembly.GetTopPart();
Part shaft = assembly.AddPart(top, "C:/models/shaft.m3d");
shaft.SetFixed(true);
Part gear = assembly.AddPart(top, "C:/models/gear.m3d");
gear.SetPlacement({{80, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}});   // начало, оси X, Y, Z

std::vector<Part> components = assembly.GetComponents();   // компоненты верхнего уровня
// грани компонента для сопряжений — components[1].GetFaces() / GetPlaneXOY()
assembly.AddMateConstraint(MateConcentric, gearBore, shaftCylinder, MateDirUndefined);
```

### Параметры готовых операций

`Node::GetParamMap()` / `SetParamMap(NodeParams)` — числовые параметры операции по
именам; после записи нужны `Update()` и перестроение модели:

| Операция | Параметры |
|---|---|
| выдавливание (24, 25, 26) | `direction` (`ExtrusionDirection`), `depth1`, `depth2`, `end1`, `end2` (`ExtrusionEnd`) |
| вращение (27, 28, 29) | `direction`, `angle1`, `angle2` |
| `Fillet` | `radius` |
| `Chamfer` | `distance1`, `distance2`, `transfer` |
| `CircularCopy` | `count`, `step` (угол между копиями) |
| `MeshCopy` | `count1`, `step1`, `count2`, `step2` |

`Node::GetIdentity()` — указатель `IUnknown` объекта КОМПАСа: две обёртки одного объекта
(например, общее ребро, полученное с двух граней) дают один указатель.

### Узлы модели

Все наследуют `Node` (`GetType`, `IsType`, `GetName`, `SetName`, `Update`, `GetError` (код ошибки построения),
`operator bool`). У каждого есть `TYPE` — константа `o3d_*` из `ksConstants3D.h`.

| Класс | TYPE | Назначение |
|---|---|---|
| `Plane`, `ParallelPlane`, `EdgePointPlane` | 1, 20, 19 | плоскости; проекции точек, вектор осей |
| `OffsetPlane` | 14 | смещённая плоскость (отрицательное смещение — против нормали) |
| `Axis`, `ConeAxis` | 71, 11 | оси |
| `Face` | 6 | грань: `GetKind`, `GetArea`, `GetNormal` (внешняя нормаль плоской грани), `GetSample` (точка и внешняя нормаль любой грани — наружный цилиндр или отверстие), `GetBox`, `GetCylinder`, `GetOwnerName`, `GetEdges` |
| `Edge` | 7 | ребро: смежные грани, вершины, `GetKind`, `GetLength`, `GetBox`, `GetPointAt(t)`, `GetPointsAt({t…})` (несколько точек за один проход), `GetRadius` |
| `Vertex` | 8 | вершина (→ `Point3D`) |
| `Sketch` | 5 | эскиз: примитивы 2D, проекция 3D-точки |
| `BaseExtrusion`, `BossExtrusion`, `CutExtrusion` | 24, 25, 26 | выдавливание: первое тело, приклеить, вырезать; `ExtrusionParams` — направление, глубины, «через всё» |
| `BaseRotated`, `BossRotated`, `CutRotated` | 27, 28, 29 | вращение: первое тело, приклеить, вырезать; `RotatedParams` (углы, направление), ось — `Axis` или осевая линия эскиза |
| `CutEvolution` | 47 | вырезать кинематически |
| `Fillet`, `Chamfer` | 34, 33 | скругление и фаска по списку рёбер или граней |
| `MeshCopy`, `CircularCopy` | 35, 36 | массивы по сетке (оси направлений — `Axis`) и концентрический |
| `CylindricSpiral` | 56 | цилиндрическая спираль |
| `ThreadDesignation` | 58 | условное изображение резьбы: создание на цилиндрической грани (`Params` — номинал, шаг, длина или на всю длину, левая; начальная грань-торец), чтение, правка длины через `SetParamMap` |
| `NodeMacro` | 63 | макрообъект (группировка операций, свои параметры) |

`Sketch::GetPlacement()` — начало и оси эскиза в координатах модели (в том числе на грани).
Определение эскиза (общий 2D-слой `Annotation2D.h`): `GetItems` (объекты со ссылками),
`Parametrize`, `AddConstraint`, `AddDimension` (управляющий размер с переменной),
`ProjectOrigin`, `Project(ребро/грань)`, `GetSupportFace`, `DeleteObject`,
`GetDefinition` (определён / недоопределён / переопределён — по КОМПАСу).
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

### Без U++ (CMake)

Зависимостей от U++ у библиотеки нет, поэтому её можно собрать и в любой другой IDE:

```cmake
add_subdirectory(KompasAPI)
target_link_libraries(МоеПриложение PRIVATE KompasAPI::KompasAPI)
```
```cpp
#include <Kompas3D.h>
```

```
cmake -B build -DKOMPAS_SDK_DIR=C:/KSDK24
```

| Переменная | Значение |
|---|---|
| `KOMPAS_SDK_DIR` | каталог SDK, по умолчанию `C:/KSDK24` |
| `KOMPASAPI_BACKEND` | `Api7` — реализация через COM (Windows + MSVC, только x64); `None` — только заголовки `Include/`, для своего бэкенда и тестов на подставных реализациях |
| `KOMPASAPI_INSTALL` | добавить правила установки, по умолчанию `OFF` |

Бэкенд `Api7` требует именно MSVC: `Api7/ComKompas.cpp` импортирует интерфейсы через
`#import "*.tlb"`. Для других компиляторов нужны заранее сгенерированные `*.tlh`/`*.tli`
в `Api7/tlh` — их подхватит `Api7/ComKompas.h`.

`Kompas3D::RunCommand` библиотека не реализует: это диспетчер команд меню, его
определяет приложение.

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
5. Если параметры операции нужно читать и менять после создания — переопределить
   `GetParamMap` / `SetParamMap` в реализации и описать имена в таблице выше.

Если узел создаётся не через `Part::NewEntity`, а извлекается из другого объекта
(как грань из ребра), метод реализации возвращает `std::unique_ptr<Node::NodeImpl>`,
а обёртка заворачивает его в нужный класс.

## Ограничения и размеры в 2D (эскиз и вид чертежа)

Справочник для общего 2D-слоя (проверено по SDK v23, справка `C:/KSDK23/Help`):

- Эскиз в режиме редактирования и вид чертежа — один 2D-документ: API5 `ksDocument2D`
  (эскиз — `ksSketchDefinition::BeginEdit`), API7 `IFragmentDocument` (`ISketch::BeginEdit`)
  или вид `IView` (`IDrawingContainer`, `ISymbols2DContainer` через QueryInterface).
- Объект API7 по ссылке API5: `KompasObject::TransferReference(ref, doc2D.reference)`.
- Отношения внутри эскиза одним вызовом: `ksDocument2D::ksParametrizeObjects(group, par)`,
  `par = GetParamStruct(ko_ParametrisationParam = 9000)`: `nearestPoints` (совпадение точек),
  `horizontal`, `vertical`, `parallel`, `perpendicular`, допуски `pointsLimit`, `angleLimit`.
- Ограничение: `IDrawingObject1::NewConstraint()` → `IParametriticConstraint`
  (`ConstraintType` — `ksConstraintTypeEnum`, `Index`/`PartnerIndex` — номера точек,
  `Partner` — второй объект или массив для ассоциации) → `Create()`.
- Размер: `ISymbols2DContainer::LineDimensions/DiametralDimensions/RadialDimensions/
  AngleDimensions` (API7) или `ksLinDimension`/`ksDiamDimension`/`ksRadDimension` (API5).
  Привязка к геометрии — `IDrawingObject1::Associate()` (по совпадающим точкам) или
  `ksCAssociation`; управляющим размер делает `ksCDimWithVariable` (переменная создаётся
  сама, `Value` — значение) или `ksCFixedDim`.
- Проверено на v23 (общий слой `Annotation2D`, `Sketch`, `Drawing::AddDimension`):
  - эскиз редактируется сеансом API7 (`ISketch::BeginEdit` → `IFragmentDocument`), документ
    API5 для рисования берётся из него (`TransferInterface`); только в таком сеансе верен
    `ConstraintsState` и работают привязки;
  - ссылки на объекты действительны в пределах одного сеанса редактирования — после
    `EndEdit` объекты получают новые ссылки;
  - объект API7 по ссылке ищется среди `IDrawingContainer::Objects` вида того же сеанса;
  - управляющий размер: сначала `ksCDimWithVariable`, затем `ksCFixedDim`; в обратном
    порядке переменная не создаётся;
  - `IParametriticConstraint::Create` и `Associate` возвращают FALSE и тогда, когда
    связь уже есть (линейный размер в совпадающих точках привязывается слиянием точек ещё
    в `Update`) — результат проверяется по списку `IDrawingObject1::Constraints`;
  - Ø и R привязываются через `BaseObject`;
  - `ksParametrizeObjects` работает на отрезках (совпадение, горизонталь, вертикаль);
  - размеры в видах чертежа ставятся в координатах вида и привязываются к линиям проекции.
  - проекции (`AddProjectionOf`) создаются основными линиями и входят в профиль операции —
    `Sketch::Project` делает их вспомогательными (стиль ставится через `ILineSegment`,
    `IArc`…: у `IDrawingObject1` в v23 свойства `Style` нет);
  - проецировать грань-опору целиком нельзя, если на ней уже есть результаты последующих
    операций: эскиз начинает зависеть от них, и модель разрушается;
  - объект «прямоугольник» размерами полностью не определяется — для определения его
    заменяют отрезками;
  - `IDrawingObject::Delete` возвращает FALSE и при удалённом объекте.
  - надпись Ø/R: `IDimensionParams::ShelfDirection` — полка вправо/влево; `IDimension2D::
    GetTextPosition` по умолчанию даёт точку на окружности, а `SetTextPosition(x, y)` задаёт
    конец выноски — так надпись уводится в свободное место (`Dimension2D::textAt`);
  - шрифты ГОСТ КОМПАСа берут символы текста размера как CP1251: «Ø» показывается как «Ш»,
    «×» — как «Ч». Знак диаметра ставится значком `IDimensionText::Sign = 1` (у линейного
    размера — `Dimension2D::sign`), «×» в надписях заменяется латинской «x».
  - знак шероховатости (`IRoughs::Add`): выноску КОМПАС строит по `IRoughParams::LeaderAngle`
    и `LeaderLength` (по умолчанию 90° и 10 мм), а не по `ShelfX/Y`, и задаются они только у
    созданного знака; точка `BranchX0/Y0` на вертикальной линии при этом сдвигается на длину
    выноски — `Drawing::AddRough` пересоздаёт знак с поправкой;
  - `IDimensionText::DeviationOn` читается FALSE и при показанных отклонениях — признак по
    текстам `HighDeviation`/`LowDeviation`; `ISpecRough::AddSign` тоже читается неверно;
  - основная надпись: `ILayoutSheet::Stamp` → `IStamp::Text(номер графы)`; графы 1
    (наименование) и 2 (обозначение) связаны со свойствами модели (`IPart7::Name`, `Marking`)
    и перезаписываются из неё — менять их надо в модели (`Part::SetTitle`); обозначение
    читается с разделителями частей «$|»;
  - разрез/сечение: линия разреза `ISymbols2DContainer::CutLines->Add()` (`Points` — x0, y0,
    x1, y1… вида), затем `IViews::Add(vt_Section)` с `IAssociationView::BaseObject` = линия и
    `Section` (TRUE — сечение). `ICutLine::ArrowPos` в v23 работает наоборот относительно справки
    (TRUE — стрелки справа по ходу линии). Вид разреза КОМПАС строит в осях базового вида: при
    взгляде «вниз» по листу изображение перевёрнуто, ход линии на это не влияет —
    `Drawing::AddSection` с `up` пересоздаёт вид с другой стороны;
  - спецзнаки в тексте: компонента `ITextItem` с `ItemType = ksTItSpecialSymbol` и `Number`
    (1 — «°», 2 — «Ø», 3 — «±», 4 — «×», 5 — «≈», 6 — «≤», 7 — «≥», 25… — знаки допусков
    формы); `ITextLine::Str` отдаёт их как «@N~». Таблица — `SDK\NumbSymb.frw`;
  - технические требования: `IDrawingDocument::TechnicalDemand`, пункты — строки `IText` с
    `Numbering = ksTNumbNumber`; перенесённые КОМПАСом продолжения — отдельные строки без
    номера. `BlocksGabarits` — по 4 числа на блок (xmin, ymin, xmax, ymax листа); при
    авторазмещении на занятом листе блок уходит за его левый край.
- Определённость: `ISketch::ConstraintsState` (`ksConstraintsStateEnum`: WellConstrained,
  UnderConstrained, UnresolvedRedundancy; с v22), по объекту — `IDrawingObject1::ConstraintsState`.
- Проекция рёбер и граней модели в эскиз: `ISketch::AddProjectionOf(IModelObject)` (с v22) —
  массив созданных объектов.

## Известные ограничения

Особенности КОМПАСа, найденные при разработке KompasMCP:

- Размещение компонента (`ksPart::GetPlacement`, и API7 `IPart7::Placement`) после
  сопряжений не обновляется — фактическое положение видно по `GetBoundingBox` компонента.
- Запись осей через API7 `IPlacement3D::SetVector` переставляет оси, поэтому
  `Part::SetPlacement` пишет через API5 `ksPlacement::SetAxes` и проверяет результат чтением.
- У смещённой плоскости с отрицательным смещением КОМПАС зеркалит оси эскиза.
- Скругление слишком большого радиуса не даёт ошибки построения — операция просто ничего
  не меняет; правка параметра может «успешно» построить операцию и сломать зависимые.
  Проверяйте объём модели и ошибки всех операций дерева.
- Каждый вызов через COM из отдельного процесса стоит около миллисекунды: на моделях с
  сотнями граней читайте только нужные свойства.

- `Part::GetEdges` (коллекция `o3d_edge`) возвращает и рёбра из истории построения, которых
  в теле уже нет; надёжный список рёбер тела — `Face::GetEdges` по всем граням.
- `Edge::GetOrigin` берёт начало размещения кривой: у рёбер, полученных из эскиза, это может
  быть начало координат эскиза, а не центр окружности. Центр надёжнее считать по точкам
  `GetPointAt`.

- У выреза (`CutExtrusion`) «прямое» направление КОМПАСа (`ExtrusionDirection::Normal`)
  идёт против нормали эскиза — в материал; у `BaseExtrusion`/`BossExtrusion` — по нормали.
- При `ExtrusionEnd::ThroughAll` глубина всё равно должна быть больше нуля.

- Чертёж: `ISheetFormat::VerticalOrientation` у листа A4 210×297 возвращает FALSE
  (альбомная), хотя лист книжный. Пока `Drawing::GetSheet` определяет ориентацию по
  размерам листа (`FormatHeight > FormatWidth`). **TODO:** разобраться, почему флаг не
  совпадает с листом (порядок записи `Format`/`VerticalOrientation`, особенность A4?).
- `ILayoutSheet::Update` возвращает FALSE, если формат не изменился; `IView::Delete` — и при
  успешном удалении. Результат проверяется чтением.
- Ассоциативные виды не обновляются `ksRebuildDocument`, а `IAssociationView::Rebuild` в v23 нет.
  `Drawing::Update` заново присваивает виду `SourceFileName` — вид перечитывает модель.
- При изменении модели, виды которой есть в открытом чертеже, КОМПАС при активации чертежа
  спрашивает «перестроить?» (модальный диалог).
- SDK должен совпадать с версией КОМПАСа: с KSDK24 при КОМПАСе v23 вызовы методов из конца
  интерфейсов API7 (`IThread::LeftThread`, `IAssociationView::Rebuild`) падали — сдвиг vtable.
  KompasMCP собирается с `C:/KSDK23`. `IAssociationView::Rebuild` в v23 нет вовсе. Автоподбор диаметра резьбы (`autoDefinDr`) на отверстии Ø10,2 дал
  «M10.741×0.5» — номинал лучше задавать явно.
- Имя узла (`ksEntity::name`) — Unicode `BSTR`; библиотека пишет и читает его без
  перекодировки в CP1251, поэтому «Ø», «×» в именах сохраняются.
- Подпись вида (имя, масштаб) выключается через `IViewDesignation` (QueryInterface от `IView`).
- `IDocuments::Count` учитывает и уже закрытые документы, их элементы пусты — `Kompas3D::ListDocuments`
  их пропускает.
- Если модель, на которую ссылается вид, открыта и изменена, КОМПАС задаёт модальный вопрос
  «сохранить?»; пока он открыт, перестроение листа отклоняется. Для автоматизации —
  `Kompas3D::SetHideMessage`.

- `Ksapi/` (Linux) не реализован — файлы содержат старый код до перехода на pimpl.
- События документа (`Doc3D::WhenCloseDocument` и прочие) объявлены, но подписка на них
  не подключена: `Doc3D` перемещается, а COM-нотификатору нужен стабильный адрес.
  Чтобы заработало, события надо перенести в `Doc3DImpl`.
- `MateConstraint::SetType` отсутствует: `IMateConstraint3D::ConstraintType` в КОМПАСе
  доступен только для чтения, тип задаётся при создании сопряжения.
- `Doc3D::CreatePorcess` — опечатка в имени намеренная: `CreateProcess` занято макросом
  Win32. По той же причине фабрика в `Doc3DImpl` называется `CreateProcessImpl`.
