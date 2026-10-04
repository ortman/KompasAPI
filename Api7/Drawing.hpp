#pragma once

#include "../Include/Drawing.h"
#include "../Include/Kompas3D.h"
#include "Node.hpp"
#include "Annotation2D.hpp"

#include <filesystem>

class DrawingApi7 : public Drawing::DrawingImpl {
	K7::IKompasDocument2DPtr doc7;

	K5::ksDocument2DPtr Doc5() {
		K5::ksDocument2DPtr d = ToApi5<K5::ksDocument2DPtr>(doc7);
		if (!d) throw Kompas3DException("Не могу получить ksDocument2D чертежа");
		return d;
	}

	K7::ISheetFormatPtr Format() {
		K7::ILayoutSheetsPtr sheets = doc7->LayoutSheets;
		K7::ILayoutSheetPtr sheet = sheets && sheets->Count > 0 ? sheets->GetItem(_variant_t(0L)) : nullptr;
		if (!sheet) throw Kompas3DException("В чертеже нет листа");
		return sheet->Format;
	}

	K7::ILayoutSheetPtr Sheet0() {
		K7::ILayoutSheetsPtr sheets = doc7->LayoutSheets;
		K7::ILayoutSheetPtr sheet = sheets && sheets->Count > 0 ? sheets->GetItem(_variant_t(0L)) : nullptr;
		if (!sheet) throw Kompas3DException("В чертеже нет листа");
		return sheet;
	}

	K7::IDrawingDocumentPtr DrawingDoc() {
		K7::IDrawingDocumentPtr d = doc7;
		if (!d) throw Kompas3DException("Документ не чертёж");
		return d;
	}

	// Строки текста; пустые строки текста КОМПАСа пропускаются
	static std::vector<std::string> Lines(K7::ITextPtr text) {
		std::vector<std::string> out;
		if (!text) return out;
		for (long i = 0; i < text->Count; ++i) {
			std::string s = LineText(text->GetTextLine(i));
			if (!s.empty()) out.push_back(s);
		}
		return out;
	}

	// Текст строки КОМПАСа: спецзнаки приходят как «@N~» — обратно в символы
	static std::string LineText(K7::ITextLinePtr line) {
		std::string s = line ? BstrToUtf8(line->Str) : std::string();
		static const std::pair<const char*, const char*> kCodes[] = {{"@1~", "°"}, {"@2~", "Ø"}, {"@3~", "±"}, {"@4~", "×"}};
		for (const auto& [code, text] : kCodes) {
			for (size_t p; (p = s.find(code)) != std::string::npos;) s.replace(p, 3, text);
		}
		return s;
	}

	// Строка текста. Шрифт ГОСТ берёт символы как CP1251, поэтому «Ø» и «×» (их там нет)
	// пишутся спецзнаками КОМПАСа: 2 — диаметр, 4 — знак умножения (1 — «°», 3 — «±»)
	static void WriteLine(K7::ITextLinePtr line, const std::string& s) {
		static const std::pair<std::string, long> kSymbols[] = {{"Ø", 2}, {"×", 4}};
		std::string part;
		auto flush = [&]() {
			if (part.empty()) return;
			K7::ITextItemPtr item = line->Add();
			item->ItemType = KConst::ksTItString;
			item->Str = Utf8ToBstr(part);
			item->Update();
			part.clear();
		};
		for (size_t pos = 0; pos < s.size();) {
			bool symbol = false;
			for (const auto& [text, number] : kSymbols) {
				if (s.compare(pos, text.size(), text) != 0) continue;
				flush();
				K7::ITextItemPtr item = line->Add();
				item->ItemType = KConst::ksTItSpecialSymbol;
				item->Number = number;
				item->Update();
				pos += text.size();
				symbol = true;
				break;
			}
			if (!symbol) part += s[pos++];
		}
		flush();
	}

