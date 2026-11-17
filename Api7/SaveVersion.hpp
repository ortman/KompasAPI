#pragma once

#include "../Include/SaveVersion.h"
#include "Node.hpp"

#include <filesystem>

// Сохранение копии документа в формате предыдущей версии КОМПАСа: IKompasDocument1::SaveAsEx
// (общий для моделей, чертежей и спецификаций)
namespace Api7SaveVersion {

inline bool Save(K7::IKompasDocumentPtr doc, const std::string& path, SaveVersion version) {
	K7::IKompasDocument1Ptr d1 = doc;
	if (!d1) throw Kompas3DException("Не могу получить IKompasDocument1 для сохранения в версию");
	const std::filesystem::path file((const wchar_t*)Utf8ToBstr(path));
	std::error_code ec;
	std::filesystem::remove(file, ec);   // судим по появлению файла: старый не должен обмануть
	d1->SaveAsEx(Utf8ToBstr(path), (long)version);
	return std::filesystem::exists(file, ec);
}

} // namespace Api7SaveVersion
