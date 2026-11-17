#ifndef _KompasAPI_SaveVersion_h_
#define _KompasAPI_SaveVersion_h_

#include <optional>
#include <string>
#include <vector>

// Версия формата файла для сохранения документа в предыдущих версиях КОМПАСа
// (значения ksSaveDocumentVersionEnum, IKompasDocument1::SaveAsEx). Сохраняется копия:
// документ остаётся связан со своим файлом и версией.
//
// Проверено на КОМПАСе v24:
// - неизвестный номер КОМПАС молча принимает и пишет файл, поэтому номера берутся только
//   из этого перечисления;
// - ksSDV_Prev (-1, «в предыдущую версию») пишет файл версии 16 SP1, а не 23 — его нет;
// - файл, записанный для версий 15–22, читается обратно с этим же номером (OpenVersion);
//   для версии 23 OpenVersion отдаёт номер потока, а не 29.
enum class SaveVersion : int {
	Kompas5_11 = 1,
	Kompas6 = 2,
	Kompas6Plus = 3,
	Kompas7 = 4,
	Kompas7Plus = 5,
	Kompas8 = 6,
	Kompas8Plus = 7,
	Kompas9 = 8,
	Kompas10 = 9,
	Kompas11 = 10,
	Kompas12 = 11,
	Kompas13 = 12,
	Kompas14 = 13,
	Kompas14Sp1 = 14,
	Kompas14Sp2 = 15,
	Kompas15 = 16,
	Kompas15Sp1 = 17,
	Kompas15Sp2 = 18,
	Kompas16 = 19,
	Kompas16Sp1 = 20,
	Kompas17 = 21,
	Kompas17Sp1 = 22,
	Kompas18 = 23,
	Kompas18Sp1 = 24,
	Kompas19 = 25,
	Kompas20 = 26,
	Kompas21 = 27,
	Kompas22 = 28,
	Kompas23 = 29,
};

struct SaveVersionInfo {
	SaveVersion version;
	const char* name;   // как пишет КОМПАС: "17", "17 SP1", "6 Plus", "5.11"
};

// Все версии, от старых к новым
inline const std::vector<SaveVersionInfo>& SaveVersions() {
	static const std::vector<SaveVersionInfo> all = {
		{SaveVersion::Kompas5_11, "5.11"},    {SaveVersion::Kompas6, "6"},
		{SaveVersion::Kompas6Plus, "6 Plus"}, {SaveVersion::Kompas7, "7"},
		{SaveVersion::Kompas7Plus, "7 Plus"}, {SaveVersion::Kompas8, "8"},
		{SaveVersion::Kompas8Plus, "8 Plus"}, {SaveVersion::Kompas9, "9"},
		{SaveVersion::Kompas10, "10"},        {SaveVersion::Kompas11, "11"},
		{SaveVersion::Kompas12, "12"},        {SaveVersion::Kompas13, "13"},
		{SaveVersion::Kompas14, "14"},        {SaveVersion::Kompas14Sp1, "14 SP1"},
		{SaveVersion::Kompas14Sp2, "14 SP2"}, {SaveVersion::Kompas15, "15"},
		{SaveVersion::Kompas15Sp1, "15 SP1"}, {SaveVersion::Kompas15Sp2, "15 SP2"},
		{SaveVersion::Kompas16, "16"},        {SaveVersion::Kompas16Sp1, "16 SP1"},
		{SaveVersion::Kompas17, "17"},        {SaveVersion::Kompas17Sp1, "17 SP1"},
		{SaveVersion::Kompas18, "18"},        {SaveVersion::Kompas18Sp1, "18 SP1"},
		{SaveVersion::Kompas19, "19"},        {SaveVersion::Kompas20, "20"},
		{SaveVersion::Kompas21, "21"},        {SaveVersion::Kompas22, "22"},
		{SaveVersion::Kompas23, "23"},
	};
	return all;
}

inline const char* SaveVersionName(SaveVersion version) {
	for (const SaveVersionInfo& v : SaveVersions()) {
		if (v.version == version) return v.name;
	}
	return "";
}

// По имени, как в SaveVersions ("21", "17 SP1"); регистр SP и пробел не важны ("17sp1")
inline std::optional<SaveVersion> SaveVersionByName(const std::string& name) {
	auto norm = [](const std::string& s) {
		std::string out;
		for (char c : s) {
			if (c == ' ' || c == '_') continue;
			out += (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
		}
		return out;
	};
	const std::string key = norm(name);
	for (const SaveVersionInfo& v : SaveVersions()) {
		if (norm(v.name) == key) return v.version;
	}
	return std::nullopt;
}

#endif
