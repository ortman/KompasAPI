#ifndef _KompasAPI_ThreadSpec_h_
#define _KompasAPI_ThreadSpec_h_

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

// Резьба по стандарту: обозначение, номинальный диаметр и шаг в мм (их принимает
// условное изображение резьбы КОМПАСа), диаметр сверла под внутреннюю резьбу.
//
//   Метрическая (ГОСТ 24705, 8724, поле допуска ГОСТ 16093):  M8, M10×1,25, M12×1,25LH-6g
//   Трапецеидальная (ГОСТ 24738, 9484):                       Tr20×4, Tr20×4LH-7e
//   Трубная цилиндрическая (ГОСТ 6357, ISO 228):              G1/2, G1 1/2-A, G3/4LH
//   Дюймовая унифицированная (ASME B1.1):                     1/4-20 UNC-2B, #10-32 UNF, 0.3-18 UNS-LH
//
// Нестандартные сочетания поддерживаются: M7×0,6 — метрическая с любым диаметром и шагом,
// 0.3-18 UNS — дюймовая специальная серия. Признак standard сообщает, есть ли резьба в рядах.
// Обозначение (Designation) — в виде, принятом стандартом: знак «×», десятичная запятая.
class ThreadSpec {
public:
	enum class Kind { Metric, Trapezoidal, Pipe, Unified };

	Kind kind = Kind::Metric;
	double diameter = 0;     // номинальный (наружный) диаметр, мм
	double pitch = 0;        // шаг, мм (у дюймовых — 25,4 / число ниток на дюйм)
	bool left = false;
	bool fine = false;       // метрическая: мелкий шаг (пишется в обозначении)
	bool standard = true;    // диаметр и шаг из рядов стандарта
	std::string size;        // дюймовые и трубные: "1/2", "1 1/2", "#10", "0.3"
	double tpi = 0;          // дюймовые и трубные: ниток на дюйм
	std::string series;      // UNC, UNF, UNEF, UNS
	std::string tolerance;   // "6H", "6g", "7e", "2B", "A" — пусто, если не задано

	// Обозначение по стандарту
	std::string Designation() const {
		std::string s;
		switch (kind) {
			case Kind::Metric:
				s = "M" + Num(diameter);
				if (fine || !standard) s += "×" + Num(pitch);
				if (left) s += "LH";
				if (!tolerance.empty()) s += "-" + tolerance;
				return s;
			case Kind::Trapezoidal:
				s = "Tr" + Num(diameter) + "×" + Num(pitch);
				if (left) s += "LH";
				if (!tolerance.empty()) s += "-" + tolerance;
				return s;
			case Kind::Pipe:
				s = "G" + size;
				if (left) s += "LH";
				if (!tolerance.empty()) s += "-" + tolerance;
				return s;
			case Kind::Unified:
				s = size + "-" + Num(tpi, '.') + " " + series;
				if (!tolerance.empty()) s += "-" + tolerance;
				if (left) s += "-LH";
				return s;
		}
		return s;
	}

	// Диаметр сверла под внутреннюю резьбу, мм
	double TapDrill() const {
		if (kind == Kind::Metric) {
			for (const MetricRow& r : MetricRows()) {
				if (Near(r.diameter, diameter) && Near(r.pitch, pitch)) return r.drill;
			}
		}
		if (kind == Kind::Pipe) {
			for (const PipeRow& r : PipeRows()) {
				if (Strip(r.size) == Strip(size)) return r.drill;
			}
		}
		return std::round((diameter - pitch) * 20) / 20;   // D − P, до 0,05
	}