	K7::IViewsPtr Views() {
		K7::IViewsAndLayersManagerPtr manager = doc7->ViewsAndLayersManager;
		K7::IViewsPtr views = manager ? manager->Views : nullptr;
		if (!views) throw Kompas3DException("Не могу получить виды чертежа");
		return views;
	}

	K7::IViewPtr FindView(long number) {
		K7::IViewPtr view = Views()->GetViewByNumber(number);
		if (!view) throw Kompas3DException("В чертеже нет вида с номером " + std::to_string(number));
		return view;
	}

	Drawing::View ReadView(K7::IViewPtr view) {
		Drawing::View v;
		v.number = view->Number;
		v.name = BstrToUtf8(view->Name);
		v.x = view->X;
		v.y = view->Y;
		v.scale = view->Scale;
		if (K7::IAssociationViewPtr assoc = view) {
			v.associative = true;
			v.source = BstrToUtf8(assoc->SourceFileName);
			v.projection = BstrToUtf8(assoc->ProjectionName);
			_variant_t matrix = assoc->ProjectionMatrix;
			if (matrix.vt == (VT_ARRAY | VT_R8) && matrix.parray) {
				double* data = nullptr;
				LONG lo = 0, hi = -1;
				SafeArrayGetLBound(matrix.parray, 1, &lo);
				SafeArrayGetUBound(matrix.parray, 1, &hi);
				// 4×4 построчно: строки 0..2 — оси листа и направление на наблюдателя
				if (hi - lo + 1 >= 12 && SUCCEEDED(SafeArrayAccessData(matrix.parray, (void**)&data))) {
					for (int r = 0; r < 3; ++r)
						for (int c = 0; c < 3; ++c) v.axes[r * 3 + c] = data[r * 4 + c];
					SafeArrayUnaccessData(matrix.parray);
					v.hasAxes = true;
				}
			}
		}
		K5::ksRectParamPtr rect = ComEvent::kompas5->GetParamStruct(KConst::ko_RectParam);
		if (rect && Doc5()->ksGetObjGabaritRect(view->Reference, rect)) {
			K5::ksMathPointParamPtr bottom = rect->GetpBot(), top = rect->GetpTop();
			if (bottom && top && top->x > bottom->x) {
				v.box = {bottom->x, bottom->y, top->x, top->y};
				v.hasBox = true;
			}
		}
		return v;
	}

public:
	DrawingApi7(K7::IKompasDocument2DPtr d) : doc7(d) {
		if (!doc7) throw Kompas3DException("Потерян указатель на чертёж");
	}

	std::string GetPath() override { return BstrToUtf8(doc7->PathName); }
	std::string GetName() override { return BstrToUtf8(doc7->Name); }
	bool IsModified() override { return doc7->Changed == VARIANT_TRUE; }

	Drawing::Sheet GetSheet() override {
		K7::ISheetFormatPtr f = Format();
		Drawing::Sheet s;
		s.format = (int)f->Format;
		s.width = f->FormatWidth;
		s.height = f->FormatHeight;
		// Флаг VerticalOrientation у A4 не совпадает с фактической ориентацией — судим по размерам
		s.vertical = s.height > s.width;
		return s;
	}

	bool SetSheet(int format, bool vertical) override {
		K7::ILayoutSheetsPtr sheets = doc7->LayoutSheets;
		K7::ILayoutSheetPtr sheet = sheets ? sheets->GetItem(_variant_t(0L)) : nullptr;
		if (!sheet) return false;
		K7::ISheetFormatPtr f = sheet->Format;
		f->Format = (KConst::ksDocumentFormatEnum)format;
		f->VerticalOrientation = vertical ? VARIANT_TRUE : VARIANT_FALSE;
		sheet->Update();   // без изменений возвращает FALSE — проверяем результат по самому листу
		Drawing::Sheet now = GetSheet();
		return now.format == format && now.vertical == vertical;
	}

