#pragma once

#include "../Include/Specification.h"

#include <filesystem>

class SpecificationApi7 : public Specification::SpecificationImpl {
	K7::IKompasDocumentPtr doc7;

	K7::ISpecificationDescriptionPtr Description() {
		K7::ISpecificationDescriptionsPtr descriptions = doc7 ? doc7->SpecificationDescriptions : nullptr;
		return descriptions ? descriptions->GetActive() : nullptr;
	}

	static std::string ColumnText(K7::ISpecificationObjectPtr object, KConst::ksSpecificationColumnTypeEnum type) {
		K7::ISpecificationColumnsPtr columns = object ? object->Columns : nullptr;
		K7::ISpecificationColumnPtr column = columns ? columns->GetColumn(type, 1, 0) : nullptr;
		K7::ITextPtr text = column ? column->Text : nullptr;
		return text ? BstrToUtf8(text->Str) : std::string();
	}

	static bool SetColumn(K7::ISpecificationObjectPtr object, KConst::ksSpecificationColumnTypeEnum type, const std::string& value) {
		if (value.empty()) return true;
		K7::ISpecificationColumnsPtr columns = object ? object->Columns : nullptr;
		K7::ISpecificationColumnPtr column = columns ? columns->GetColumn(type, 1, 0) : nullptr;
		K7::ITextPtr text = column ? column->Text : nullptr;
		if (!text) return false;
		text->Str = Utf8ToBstr(value);
		return true;
	}

public:
	explicit SpecificationApi7(K7::IKompasDocumentPtr d) : doc7(d) {}

	bool AddRow(const Specification::Row& row) override {
		K7::ISpecificationDescriptionPtr description = Description();
		K7::ISpecificationBaseObjectsPtr objects = description ? description->BaseObjects : nullptr;
		if (!objects) throw Kompas3DException("У спецификации нет описания с базовыми объектами");
		K7::ISpecificationBaseObjectPtr base = objects->Add(row.section, 0.0);
		K7::ISpecificationObjectPtr object = base;
		if (!object) return false;
		if (!row.attach.empty()) {
			K7::IAttachedDocumentsPtr attached = object->AttachedDocuments;
			if (attached) attached->Add(Utf8ToBstr(row.attach), VARIANT_TRUE);
		}
		// Сначала подключённый документ передаёт свои данные, затем заданные поля — поверх
		object->Update();
		SetColumn(object, KConst::ksSColumnFormat, row.format);
		SetColumn(object, KConst::ksSColumnZone, row.zone);
		SetColumn(object, KConst::ksSColumnPosition, row.position);
		SetColumn(object, KConst::ksSColumnMark, row.designation);
		SetColumn(object, KConst::ksSColumnName, row.name);
		SetColumn(object, KConst::ksSColumnCount, row.count);
		SetColumn(object, KConst::ksSColumnNote, row.note);
		return object->Update() == VARIANT_TRUE || !ColumnText(object, KConst::ksSColumnName).empty();
	}

	std::vector<Specification::Row> Rows() override {
		std::vector<Specification::Row> out;
		K7::ISpecificationDescriptionPtr description = Description();
		K7::ISpecificationBaseObjectsPtr objects = description ? description->BaseObjects : nullptr;
		const long count = objects ? objects->Count : 0;
		for (long i = 0; i < count; ++i) {
			K7::ISpecificationObjectPtr object = objects->GetItem(_variant_t(i));
			if (!object) continue;
			Specification::Row row;
			row.section = object->Section;
			row.format = ColumnText(object, KConst::ksSColumnFormat);
			row.zone = ColumnText(object, KConst::ksSColumnZone);
			row.position = ColumnText(object, KConst::ksSColumnPosition);
			row.designation = ColumnText(object, KConst::ksSColumnMark);
			row.name = ColumnText(object, KConst::ksSColumnName);
			row.count = ColumnText(object, KConst::ksSColumnCount);
			row.note = ColumnText(object, KConst::ksSColumnNote);
			out.push_back(row);
		}
		return out;
	}

	int SetStamp(const std::map<int, std::string>& cells) override {
		K7::ILayoutSheetsPtr sheets = doc7->LayoutSheets;
		K7::ILayoutSheetPtr sheet = sheets && sheets->Count > 0 ? sheets->GetItem(_variant_t(0L)) : nullptr;
		K7::IStampPtr stamp = sheet ? sheet->Stamp : nullptr;
		if (!stamp) return 0;
		for (const auto& [id, value] : cells) {
			if (K7::ITextPtr text = stamp->GetText(id)) text->Str = Utf8ToBstr(value);
		}
		stamp->Update();
		int written = 0;
		for (const auto& [id, value] : cells) {
			K7::ITextPtr text = stamp->GetText(id);
			std::string got = text ? BstrToUtf8(text->Str) : std::string();
			for (size_t p; (p = got.find("$|")) != std::string::npos;) got.erase(p, 2);
			written += got == value;
		}
		return written;
	}

	// Результат SaveAs ненадёжен (FALSE и при записанном файле) — проверяем, что файл есть
	bool SaveAs(const std::string& path) override {
		doc7->SaveAs(Utf8ToBstr(path));
		std::error_code ec;
		return std::filesystem::exists(std::filesystem::path((const wchar_t*)Utf8ToBstr(path)), ec);
	}

	bool ExportPdf(const std::string& path) override {
		const std::string source = GetPath();
		if (source.empty()) return false;
		std::filesystem::path sys((const wchar_t*)Utf8ToBstr(Kompas3D::SystemPath(0)));
		std::filesystem::path dll = sys.parent_path() / "Bin" / "Pdf2d.dll";
		K7::IConverterPtr converter = ComEvent::kompas7->GetConverter(_variant_t(dll.wstring().c_str()));
		if (!converter) throw Kompas3DException("Не найден конвертер PDF: " + BstrToUtf8(_bstr_t(dll.c_str())));
		converter->Convert(Utf8ToBstr(source), Utf8ToBstr(path), 0, VARIANT_FALSE);
		std::error_code ec;
		return std::filesystem::exists(std::filesystem::path((const wchar_t*)Utf8ToBstr(path)), ec);
	}

	std::string GetPath() override {
		return BstrToUtf8(doc7->PathName);
	}

	void Close() override {
		doc7->Close(KConst::kdDoNotSaveChanges);
	}
};