	// Разбор обозначения: "M8", "м10х1,25", "M12x1.25LH-6g", "Tr20x4", "G1/2", "G1 1/2-A",
	// "1/4-20 UNC-2B", "#10-32", "0.3-18 UNS". nullopt — не разобрано
	static std::optional<ThreadSpec> Parse(const std::string& text) {
		std::string s = Normalize(text);
		if (s.empty()) return std::nullopt;
		const char c0 = (char)std::toupper((unsigned char)s[0]);
		const char c1 = s.size() > 1 ? (char)std::toupper((unsigned char)s[1]) : '\0';
		if (c0 == 'T' && c1 == 'R') return ParseTrapezoidal(Strip(s.substr(2)));
		if (c0 == 'G') return ParsePipe(s.substr(1));
		if (c0 == 'M') return ParseMetric(Strip(s.substr(1)));
		return ParseUnified(s);
	}

	// Нестандартная метрическая резьба с заданными диаметром и шагом
	static ThreadSpec Custom(double diameter, double pitch) {
		ThreadSpec t;
		t.diameter = diameter;
		t.pitch = pitch;
		t.fine = true;
		t.standard = IsStandardMetric(diameter, pitch);
		return t;
	}

	// Стандартная резьба по найденным у резьбы диаметру и шагу (условное изображение)
	static ThreadSpec FromMeasured(double diameter, double pitch, bool left) {
		for (const PipeRow& r : PipeRows()) {
			if (std::abs(r.diameter - diameter) < 0.05 && std::abs(25.4 / r.tpi - pitch) < 0.01) {
				ThreadSpec t = Pipe(r);
				t.left = left;
				return t;
			}
		}
		for (const UnifiedRow& r : UnifiedRows()) {
			if (std::abs(r.inches * 25.4 - diameter) < 0.01 && std::abs(25.4 / r.tpi - pitch) < 0.005) {
				ThreadSpec t = Unified(r);
				t.left = left;
				return t;
			}
		}
		ThreadSpec t = Custom(diameter, pitch);
		auto coarse = CoarsePitch(diameter);
		t.fine = !coarse || !Near(*coarse, pitch);
		t.left = left;
		return t;
	}

	// Стандартная резьба, под которую сверлят отверстие этого диаметра (метрическая с крупным
	// шагом или трубная)
	static std::optional<ThreadSpec> ForTapDrill(double drill) {
		for (const MetricRow& r : MetricRows()) {
			if (std::abs(r.drill - drill) < 1e-3) return Metric(r.diameter, r.pitch);
		}
		for (const PipeRow& r : PipeRows()) {
			if (std::abs(r.drill - drill) < 1e-3) return Pipe(r);
		}
		return std::nullopt;
	}

	// Стандартная резьба на валу этого диаметра (метрическая с крупным шагом или трубная)
	static std::optional<ThreadSpec> ForNominal(double diameter) {
		if (auto pitch = CoarsePitch(diameter)) return Metric(diameter, *pitch);
		for (const PipeRow& r : PipeRows()) {
			if (std::abs(r.diameter - diameter) < 0.05) return Pipe(r);
		}
		return std::nullopt;
	}

	// Крупный шаг метрической резьбы стандартного диаметра
	static std::optional<double> CoarsePitch(double diameter) {
		for (const MetricRow& r : MetricRows()) {
			if (Near(r.diameter, diameter)) return r.pitch;
		}
		return std::nullopt;
	}

private:
	struct MetricRow { double diameter, pitch, drill; std::vector<double> fine; };
	struct PipeRow { const char* size; double diameter, tpi, drill; };
	struct UnifiedRow { const char* size; double inches, tpi; const char* series; };