	std::vector<Drawing::View> GetViews() override {
		std::vector<Drawing::View> out;
		K7::IViewsPtr views = Views();
		for (long i = 0; i < views->Count; ++i) {
			K7::IViewPtr view = views->GetView(_variant_t(i));
			if (view && view->Number != 0) out.push_back(ReadView(view));
		}
		return out;
	}

	Drawing::View AddView(const Drawing::ViewParams& p) override {
		K7::IViewPtr view = Views()->Add(KConst::vt_Arbitrary);
		K7::IAssociationViewPtr assoc = view;
		if (!assoc) throw Kompas3DException("КОМПАС не создал ассоциативный вид");
		assoc->SourceFileName = Utf8ToBstr(p.source);
		assoc->ProjectionName = Utf8ToBstr(p.projection);
		view->X = p.x;
		view->Y = p.y;
		view->Scale = p.scale;
		if (!p.name.empty()) view->Name = Utf8ToBstr(p.name);
		if (K7::IViewDesignationPtr label = view) {
			label->ShowName = p.showLabel ? VARIANT_TRUE : VARIANT_FALSE;
			label->ShowScale = p.showLabel ? VARIANT_TRUE : VARIANT_FALSE;
		}
		if (!view->Update()) {
			view->Delete();
			throw Kompas3DException("КОМПАС не построил вид '" + p.projection + "' модели " + p.source);
		}
		return ReadView(view);
	}

	Drawing::View MoveView(long number, double x, double y, double scale) override {
		K7::IViewPtr view = FindView(number);
		view->X = x;
		view->Y = y;
		view->Scale = scale;
		view->Update();
		return ReadView(view);
	}

	std::vector<std::string> ComponentsAt(long number, const std::vector<std::array<double, 2>>& points, Doc3D& model3d) override;

	long AddPositionLeader(long number, double x, double y, double shelfX, double shelfY, const std::string& text, bool shelfRight) override {
		K7::ISymbols2DContainerPtr symbols = FindView(number);
		K7::ILeadersPtr leaders = symbols ? symbols->Leaders : nullptr;
		if (!leaders) return 0;
		K7::IBaseLeaderPtr base = leaders->Add(KConst::ksDrPosLeader);
		K7::IPositionLeaderPtr position = base;
		K7::IBranchsPtr branchs = base;
		if (!base || !position || !branchs) return 0;
		branchs->X0 = shelfX;
		branchs->Y0 = shelfY;
		branchs->AddBranchByPoint(-1, x, y);
		base->ArrowType = KConst::ksLeaderPoint;
		position->ShelfDirection = shelfRight ? KConst::ksLSRight : KConst::ksLSLeft;
		if (K7::ITextPtr positions = position->Positions) positions->Str = Utf8ToBstr(text);
		K7::IDrawingObjectPtr object = base;
		if (!object->Update()) {
			object->Delete();
			return 0;
		}
		return object->Reference;
	}

	int DeletePositionLeaders(long number) override {
		K7::ISymbols2DContainerPtr symbols = FindView(number);
		K7::ILeadersPtr leaders = symbols ? symbols->Leaders : nullptr;
		if (!leaders) return 0;
		std::vector<K7::IDrawingObjectPtr> found;
		for (long i = 0; i < leaders->Count; ++i) {
			K7::IBaseLeaderPtr leader = leaders->GetLeader(_variant_t(i));
			if (K7::IPositionLeaderPtr position = leader) found.push_back(leader);
		}
		for (K7::IDrawingObjectPtr& o : found) o->Delete();
		return (int)found.size();
	}

