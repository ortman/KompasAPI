#ifndef _KompasAPI_Extrusion_h_
#define _KompasAPI_Extrusion_h_

// Общие параметры операций выдавливания (BaseExtrusion, BossExtrusion, CutExtrusion)

enum class ExtrusionDirection : int {
	Normal = 0,       // dtNormal: прямое направление (по нормали эскиза)
	Reverse = 1,      // dtReverse: обратное направление
	Both = 2,         // dtBoth: в обе стороны, depth1 и depth2 отдельно
	MiddlePlane = 3   // dtMiddlePlane: симметрично, depth1 — полная глубина
};

enum class ExtrusionEnd : int {
	Blind = 0,        // etBlind: на расстояние
	ThroughAll = 1    // etThroughAll: через всё
};

struct ExtrusionParams {
	double depth1 = 10.0;                 // глубина в прямом направлении
	double depth2 = 0.0;                  // глубина в обратном направлении (для Both)
	ExtrusionDirection direction = ExtrusionDirection::Normal;
	ExtrusionEnd end1 = ExtrusionEnd::Blind;
	ExtrusionEnd end2 = ExtrusionEnd::Blind;
};

#endif