	// ГОСТ 24705 / 8724: крупный шаг, сверло (ГОСТ 19257), мелкие шаги
	static const std::vector<MetricRow>& MetricRows() {
		static const std::vector<MetricRow> rows = {
			{1, 0.25, 0.75, {0.2}},         {1.2, 0.25, 0.95, {0.2}},       {1.6, 0.35, 1.25, {0.2}},
			{2, 0.4, 1.6, {0.25}},          {2.5, 0.45, 2.05, {0.35}},      {3, 0.5, 2.5, {0.35}},
			{4, 0.7, 3.3, {0.5}},           {5, 0.8, 4.2, {0.5}},           {6, 1, 5.0, {0.75, 0.5}},
			{8, 1.25, 6.8, {1, 0.75, 0.5}}, {10, 1.5, 8.5, {1.25, 1, 0.75, 0.5}},
			{12, 1.75, 10.2, {1.5, 1.25, 1, 0.75, 0.5}}, {14, 2, 12.0, {1.5, 1.25, 1, 0.75, 0.5}},
			{16, 2, 14.0, {1.5, 1, 0.75, 0.5}},          {18, 2.5, 15.5, {2, 1.5, 1, 0.75, 0.5}},
			{20, 2.5, 17.5, {2, 1.5, 1, 0.75, 0.5}},     {22, 2.5, 19.5, {2, 1.5, 1, 0.75, 0.5}},
			{24, 3, 21.0, {2, 1.5, 1, 0.75}},  {27, 3, 24.0, {2, 1.5, 1, 0.75}},  {30, 3.5, 26.5, {3, 2, 1.5, 1, 0.75}},
			{33, 3.5, 29.5, {3, 2, 1.5, 1, 0.75}}, {36, 4, 32.0, {3, 2, 1.5, 1}}, {39, 4, 35.0, {3, 2, 1.5, 1}},
			{42, 4.5, 37.5, {4, 3, 2, 1.5, 1}}, {48, 5, 43.0, {4, 3, 2, 1.5, 1}}, {56, 5.5, 50.5, {4, 3, 2, 1.5, 1}},
			{64, 6, 58.0, {4, 3, 2, 1.5, 1}},
		};
		return rows;
	}

	// ГОСТ 6357: наружный диаметр, ниток на дюйм, сверло под внутреннюю резьбу
	static const std::vector<PipeRow>& PipeRows() {
		static const std::vector<PipeRow> rows = {
			{"1/8", 9.728, 28, 8.8},      {"1/4", 13.157, 19, 11.8},    {"3/8", 16.662, 19, 15.25},
			{"1/2", 20.955, 14, 19.0},    {"5/8", 22.911, 14, 21.0},    {"3/4", 26.441, 14, 24.5},
			{"7/8", 30.201, 14, 28.25},   {"1", 33.249, 11, 30.75},     {"1 1/8", 37.897, 11, 35.5},
			{"1 1/4", 41.910, 11, 39.5},  {"1 1/2", 47.803, 11, 45.25}, {"1 3/4", 53.746, 11, 51.0},
			{"2", 59.614, 11, 57.0},      {"2 1/4", 65.710, 11, 63.0},  {"2 1/2", 75.184, 11, 72.5},
			{"3", 87.884, 11, 85.25},
		};
		return rows;
	}