	std::vector<std::vector<double>> GetCutLines(long number) override {
		std::vector<std::vector<double>> out;
		K7::ISymbols2DContainerPtr symbols = FindView(number);
		K7::ICutLinesPtr lines = symbols ? symbols->CutLines : nullptr;
		const long count = lines ? lines->Count : 0;
		for (long i = 0; i < count; ++i) {
			K7::ICutLinePtr cut = lines->GetCutLine(_variant_t(i));
			if (!cut) continue;
			_variant_t points = cut->Points;
			if (points.vt != (VT_ARRAY | VT_R8) || !points.parray) continue;
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(points.parray, 1, &lo);
			SafeArrayGetUBound(points.parray, 1, &hi);
			std::vector<double> v;
			for (LONG k = lo; k <= hi; ++k) {
				double d = 0;
				SafeArrayGetElement(points.parray, &k, &d);
				v.push_back(d);
			}
			if (v.size() >= 4) out.push_back(std::move(v));
		}
		return out;
	}

	// Вид разреза строится КОМПАСом в осях базового вида, «верх» зависит от направления взгляда
	Drawing::View AddSection(Drawing::SectionParams& p) override {
		K7::IViewPtr base = FindView(p.baseView);
		K7::ISymbols2DContainerPtr symbols = base;
		K7::ICutLinesPtr lines = symbols ? symbols->CutLines : nullptr;
		if (!lines) throw Kompas3DException("Не могу получить линии разреза вида");
		auto build = [&](bool arrowsLeft, K7::IDrawingObjectPtr& cutObject) -> K7::IViewPtr {
			K7::ICutLinePtr cut = lines->Add();
			if (!cut) throw Kompas3DException("КОМПАС не создал линию разреза");
			SAFEARRAY* array = SafeArrayCreateVector(VT_R8, 0, (ULONG)p.points.size());
			for (LONG i = 0; i < (LONG)p.points.size(); ++i) {
				double d = p.points[i];
				SafeArrayPutElement(array, &i, &d);
			}
			_variant_t points;
			points.vt = VT_ARRAY | VT_R8;
			points.parray = array;
			cut->Points = points;
			// ArrowPos по справке «TRUE — слева по направлению», фактически (v23) — наоборот
			cut->ArrowPos = arrowsLeft ? VARIANT_FALSE : VARIANT_TRUE;
			cutObject = cut;
			if (!cutObject->Update()) {
				cutObject->Delete();
				throw Kompas3DException("КОМПАС не построил линию разреза");
			}
			K7::IViewPtr view = Views()->Add(KConst::vt_Section);
			K7::IAssociationViewPtr assoc = view;
			if (!assoc) {
				cutObject->Delete();
				throw Kompas3DException("КОМПАС не создал вид разреза");
			}
			assoc->BaseObject = cutObject;
			assoc->Section = p.section ? VARIANT_TRUE : VARIANT_FALSE;
			assoc->SameHatch = VARIANT_FALSE;   // сборка: у каждой детали своя штриховка (ГОСТ 2.306)
			assoc->ProjectionLink = p.projectionLink ? VARIANT_TRUE : VARIANT_FALSE;
			view->X = p.x;
			view->Y = p.y;
			view->Scale = base->Scale;
			// «А-А» без «(1:1)»: масштаб тот же, что у базового вида (ГОСТ 2.305)
			if (K7::IViewDesignationPtr label = view) label->ShowScale = VARIANT_FALSE;
			if (!view->Update()) {
				view->Delete();
				cutObject->Delete();
				throw Kompas3DException("КОМПАС не построил вид разреза");
			}
			return view;
		};
		K7::IDrawingObjectPtr cutObject;
		K7::IViewPtr view = build(p.arrowsLeft, cutObject);
		p.flipped = false;
		if (p.up) {
			Drawing::View v = ReadView(view);
			const auto& u = *p.up;
			if (v.hasAxes && v.axes[3] * u[0] + v.axes[4] * u[1] + v.axes[5] * u[2] < -0.5) {
				view->Delete();
				cutObject->Delete();
				view = build(!p.arrowsLeft, cutObject);
				p.flipped = true;
			}
		}
		return ReadView(view);
	}

