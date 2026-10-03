#ifndef _KompasAPI_Annotation2D_h_
#define _KompasAPI_Annotation2D_h_

#include <optional>
#include <string>
#include <vector>

#include "Graphic2D.h"

// Ограничения и размеры 2D-документа КОМПАСа. Общие для эскиза (в режиме редактирования)
// и вида чертежа: это один и тот же графический документ (ksDocument2D). Объекты
// адресуются ссылками API5 (long) текущего документа; координаты — в системе эскиза или вида.

// Параметрические ограничения (значения ksConstraintTypeEnum)
enum class Constraint2D : int {
	FixedPoint = 1,          // зафиксировать точку
	PointOnCurve = 2,        // точка на кривой
	Horizontal = 3,
	Vertical = 4,
	Parallel = 5,
	Perpendicular = 6,
	EqualLength = 7,
	EqualRadius = 8,
	HAlignPoints = 9,        // две точки на одной горизонтали
	VAlignPoints = 10,       // две точки на одной вертикали
	MergePoints = 11,        // совпадение точек
	Symmetry = 16,           // симметрия двух точек относительно отрезка
	Collinear = 17,
	PointOnCurveMiddle = 20,
	Concentric = 22,
};

// Автоматическая параметризация группы объектов (команда «Параметризовать» КОМПАСа)
struct Parametrize2D {
	bool mergePoints = true;    // совпадающие точки → «совпадение точек»
	bool horizontal = true;
	bool vertical = true;
	bool parallel = true;
	bool perpendicular = true;
	double pointsLimit = 0.001; // допуск совпадения точек, мм
	double angleLimit = 0.01;   // угловой допуск, градусы
};

// Размер. Линейный: измеряемые точки (x1, y1) и (x2, y2); размерная линия проходит на
// расстоянии offset от первой точки: у горизонтального — по Y, у вертикального — по X, у
// параллельного — влево от направления 1→2. Линейный размер привязывается к геометрии по
// совпадающим точкам. Диаметр и радиус ставятся на окружность или дугу object, размерная
// линия под углом angle (градусы).
struct Dimension2D {
	enum class Kind { Horizontal, Vertical, Aligned, Diameter, Radius };
	Kind kind = Kind::Horizontal;
	double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	long object = 0;            // Ø/R: окружность или дуга
	double offset = 10;
	double angle = 45;
	bool driving = true;        // управляющий размер: «размер с переменной»
	std::string text;           // свой текст вместо значения (на чертеже: «M8-6H»), без знака Ø
	std::string prefix;         // текст перед значением (на чертеже: «2 отв. »)
	std::string suffix;         // текст после значения (на чертеже: «x90°»)
	int shelf = 0;              // Ø/R: горизонтальная полка вправо (1) или влево (-1), 0 — по линии
	int sign = -1;              // значок перед номиналом (IDimensionText::Sign): 1 — Ø у линейного размера; -1 — как есть
	bool textAt = false;        // Ø/R: надпись в точке textX, textY (иначе КОМПАС ставит сам у окружности)
	double textX = 0, textY = 0;
};

struct DimensionInfo {
	long ref = 0;               // 0 — размер не создан
	bool associated = false;    // привязан к геометрии
	bool driving = false;       // управляющий
	std::string variable;       // имя переменной управляющего размера
	double textX = 0, textY = 0; // точка надписи после создания (IDimension2D)
};

// Допуск размера: поле допуска с квалитетом и/или предельные отклонения
struct DimensionTolerance {
	std::string fit;                    // «H7», «h14»; пусто — без квалитета
	bool showFit = true;                // квалитет в надписи
	bool deviations = false;            // предельные отклонения в надписи (по квалитету или заданные)
	std::optional<double> upper, lower; // заданные отклонения, мм (без квалитета — обязательно оба)
};

// Размер, прочитанный из вида
struct DimensionRead {
	long ref = 0;
	int type = 0;                       // DrawingObjectType: 9 линейный, 10 угловой, 13 Ø, 14 R…
	double value = 0;                   // номинал, мм или градусы
	int sign = 0;                       // значок: 1 — Ø
	std::string prefix, suffix, text;   // text — свой текст вместо номинала
	std::string fit;                    // квалитет, если включён
	bool deviations = false;
	std::string upper, lower;           // тексты отклонений в надписи («+0,015», «-0,1»)
	int orientation = -1;               // линейный: ksLineDimensionOrientationEnum (1 — горизонтальный, 2 — вертикальный)
	double points[6] = {0, 0, 0, 0, 0, 0}; // линейный: x1, y1, x2, y2 и точка размерной линии x3, y3 (координаты вида)
	bool hasBox = false;                // габарит размера (с выносными линиями и надписью)
	double box[4] = {0, 0, 0, 0};       // xmin, ymin, xmax, ymax — координаты листа
};

// Обозначение шероховатости поверхности: знак на линии вида (object) в точке x, y
struct Rough2D {
	long object = 0;                    // линия или окружность вида
	double x = 0, y = 0;                // точка на линии (координаты вида)
	std::string text;                   // «Ra 1,6»
	int sign = 1;                       // ksRoughSignEnum: 0 — без указания способа, 1 — со снятием материала, 2 — без снятия
	bool leader = false;                // на полке выноски: shelfX, shelfY — начало полки
	double shelfX = 0, shelfY = 0;      // без выноски — точка со стороны знака (снаружи материала)
};

// Определённость эскиза (значения ksConstraintsStateEnum)
enum class SketchDefinition : int {
	Unknown = 0,
	Defined = 1,                // полностью определён
	Under = 2,                  // недоопределён: есть свободные степени
	Redundant = 3,              // избыточные (конфликтующие) ограничения
};

// Определённость объекта 2D (значения ksObjectConstraintsStateEnum)
enum class ObjectDefinition : int {
	Unknown = 0,                // не определён полностью
	Defined = 1,
	Over = 2,                   // переопределён
	Informative = 3,            // информационный (неуправляющий) размер
	WrongProjection = 4,        // потерянная проекция
};

#endif
