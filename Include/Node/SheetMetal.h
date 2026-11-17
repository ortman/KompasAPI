#ifndef _KompasAPI_SheetMetal_h_
#define _KompasAPI_SheetMetal_h_

#include "Sketch.h"

#include <optional>
#include <string>
#include <vector>

// Листовое моделирование: листовое тело, сгибы по рёбрам, вырезы, замыкание углов,
// штамповки, развёртка (Part::SetSheetMetalUnfolded).
// Узлы создаются как остальные операции: part.Create<SheetMetalBend>(nullptr, edges, params, name).
// Ошибка построения — GetError() != 0 (узел остаётся в дереве, удаляет вызывающий).
//
// Проверено на КОМПАСе v24:
// - тело из замкнутого эскиза — пластина, из незамкнутого — профиль на глубину (углы
//   сгибаются радиусом тела); толщина откладывается по нормали эскиза;
// - сгиб берёт свой радиус (по умолчанию 5), а не радиус тела — задавать явно;
// - сгиб, штамповки, буртик и жалюзи строятся наружу от грани ребра или эскиза при
//   Direction = TRUE, хотя у нового объекта Direction читается FALSE; явный FALSE — в другую сторону;
// - вырез строится только по эскизу на грани листа (эскиз на плоскости под листом не режет);
// - объект замыкания угла — плоский торец зоны сгиба на углу, один на угол;
// - величина и уменьшение сгиба не меняют модель, только длину развёртки.

// Расчёт развёртки сгиба: коэффициент нейтрального слоя, величина сгиба (длина дуги
// сгиба в развёртке) или уменьшение сгиба, мм
struct BendAllowance {
	enum class Kind { Coefficient = 0, Value = 1, Reduction = 2 };
	Kind kind = Kind::Coefficient;
	double value = 0.4;
};

// Замыкание угла (ksClosingClosedTypeEnum)
enum class CornerClosing { Joint = 0, Overlap = 1, Tight = 2 };

struct SheetMetalParams {
	double thickness = 0;     // тело
	double radius = 0;        // тело и сгиб: внутренний радиус сгиба; 0 — по умолчанию КОМПАСа
	double depth = 0;         // тело из незамкнутого профиля: глубина выдавливания
	int direction = 0;        // тело из профиля: 0 — по нормали, 1 — обратно, 3 — симметрично
	bool reverse = false;     // тело — толщина в другую сторону; сгиб и штамповки — в другую сторону
	double length = 0;        // сгиб: длина полки
	double angle = 90;        // сгиб: угол сгиба
	double height = 0;        // штамповки, жалюзи, буртик
	double width = 0;         // жалюзи, буртик
	CornerClosing closing = CornerClosing::Joint;
	double gap = 0;           // замыкание углов: зазор
	std::optional<BendAllowance> allowance;
	// Сгиб по рёбрам: размещение относительно ребра (ksBendOffsetTypeEnum) и способ отсчёта
	// длины (ksBendLengthTypeEnum); -1 — как у КОМПАСа по умолчанию
	int placement = -1;
	int lengthType = -1;
	// Боковые стороны сгиба: уклон (градусы) или расширение (мм); слева и справа — как в КОМПАСе
	struct Side {
		std::optional<double> taper;
		std::optional<double> widen;
	};
	Side left, right;
	// Сгиб по линии: положение сгиба относительно линии (ksBendTypeEnum), -1 — по умолчанию;
	// fixOtherSide — неподвижной остаётся другая сторона от линии
	int bendType = -1;
	bool fixOtherSide = false;
};

class SheetMetalImpl : virtual public Node::NodeImpl {
public:
	virtual void SetSketch(Sketch& sketch) {}
	virtual void SetObjects(const std::vector<const Node*>& objects) {}
	virtual void SetParams(const SheetMetalParams& params) = 0;
};

template <int Type>
class SheetMetalNode : public Node {
public:
	static inline int TYPE = Type;
	SheetMetalNode(std::unique_ptr<NodeImpl> p, Sketch* sketch, const std::vector<const Node*>& objects,
	               const SheetMetalParams& params, const std::optional<std::string>& name = std::nullopt) : Node(std::move(p)) {
		if (sketch) sketch->EndEdit();
		if (name.has_value()) node->SetName(name.value());
		if (SheetMetalImpl* s = dynamic_cast<SheetMetalImpl*>(node.get())) {
			if (sketch) s->SetSketch(*sketch);
			if (!objects.empty()) s->SetObjects(objects);
			s->SetParams(params);
		}
		node->Create();
	}
	SheetMetalNode(Node&& node) : Node(std::move(node)) {}
};

// Листовое тело по эскизу: thickness, radius, depth/direction (незамкнутый профиль), allowance
using SheetMetalBody = SheetMetalNode<74>;           /* o3d_sheetMetalBody */
// Сгиб по рёбрам (objects): length, angle, radius, reverse, allowance
using SheetMetalBend = SheetMetalNode<75>;           /* o3d_sheetMetalBend */
// Сгиб по линии: sketch — эскиз с одним отрезком на грани листа, objects — эта грань;
// angle, radius, reverse, bendType, fixOtherSide, allowance
using SheetMetalLineBend = SheetMetalNode<76>;       /* o3d_sheetMetalLineBend */
// Пластина: замкнутый эскиз на грани листа добавляет к листу материал той же толщины
using SheetMetalPlate = SheetMetalNode<534>;         /* o3d_sheetMetalPlate */
// Вырез по эскизу на грани листа, сквозь толщину
using SheetMetalCut = SheetMetalNode<78>;            /* o3d_sheetMetalCut */
// Замыкание углов: objects — торцы зон сгибов на углах, closing, gap
using SheetMetalClosedCorner = SheetMetalNode<539>;  /* o3d_sheetMetalClosedCorner */
// Закрытая и открытая штамповка по замкнутому контуру: height, reverse
using SheetMetalClosedStamp = SheetMetalNode<541>;   /* o3d_sheetMetalDimpleCutout */
using SheetMetalOpenStamp = SheetMetalNode<542>;     /* o3d_sheetMetalDrawnCutout */
// Буртик по линиям эскиза: height, width, reverse
using SheetMetalBead = SheetMetalNode<543>;          /* o3d_sheetMetalBeat */
// Жалюзи по отрезкам эскиза: height, width, reverse
using SheetMetalLouver = SheetMetalNode<544>;        /* o3d_sheetMetalLouver */

#endif