	// Delete возвращает FALSE и при успешном удалении — проверяем, что вида больше нет
	bool DeleteView(long number) override {
		FindView(number)->Delete();
		return Views()->GetViewByNumber(number) == nullptr;
	}

	// Ассоциативный вид перечитывает модель, если заново задать ему файл модели.
	// ksRebuildDocument виды не обновляет; IAssociationView::Rebuild в КОМПАСе v23 через
	// IDispatch недоступен, а прямой вызов при SDK v24 уходит мимо vtable
	bool Update() override {
		bool ok = true;
		K7::IViewsPtr views = Views();
		for (long i = 0; i < views->Count; ++i) {
			K7::IViewPtr view = views->GetView(_variant_t(i));
			K7::IAssociationViewPtr assoc = view;
			if (!assoc) continue;
			_bstr_t source = assoc->SourceFileName;
			assoc->SourceFileName = source;
			view->Update();   // возвращает FALSE и при успехе
		}
		Doc5()->ksRebuildDocument();
		return ok;
	}

	std::vector<SketchItem> GetViewItems(long number) override {
		return Annotation2DApi7(Doc5(), FindView(number)).Items();
	}

	DimensionInfo AddDimension(long number, const Dimension2D& dimension) override {
		Dimension2D d = dimension;
		d.driving = false;   // размеры чертежа не управляют моделью
		return Annotation2DApi7(Doc5(), FindView(number)).Dimension(d);
	}

	bool DeleteViewObject(long number, long ref) override {
		K7::IDrawingObjectPtr object = Annotation2DApi7(Doc5(), FindView(number)).Object(ref);
		if (!object) return false;
		object->Delete();   // возвращает FALSE и при удалённом объекте
		return true;
	}

	std::vector<DimensionRead> GetDimensions(long number) override {
		return Annotation2DApi7(Doc5(), FindView(number)).Dimensions();
	}

	bool SetTolerance(long number, long ref, const DimensionTolerance& t) override {
		return Annotation2DApi7(Doc5(), FindView(number)).Tolerance(ref, t);
	}

	long AddRough(long number, const Rough2D& rough) override {
		return Annotation2DApi7(Doc5(), FindView(number)).Rough(rough);
	}

	std::map<int, std::string> GetStamp(const std::vector<int>& cells) override {
		std::map<int, std::string> out;
		K7::IStampPtr stamp = Sheet0()->Stamp;
		if (!stamp) return out;
		for (int id : cells) {
			K7::ITextPtr text = stamp->GetText(id);
			if (!text) continue;
			std::string s;
			for (const std::string& line : Lines(text)) s += (s.empty() ? "" : "\n") + line;
			if (!s.empty()) out[id] = s;
		}
		return out;
	}

	int SetStamp(const std::map<int, std::string>& cells) override {
		K7::IStampPtr stamp = Sheet0()->Stamp;
		if (!stamp) throw Kompas3DException("У листа нет основной надписи");
		for (const auto& [id, value] : cells) {
			K7::ITextPtr text = stamp->GetText(id);
			if (!text) continue;
			// Несколько строк («Узел вала» / «Сборочный чертёж») — отдельными строками текста
			std::vector<std::string> lines;
			for (size_t start = 0;;) {
				const size_t end = value.find('\n', start);
				lines.push_back(value.substr(start, end == std::string::npos ? std::string::npos : end - start));
				if (end == std::string::npos) break;
				start = end + 1;
			}
			text->Str = Utf8ToBstr(lines[0]);
			for (size_t i = 1; i < lines.size(); ++i) {
				if (K7::ITextLinePtr line = text->Add()) line->Str = Utf8ToBstr(lines[i]);
			}
		}
		stamp->Update();
		int written = 0;
		std::vector<int> ids;
		for (const auto& [id, value] : cells) ids.push_back(id);
		std::map<int, std::string> now = GetStamp(ids);
		for (const auto& [id, value] : cells) {
			auto it = now.find(id);
			std::string got = it == now.end() ? std::string() : it->second;
			for (size_t p; (p = got.find("$|")) != std::string::npos;) got.erase(p, 2);   // части обозначения
			written += got == value;
		}
		return written;
	}