	// ASME B1.1: размер, диаметр в дюймах, ниток на дюйм, серия
	static const std::vector<UnifiedRow>& UnifiedRows() {
		static const std::vector<UnifiedRow> rows = {
			{"#2", 0.086, 56, "UNC"},  {"#4", 0.112, 40, "UNC"},  {"#5", 0.125, 40, "UNC"},  {"#6", 0.138, 32, "UNC"},
			{"#8", 0.164, 32, "UNC"},  {"#10", 0.190, 24, "UNC"}, {"#12", 0.216, 24, "UNC"}, {"1/4", 0.25, 20, "UNC"},
			{"5/16", 0.3125, 18, "UNC"}, {"3/8", 0.375, 16, "UNC"}, {"7/16", 0.4375, 14, "UNC"}, {"1/2", 0.5, 13, "UNC"},
			{"9/16", 0.5625, 12, "UNC"}, {"5/8", 0.625, 11, "UNC"}, {"3/4", 0.75, 10, "UNC"}, {"7/8", 0.875, 9, "UNC"},
			{"1", 1.0, 8, "UNC"},      {"1 1/8", 1.125, 7, "UNC"}, {"1 1/4", 1.25, 7, "UNC"}, {"1 1/2", 1.5, 6, "UNC"},
			{"2", 2.0, 4.5, "UNC"},
			{"#2", 0.086, 64, "UNF"},  {"#4", 0.112, 48, "UNF"},  {"#6", 0.138, 40, "UNF"},  {"#8", 0.164, 36, "UNF"},
			{"#10", 0.190, 32, "UNF"}, {"#12", 0.216, 28, "UNF"}, {"1/4", 0.25, 28, "UNF"},  {"5/16", 0.3125, 24, "UNF"},
			{"3/8", 0.375, 24, "UNF"}, {"7/16", 0.4375, 20, "UNF"}, {"1/2", 0.5, 20, "UNF"}, {"9/16", 0.5625, 18, "UNF"},
			{"5/8", 0.625, 18, "UNF"}, {"3/4", 0.75, 16, "UNF"},  {"7/8", 0.875, 14, "UNF"}, {"1", 1.0, 12, "UNF"},
			{"1 1/4", 1.25, 12, "UNF"}, {"1 1/2", 1.5, 12, "UNF"},
			{"1/4", 0.25, 32, "UNEF"}, {"5/16", 0.3125, 32, "UNEF"}, {"3/8", 0.375, 32, "UNEF"}, {"1/2", 0.5, 28, "UNEF"},
			{"5/8", 0.625, 24, "UNEF"}, {"3/4", 0.75, 20, "UNEF"}, {"1", 1.0, 20, "UNEF"},
		};
		return rows;
	}

	static bool Near(double a, double b) { return std::abs(a - b) < 1e-6; }

	// Число без лишних нулей; десятичный разделитель по ГОСТ — запятая
	static std::string Num(double v, char separator = ',') {
		char buffer[32];
		std::snprintf(buffer, sizeof buffer, "%.4f", v);
		std::string s = buffer;
		while (s.back() == '0') s.pop_back();
		if (s.back() == '.') s.pop_back();
		std::replace(s.begin(), s.end(), '.', separator);
		return s;
	}

	static std::string Strip(const std::string& s) {
		std::string out;
		for (char c : s) {
			if (c != ' ') out += c;
		}
		return out;
	}

	static bool IsStandardMetric(double diameter, double pitch) {
		for (const MetricRow& r : MetricRows()) {
			if (!Near(r.diameter, diameter)) continue;
			if (Near(r.pitch, pitch)) return true;
			for (double f : r.fine) {
				if (Near(f, pitch)) return true;
			}
		}
		return false;
	}

	static ThreadSpec Metric(double diameter, double pitch) {
		ThreadSpec t;
		t.diameter = diameter;
		t.pitch = pitch;
		auto coarse = CoarsePitch(diameter);
		t.fine = !coarse || !Near(*coarse, pitch);
		t.standard = IsStandardMetric(diameter, pitch);
		return t;
	}

	static ThreadSpec Pipe(const PipeRow& r) {
		ThreadSpec t;
		t.kind = Kind::Pipe;
		t.size = r.size;
		t.diameter = r.diameter;
		t.tpi = r.tpi;
		t.pitch = 25.4 / r.tpi;
		return t;
	}

	static ThreadSpec Unified(const UnifiedRow& r) {
		ThreadSpec t;
		t.kind = Kind::Unified;
		t.size = r.size;
		t.diameter = r.inches * 25.4;
		t.tpi = r.tpi;
		t.pitch = 25.4 / r.tpi;
		t.series = r.series;
		return t;
	}

