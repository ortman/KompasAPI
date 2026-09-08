##
##  KompasAPI — обёртка над API КОМПАС-3D.
##
##  Сборка без U++: библиотека не использует ни одного класса U++, только стандартный
##  C++ и COM-интерфейсы КОМПАСа.
##
##  Использование:
##      add_subdirectory(KompasAPI)
##      target_link_libraries(МоеПриложение PRIVATE KompasAPI::KompasAPI)
##  далее в коде:
##      #include <Kompas3D.h>
##
##  Путь к SDK задаётся переменной KOMPAS_SDK_DIR:
##      cmake -B build -DKOMPAS_SDK_DIR=C:/KSDK24
##
##  ВАЖНО: Kompas3D::RunCommand объявлен в Include/Kompas3D.h, но не реализован —
##  это диспетчер команд меню, его определяет приложение:
##      void Kompas3D::RunCommand(uint32_t comm) { ... }
##

cmake_minimum_required(VERSION 3.16)

project(KompasAPI
        VERSION     1.0.0
        DESCRIPTION "Обёртка над API КОМПАС-3D"
        LANGUAGES   CXX)

set(KOMPAS_SDK_DIR "C:/KSDK24" CACHE PATH
    "Каталог установленного SDK КОМПАС-3D")

if(WIN32)
    set(_kompasapi_default_backend "Api7")
else()
    set(_kompasapi_default_backend "None")
endif()

set(KOMPASAPI_BACKEND "${_kompasapi_default_backend}" CACHE STRING
    "Реализация: Api7 — КОМПАС через COM; None — только заголовки Include/ (свой бэкенд, тесты на подставных реализациях)")
set_property(CACHE KOMPASAPI_BACKEND PROPERTY STRINGS "Api7" "None")

# ---------------------------------------------------------------- исходники --
# Заголовки перечисляются только чтобы их показывали IDE: код в Include/ и Api7/
# целиком header-only, компилируется единственная единица трансляции ComKompas.cpp.
file(GLOB_RECURSE KOMPASAPI_PUBLIC_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h")
file(GLOB_RECURSE KOMPASAPI_API7_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Api7/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/Api7/*.hpp")

# ------------------------------------------------------------------- цель ----
if(KOMPASAPI_BACKEND STREQUAL "None")

    # Только публичный API. Собрать реализацию Kompas3DImpl / Doc3DImpl / PartImpl
    # и передать её через Kompas3D::SetKompas() — задача пользователя.
    add_library(KompasAPI INTERFACE)
    target_compile_features(KompasAPI INTERFACE cxx_std_20)
    target_include_directories(KompasAPI INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:include>)

elseif(KOMPASAPI_BACKEND STREQUAL "Api7")

    if(NOT WIN32)
        message(FATAL_ERROR
            "KOMPASAPI_BACKEND=Api7 работает только под Windows. "
            "Для остальных платформ соберите с -DKOMPASAPI_BACKEND=None "
            "(реализация Ksapi/ пока не написана).")
    endif()
    if(NOT MSVC AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Api7/tlh")
        message(FATAL_ERROR
            "Нужен MSVC: Api7/ComKompas.cpp импортирует интерфейсы КОМПАСа через #import \"*.tlb\", "
            "а это расширение MSVC. Для других компиляторов положите заранее сгенерированные "
            "*.tlh/*.tli в ${CMAKE_CURRENT_SOURCE_DIR}/Api7/tlh — их подхватит Api7/ComKompas.h.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "Поддерживается только x64: библиотеки SDK лежат в ${KOMPAS_SDK_DIR}/lib64.")
    endif()
    if(NOT IS_DIRECTORY "${KOMPAS_SDK_DIR}/Include")
        message(FATAL_ERROR
            "SDK КОМПАС-3D не найден в '${KOMPAS_SDK_DIR}'. "
            "Укажите путь: cmake -B build -DKOMPAS_SDK_DIR=<каталог SDK>")
    endif()

    add_library(KompasAPI STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/Api7/ComKompas.cpp"
        ${KOMPASAPI_PUBLIC_HEADERS}
        ${KOMPASAPI_API7_HEADERS})

    # concepts в Include/Process3D.h — нужен C++20
    target_compile_features(KompasAPI PUBLIC cxx_std_20)

    target_include_directories(KompasAPI PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:include>)

    # Заголовки КОМПАСа нужны только при сборке ComKompas.cpp.
    # Каталог lib/ обязателен: там лежат .tlb, которые ищет #import.
    target_include_directories(KompasAPI PRIVATE
        "${KOMPAS_SDK_DIR}/Include"
        "${KOMPAS_SDK_DIR}"
        "${KOMPAS_SDK_DIR}/lib")

    # Реализацию выбирают макросы; в Include/ они не используются, поэтому PRIVATE.
    target_compile_definitions(KompasAPI PRIVATE KAPI5 KAPI7)

    foreach(_lib kAPI7 kAPI5 kAPI3D5 kAPI2D5)
        set(_lib_path "${KOMPAS_SDK_DIR}/lib64/${_lib}.lib")
        if(NOT EXISTS "${_lib_path}")
            message(FATAL_ERROR "Не найдена библиотека SDK: ${_lib_path}")
        endif()
        target_link_libraries(KompasAPI PUBLIC "${_lib_path}")
    endforeach()

    target_link_libraries(KompasAPI PUBLIC ole32 oleaut32 uuid)

    if(MSVC)
        # Исходники в UTF-8 БЕЗ BOM. Без /utf-8 MSVC читает их как ANSI и русские
        # строки (сообщения об ошибках, имена объектов) превращаются в мусор.
        # PUBLIC, потому что публичные заголовки с русскими литералами компилируются
        # в составе кода пользователя — его исходники тоже должны быть в UTF-8.
        target_compile_options(KompasAPI PUBLIC /utf-8)
        target_compile_options(KompasAPI PRIVATE
            /bigobj      # #import разворачивает .tlb КОМПАСа в очень крупные объектники
            /wd4250      # "наследуется через превосходство" — так и задуман pimpl
            /wd4278      # #import: имена из .tlb, совпадающие с макросами Windows
            /wd4828)     # LIBTOOL.H самого SDK лежит в cp1251 и ругается под /utf-8
    endif()

else()
    message(FATAL_ERROR "KOMPASAPI_BACKEND должен быть Api7 или None, а не '${KOMPASAPI_BACKEND}'")
endif()

add_library(KompasAPI::KompasAPI ALIAS KompasAPI)

# Раскладка файлов в IDE как на диске
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             FILES ${KOMPASAPI_PUBLIC_HEADERS} ${KOMPASAPI_API7_HEADERS})

# ---------------------------------------------------------------- установка --
option(KOMPASAPI_INSTALL "Добавить правила установки KompasAPI" OFF)

if(KOMPASAPI_INSTALL)
    install(TARGETS KompasAPI EXPORT KompasAPITargets)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Include/"
            DESTINATION include
            FILES_MATCHING PATTERN "*.h")
    install(EXPORT KompasAPITargets
            NAMESPACE   KompasAPI::
            DESTINATION lib/cmake/KompasAPI)
endif()
