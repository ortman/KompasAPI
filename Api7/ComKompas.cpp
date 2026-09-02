#ifdef KAPI7

#include "ComKompas.h"

#import "ksConstants.tlb" rename_namespace("KConst") implementation_only no_registry
#import "ksConstants3D.tlb" rename_namespace("KConst3D") implementation_only no_registry
#import "kAPI5.tlb" named_guids rename_namespace("K5") implementation_only no_registry
#import "kAPI7.tlb" named_guids rename_namespace("K7") implementation_only no_registry

std::string GetClassName(IUnknown* pUnk) {
    if (!pUnk) return "Null pointer";

    // Запрашиваем IDispatch, который есть у 99% объектов Kompas API
    IDispatch* pDispatch = nullptr;
    if (SUCCEEDED(pUnk->QueryInterface(IID_IDispatch, (void**)&pDispatch))) {
        ITypeInfo* pTypeInfo = nullptr;
        // Запрашиваем информацию о типе напрямую у диспетчера
        if (SUCCEEDED(pDispatch->GetTypeInfo(0, LOCALE_USER_DEFAULT, &pTypeInfo))) {
            BSTR bstrName = nullptr;
            if (SUCCEEDED(pTypeInfo->GetDocumentation(MEMBERID_NIL, &bstrName, nullptr, nullptr, nullptr))) {
                std::string className = (const char*)_bstr_t(bstrName);
                SysFreeString(bstrName);
                pTypeInfo->Release();
                pDispatch->Release();
                return className; // Вернет реальное имя интерфейса, например "IPart3D" или "IDocument"
            }
            pTypeInfo->Release();
        }
        pDispatch->Release();
    }
    return "Unknown Kompas Object";
}

#endif

#include "../Api7/Kompas3D.hpp"
#include "../Ksapi/Kompas3D.hpp"

#include "../Include/Kompas3D.h"

std::string Kompas3D::Cp1251ToUtf8(const char* cp1251Str) {
    if (!cp1251Str || strlen(cp1251Str) == 0) return "";

    // 1. Из CP1251 в UTF-16
    int wsize = MultiByteToWideChar(1251, 0, cp1251Str, -1, NULL, 0);
    std::wstring wstr(wsize, 0);
    // Использовали wstr.data() вместо &wstr
    MultiByteToWideChar(1251, 0, cp1251Str, -1, wstr.data(), wsize);

    // 2. Из UTF-16 в UTF-8
    int csize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string utf8Str(csize, 0);
    // Использовали utf8Str.data() вместо &utf8Str
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, utf8Str.data(), csize, NULL, NULL);

    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }
    return utf8Str;
}

std::string Kompas3D::Utf8ToCp1251(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";
    
    // 1. Из UTF-8 в UTF-16
    int wsize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    std::wstring wstr(wsize, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wstr[0], wsize);
    
    // 2. Из UTF-16 в CP1251
    int csize = WideCharToMultiByte(1251, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string cp1251Str(csize, 0);
    WideCharToMultiByte(1251, 0, wstr.c_str(), -1, &cp1251Str[0], csize, NULL, NULL);
    
    // Убираем лишний нуль-терминатор из размера std::string
    if (!cp1251Str.empty() && cp1251Str.back() == '\0') {
        cp1251Str.pop_back();
    }
    return cp1251Str;
}