	// Кириллица М/Тр/х → латиница, «×» и «*» → X, запятая → точка; регистр сохраняется
	// (6H — внутренняя резьба, 6g — наружная).
	// Пробелы сохраняются одиночными (нужны в «G1 1/2» и «1/4-20 UNC»)
	static std::string Normalize(const std::string& text) {
		std::string out;
		for (size_t i = 0; i < text.size(); ++i) {
			unsigned char c = (unsigned char)text[i];
			unsigned char n = i + 1 < text.size() ? (unsigned char)text[i + 1] : 0;
			if (c == 0xD0 && (n == 0x9C || n == 0xBC)) { out += 'M'; ++i; continue; }   // М м
			if (c == 0xD0 && (n == 0xA2 || n == 0xB2)) { out += 'T'; ++i; continue; }   // Т т
			if (c == 0xD1 && n == 0x80) { out += 'R'; ++i; continue; }                  // р
			if (c == 0xD0 && n == 0xA0) { out += 'R'; ++i; continue; }                  // Р
			if (c == 0xD0 && n == 0xA5) { out += 'X'; ++i; continue; }                  // Х
			if (c == 0xD1 && n == 0x85) { out += 'X'; ++i; continue; }                  // х
			if (c == 0xC3 && n == 0x97) { out += 'X'; ++i; continue; }                  // ×
			if (c == '*') { out += 'X'; continue; }
			if (c == ',') { out += '.'; continue; }
			if (c == '"' || c == '\'') continue;   // 1/4" — знак дюйма
			if (std::isspace(c)) {
				if (!out.empty() && out.back() != ' ') out += ' ';
				continue;
			}
			out += (char)c;
		}
		while (!out.empty() && out.back() == ' ') out.pop_back();
		return out;
	}

	// Хвост обозначения: LH и поле допуска (класс) в любом порядке: "LH-6g", "-6H", "-2B-LH", "-A"
	static bool ParseTail(const std::string& tail, ThreadSpec& t) {
		std::string token;
		auto flush = [&]() {
			if (token.empty()) return true;
			std::string upper = token;
			for (char& c : upper) c = (char)std::toupper((unsigned char)c);
			bool ok = true;
			if (upper == "LH") {
				t.left = true;
			} else if (t.tolerance.empty() && std::all_of(token.begin(), token.end(), [](char c) {
				           return std::isalnum((unsigned char)c);
			           })) {
				t.tolerance = token;
			} else if (upper.size() > 2 && upper.compare(upper.size() - 2, 2, "LH") == 0 && t.tolerance.empty()) {
				t.left = true;   // "6gLH"
				t.tolerance = token.substr(0, token.size() - 2);
			} else {
				ok = false;
			}
			token.clear();
			return ok;
		};
		for (char c : tail) {
			if (c == '-' || c == ' ') {
				if (!flush()) return false;
			} else {
				token += c;
			}
		}
		return flush();
	}

	static std::optional<ThreadSpec> ParseMetric(const std::string& s) {
		char* end = nullptr;
		const double d = std::strtod(s.c_str(), &end);
		if (end == s.c_str() || d <= 0) return std::nullopt;
		std::string rest = end;
		double p = 0;
		if (!rest.empty() && (rest[0] == 'X' || rest[0] == 'x')) {
			const char* ps = rest.c_str() + 1;
			p = std::strtod(ps, &end);
			if (end == ps || p <= 0) return std::nullopt;
			rest = end;
		} else {
			auto coarse = CoarsePitch(d);
			if (!coarse) return std::nullopt;   // нестандартный диаметр без шага
			p = *coarse;
		}
		if (p >= d) return std::nullopt;
		ThreadSpec t = Metric(d, p);
		if (!ParseTail(rest, t)) return std::nullopt;
		return t;
	}

	static std::optional<ThreadSpec> ParseTrapezoidal(const std::string& s) {
		char* end = nullptr;
		const double d = std::strtod(s.c_str(), &end);
		if (end == s.c_str() || d <= 0 || (*end != 'X' && *end != 'x')) return std::nullopt;
		const char* ps = end + 1;
		const double p = std::strtod(ps, &end);
		if (end == ps || p <= 0 || p >= d) return std::nullopt;
		ThreadSpec t;
		t.kind = Kind::Trapezoidal;
		t.diameter = d;
		t.pitch = p;
		if (!ParseTail(end, t)) return std::nullopt;
		return t;
	}