	std::vector<std::string> GetTechnicalRequirements() override {
		K7::ITechnicalDemandPtr demand = DrawingDoc()->TechnicalDemand;
		if (!demand || !demand->IsCreated) return {};
		// Пункт — строка с номером; перенесённые КОМПАСом продолжения приклеиваются к нему
		std::vector<std::string> items;
		K7::ITextPtr text = demand->Text;
		for (long i = 0; text && i < text->Count; ++i) {
			K7::ITextLinePtr line = text->GetTextLine(i);
			if (!line) continue;
			std::string s = LineText(line);
			const auto numbering = line->Numbering;
			const bool numbered = numbering == KConst::ksTNumbNumber || numbering == KConst::ksTNumbNewNumber;
			if (numbered || items.empty()) {
				if (!s.empty() || numbered) items.push_back(s);
			} else if (!s.empty()) {
				items.back() += (items.back().empty() ? "" : " ") + s;
			}
		}
		return items;
	}

	bool SetTechnicalRequirements(const std::vector<std::string>& lines) override {
		K7::ITechnicalDemandPtr demand = DrawingDoc()->TechnicalDemand;
		if (!demand) throw Kompas3DException("Не могу получить технические требования");
		if (lines.empty()) {
			if (demand->IsCreated) demand->Delete();
			return !demand->IsCreated;
		}
		K7::ITextPtr text = demand->Text;
		if (!text) return false;
		text->Clear();
		for (const std::string& s : lines) {
			K7::ITextLinePtr line = text->Add();
			if (!line) return false;
			WriteLine(line, s);
			line->Numbering = KConst::ksTNumbNumber;
		}
		demand->AutoPlacement = VARIANT_TRUE;
		demand->Update();
		return GetTechnicalRequirements().size() == lines.size();
	}

	std::vector<std::array<double, 4>> GetTechnicalRequirementsBlocks() override {
		std::vector<std::array<double, 4>> out;
		K7::ITechnicalDemandPtr demand = DrawingDoc()->TechnicalDemand;
		if (!demand || !demand->IsCreated) return out;
		_variant_t v = demand->BlocksGabarits;
		if ((v.vt & VT_ARRAY) && v.parray) {
			LONG lo = 0, hi = -1;
			SafeArrayGetLBound(v.parray, 1, &lo);
			SafeArrayGetUBound(v.parray, 1, &hi);
			std::vector<double> values;
			for (LONG i = lo; i <= hi; ++i) {
				double d = 0;
				SafeArrayGetElement(v.parray, &i, &d);
				values.push_back(d);
			}
			for (size_t i = 0; i + 3 < values.size(); i += 4) {
				out.push_back({(std::min)(values[i], values[i + 2]), (std::min)(values[i + 1], values[i + 3]),
				               (std::max)(values[i], values[i + 2]), (std::max)(values[i + 1], values[i + 3])});
			}
		}
		return out;
	}

	bool SetTechnicalRequirementsBlocks(const std::vector<std::array<double, 4>>& blocks) override {
		K7::ITechnicalDemandPtr demand = DrawingDoc()->TechnicalDemand;
		if (!demand || !demand->IsCreated || blocks.empty()) return false;
		SAFEARRAY* array = SafeArrayCreateVector(VT_R8, 0, (ULONG)(blocks.size() * 4));
		for (size_t b = 0; b < blocks.size(); ++b) {
			for (LONG k = 0; k < 4; ++k) {
				LONG index = (LONG)(b * 4) + k;
				double d = blocks[b][k];
				SafeArrayPutElement(array, &index, &d);
			}
		}
		_variant_t v;
		v.vt = VT_ARRAY | VT_R8;
		v.parray = array;
		demand->AutoPlacement = VARIANT_FALSE;
		demand->BlocksGabarits = v;
		demand->Update();
		std::vector<std::array<double, 4>> now = GetTechnicalRequirementsBlocks();
		return !now.empty() && std::abs(now[0][0] - blocks[0][0]) < 0.5 && std::abs(now[0][3] - blocks[0][3]) < 0.5;
	}

