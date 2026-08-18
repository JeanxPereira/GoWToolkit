#pragma once
#include <cstdint>
#include <string>

namespace Onyx {

// Disassembles a DXBC/DXIL container into text.
//
// DXIL is LLVM bitcode, so there is no practical way to disassemble it by hand;
// the work is delegated to Microsoft's dxcompiler.dll, which is what dxc.exe
// itself calls. The DLL is loaded lazily with LoadLibrary and resolved by name,
// so the toolkit neither links against it nor requires it to be present: when
// it is missing the caller gets a reason string and can fall back to the
// structured container view.
//
// Ship dxcompiler.dll next to the executable, or rely on one already on PATH
// (the Windows SDK installs one under bin/<version>/x64).
class DxilDisassembler {
public:
    // True once the DLL has been found and its entry point resolved.
    static bool Available();

    // Human-readable reason why Available() is false. Empty when it is true.
    static const std::string& UnavailableReason();

    // Disassembles a complete DXBC container (starting at its 'DXBC' magic).
    // Returns false and fills `error` on failure.
    static bool Disassemble(const uint8_t* dxbc, size_t size,
                            std::string& out, std::string& error);
};

} // namespace Onyx
