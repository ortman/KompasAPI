#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

// Документ спецификации КОМПАС (.spw): строки по разделам, подключённые документы, основная надпись
class Specification {
public:
	// Разделы стандартного стиля спецификации КОМПАС (ГОСТ 2.106)
	enum Section { Documentation = 5, Complexes = 10, Assemblies = 15, Parts = 20, Standard = 25, Other = 30, Materials = 35 };

	struct Row {
		int section = Parts;
		std::string attach;       // подключённый документ: модель или чертёж — из него наименование, обозначение, формат
		std::string format, zone, position, designation, name, count, note;   // непустые — записать поверх
	};

	class SpecificationImpl {
	public:
		virtual bool AddRow(const Row& row) { return false; }
		virtual std::vector<Row> Rows() { return {}; }
		virtual int SetStamp(const std::map<int, std::string>& cells) { return 0; }
		virtual bool SaveAs(const std::string& path) { return false; }
		virtual bool ExportPdf(const std::string& path) { return false; }
		virtual std::string GetPath() { return std::string(); }
		virtual void Close() {}
		virtual ~SpecificationImpl() = default;
	};

	Specification() = default;
	Specification(std::unique_ptr<SpecificationImpl> p) : impl(std::move(p)) {}
	explicit operator bool() const { return impl != nullptr; }

	// Строка в разделе; после подключения документа КОМПАС заполняет колонки, заданные поля — поверх
	bool AddRow(const Row& row) { return impl->AddRow(row); }
	// Строки документа (колонки — как видны в спецификации)
	std::vector<Row> Rows() { return impl->Rows(); }
	// Основная надпись: 1 — наименование, 2 — обозначение…; число записанных граф
	int SetStamp(const std::map<int, std::string>& cells) { return impl->SetStamp(cells); }
	bool SaveAs(const std::string& path) { return impl->SaveAs(path); }
	// PDF из сохранённого .spw
	bool ExportPdf(const std::string& path) { return impl->ExportPdf(path); }
	std::string GetPath() { return impl->GetPath(); }
	void Close() { impl->Close(); }

private:
	std::unique_ptr<SpecificationImpl> impl;
};
