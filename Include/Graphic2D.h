#ifndef _KompasAPI_Graphic2D_h_
#define _KompasAPI_Graphic2D_h_

#include <vector>

// Геометрия 2D-документа (эскиз, вид чертежа): стили линий и описание объектов

enum class LineStyle : int {
    Main = 1,                           // Основная
    Thin = 2,                           // Тонкая
    Axial = 3,                          // Осевая
    Dashed = 4,                         // Штриховая
    Break = 5,                          // Для линии обрыва
    Auxiliary = 6,                      // Вспомогательная
    Thickened = 7,                      // Утолщенная
    Dotted2 = 8,                        // Пунктир 2
    DashedMain = 9,                     // Штриховая осн
    AxialMain = 10,                     // Осевая осн
    ThinInHatching = 11,                // Тонкая линия, включаемая в штриховку
    // Стили ISO (12-25)
    Iso02_Dashed = 12,                  // ISO 02 штриховая линия
    Iso03_DashedLongSpace = 13,         // ISO 03 штриховая линия (дл. пробел)
    Iso04_ChainLongDash = 14,           // ISO 04 штрихпунктирная линия (дл. штрих)
    Iso05_ChainLongDash2Dot = 15,       // ISO 05 штрихпунктирная линия (дл. штрих 2 пунктира)
    Iso06_ChainLongDash3Dot = 16,       // ISO 06 штрихпунктирная линия (дл. штрих 3 пунктира)
    Iso07_Dotted = 17,                  // ISO 07 пунктирная линия
    Iso08_ChainLongShortDash = 18,      // ISO 08 штрихпунктирная линия (дл. и кор. штрихи)
    Iso09_ChainLong2ShortDash = 19,     // ISO 09 штрихпунктирная линия (дл. и 2 кор. штриха)
    Iso10_Chain = 20,                   // ISO 10 штрихпунктирная линия
    Iso11_Chain2Dash = 21,              // ISO 11 штрихпунктирная линия (2 штриха)
    Iso12_Chain2Dot = 22,               // ISO 12 штрихпунктирная линия (2 пунктира)
    Iso13_Chain3Dot = 23,               // ISO 13 штрихпунктирная линия (3 пунктира)
    Iso14_Chain2Dash2Dot = 24,          // ISO 14 штрихпунктирная линия (2 штриха 2 пунктира)
    Iso15_Chain2Dash3Dot = 25           // ISO 15 штрихпунктирная линия (2 штриха 3 пунктира)
};


// Объект эскиза в локальных координатах эскиза. v:
//   Line       x1 y1 x2 y2
//   Circle     xc yc r
//   Arc        xc yc r x1 y1 x2 y2 dir (1 — против часовой, -1 — по часовой)
//   Point      x y
//   Ellipse    xc yc a b angle
//   EllipseArc xc yc a b angle angle1 angle2 dir
//   Rect       x y width height angle
//   Polygon    xc yc r count describe angle
struct SketchItem {
	// Dimension — размер (линейный, угловой, Ø, R…); Other — прочие объекты (сплайны, текст…)
	enum class Kind { Line, Circle, Arc, Point, Ellipse, EllipseArc, Rect, Polygon, Dimension, Other };
	Kind kind = Kind::Other;
	LineStyle style = LineStyle::Main;
	std::vector<double> v;
	long ref = 0;   // ссылка на объект в эскизе (для ограничений и размеров)
};

#endif