	static std::optional<ThreadSpec> ParsePipe(const std::string& s) {
		std::string body = s;
		while (!body.empty() && body[0] == ' ') body = body.substr(1);
		// Самый длинный подходящий размер: "1 1/2" раньше "1"
		const PipeRow* best = nullptr;
		size_t bestLength = 0;
		for (const PipeRow& r : PipeRows()) {
			const std::string key = r.size;
			if (body.rfind(key, 0) == 0 && key.size() > bestLength) {
				best = &r;
				bestLength = key.size();
			}
			const std::string packed = Strip(key);
			if (packed != key && body.rfind(packed, 0) == 0 && packed.size() > bestLength) {
				best = &r;
				bestLength = packed.size();
			}
		}
		if (!best) return std::nullopt;
		ThreadSpec t = Pipe(*best);
		std::string tail = body.substr(bestLength);
		if (!tail.empty() && std::isdigit((unsigned char)tail[0])) return std::nullopt;   // "G12" — не размер ряда
		if (!ParseTail(tail, t)) return std::nullopt;
		return t;
	}

	static std::optional<ThreadSpec> ParseUnified(const std::string& s) {
		// размер-TPI [серия][-класс][-LH]; размер: #10, 1/4, 1 1/4, 0.3
		const size_t dash = s.find('-');
		if (dash == std::string::npos || dash == 0) return std::nullopt;
		std::string size = s.substr(0, dash);
		while (!size.empty() && size.back() == ' ') size.pop_back();
		double inches = 0;
		if (size[0] == '#') {
			char* end = nullptr;
			const long n = std::strtol(size.c_str() + 1, &end, 10);
			if (*end != '\0' || n < 0 || n > 12) return std::nullopt;
			inches = 0.060 + 0.013 * n;
		} else {
			std::string whole, frac = size;
			if (const size_t sp = size.find(' '); sp != std::string::npos) {
				whole = size.substr(0, sp);
				frac = size.substr(sp + 1);
			}
			if (const size_t slash = frac.find('/'); slash != std::string::npos) {
				const double num = std::atof(frac.substr(0, slash).c_str());
				const double den = std::atof(frac.substr(slash + 1).c_str());
				if (num <= 0 || den <= 0) return std::nullopt;
				inches = num / den + (whole.empty() ? 0.0 : std::atof(whole.c_str()));
			} else {
				char* end = nullptr;
				inches = std::strtod(size.c_str(), &end);
				if (*end != '\0') return std::nullopt;
			}
		}
		if (inches <= 0) return std::nullopt;
		char* end = nullptr;
		const char* ts = s.c_str() + dash + 1;
		const double tpi = std::strtod(ts, &end);
		if (end == ts || tpi <= 0) return std::nullopt;
		std::string rest = end;
		while (!rest.empty() && rest[0] == ' ') rest = rest.substr(1);
		std::string series;
		std::string upperRest = rest;
		for (char& c : upperRest) c = (char)std::toupper((unsigned char)c);
		for (const char* name : {"UNEF", "UNC", "UNF", "UNS", "UN"}) {
			if (upperRest.rfind(name, 0) == 0) {
				series = name;
				rest = rest.substr(series.size());
				break;
			}
		}
		ThreadSpec t;
		t.kind = Kind::Unified;
		t.size = size;
		t.diameter = inches * 25.4;
		t.tpi = tpi;
		t.pitch = 25.4 / tpi;
		t.standard = false;
		for (const UnifiedRow& r : UnifiedRows()) {
			if (std::abs(r.inches - inches) < 1e-4 && Near(r.tpi, tpi) && (series.empty() || series == r.series)) {
				t.series = r.series;
				t.standard = true;
				break;
			}
		}
		if (t.series.empty()) t.series = series.empty() || series == "UN" ? "UNS" : series;
		if (!t.standard && (t.series == "UNC" || t.series == "UNF" || t.series == "UNEF")) {
			t.series = "UNS";   // нет в ряду серии — специальная
		}
		if (!ParseTail(rest, t)) return std::nullopt;
		return t;
	}
};

#endif
