#pragma once

#include <ppfbase/preprocessor_utils.h>

// IMPORTANT
// This file is used by various rc files. Don't include any headers that aren't
// entirely made of '#define's in here.
//

// =============================================================================
// version
// =============================================================================
#define TDD_PRODUCT_DOT_VER      0.1.0.0
#define TDD_PRODUCT_COMMA_VER    0,1,0,0

#define TDD_PRODUCT_DOT_VER_STR  TDD_STR(TDD_PRODUCT_DOT_VER)

#define TDD_COMPANY_NAME         "Tarot Driven Development"
#define TDD_COMPANY_NAME_W       TDD_WIDEN(TDD_COMPANY_NAME)
#define TDD_COPYRIGHT            "Copyright (c) 2023"
#define TDD_PRODUCT_NAME         TDD_COMPANY_NAME " Toolbox"
#define TDD_PRODUCT_NAME_W       TDD_WIDEN(TDD_PRODUCT_NAME)

// Values taken from 'verrsrc.h'

#define TDD_VERSION_BIN_TYPE_EXE         0x1L // VFT_APP
#define TDD_VERSION_BIN_TYPE_DLL         0x2L // VFT_DLL


// =============================================================================
// Files
// =============================================================================

#define TDD_MAKE_BIN_DESCRIPTION(str) TDD_PRODUCT_NAME " " str

#define TDD_EMU_LAUNCHER_EXE_A            "emulauncher.exe"
#define TDD_EMU_LAUNCHER_EXE_W            TDD_WIDEN(TDD_EMU_LAUNCHER_EXE_A)
#define TDD_EMU_LAUNCHER_EXE_DESCRIPTION  TDD_MAKE_BIN_DESCRIPTION("Emulator Launcher")

#define TDD_PPF_INJECTOR_DLL_A            "ppfinjector.dll"
#define TDD_PPF_INJECTOR_DLL_W            TDD_WIDEN(TDD_PPF_INJECTOR_DLL_A)
#define TDD_PPF_INJECTOR_DLL_DESCRIPTION  TDD_MAKE_BIN_DESCRIPTION("PPF Injector")

// Leave a new line at the end to keep resource compiler happy