	std::optional<Drawing::UnspecifiedRough> GetUnspecifiedRough() override {
		K7::ISpecRoughPtr rough = DrawingDoc()->SpecRough;
		if (!rough || !rough->IsCreated) return std::nullopt;
		Drawing::UnspecifiedRough r;
		r.text = BstrToUtf8(rough->Text);
		r.sign = (int)rough->SignType;
		r.addSign = rough->AddSign == VARIANT_TRUE;
		return r;
	}

	bool SetUnspecifiedRough(const std::optional<Drawing::UnspecifiedRough>& r) override {
		K7::ISpecRoughPtr rough = DrawingDoc()->SpecRough;
		if (!rough) throw Kompas3DException("Не могу получить неуказанную шероховатость");
		if (!r) {
			if (rough->IsCreated) rough->Delete();
			return !rough->IsCreated;
		}
		rough->Text = Utf8ToBstr(r->text);
		rough->SignType = (KConst::ksRoughSignEnum)r->sign;
		rough->AddSign = r->addSign ? VARIANT_TRUE : VARIANT_FALSE;
		rough->AutoPlacement = VARIANT_TRUE;
		rough->Update();
		auto now = GetUnspecifiedRough();
		return now && now->text == r->text;
	}

	bool Save() override {
		if (GetPath().empty()) return false;
		return SUCCEEDED(doc7->Save());
	}

	bool SaveAs(const std::string& path) override { return SUCCEEDED(doc7->SaveAs(Utf8ToBstr(path))); }

	bool SaveImage(const std::string& path, int dpi) override {
		K5::ksDocument2DPtr doc = Doc5();
		K5::ksRasterFormatParamPtr raster = doc->RasterFormatParam();
		if (!raster) throw Kompas3DException("Не могу получить параметры растрового формата");
		raster->Init();
		raster->format = 3;          // FORMAT_PNG
		raster->colorBPP = 24;       // BPP_COLOR_24
		raster->colorType = 3;       // COLOROBJECT
		raster->greyScale = false;
		raster->onlyThinLine = false;
		raster->extResolution = dpi;
		raster->extScale = 1.0;
		return doc->SaveAsToRasterFormat(Kompas3D::Utf8ToCp1251(path).c_str(), raster) == VARIANT_TRUE;
	}

	bool ExportPdf(const std::string& path) override {
		const std::string source = GetPath();
		if (source.empty()) return false;
		// Конвертер лежит в Bin рядом с Sys (ksSystemPath(sptSYSTEM_FILES) = ...\Sys)
		std::filesystem::path sys((const wchar_t*)Utf8ToBstr(Kompas3D::SystemPath(0)));
		std::filesystem::path dll = sys.parent_path() / "Bin" / "Pdf2d.dll";
		K7::IConverterPtr converter = ComEvent::kompas7->GetConverter(_variant_t(dll.wstring().c_str()));
		if (!converter) throw Kompas3DException("Не найден конвертер PDF: " + BstrToUtf8(_bstr_t(dll.c_str())));
		return converter->Convert(Utf8ToBstr(source), Utf8ToBstr(path), 0, VARIANT_FALSE) != 0;
	}

	void Activate() override { doc7->Active = VARIANT_TRUE; }

	void Close() override {
		if (doc7) {
			doc7->Close(KConst::kdDoNotSaveChanges);
			doc7 = nullptr;
		}
	}
};
