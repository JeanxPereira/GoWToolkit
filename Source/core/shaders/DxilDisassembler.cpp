#include "DxilDisassembler.h"
#include <Onyx/Services/Logger.h>

#ifdef _WIN32
#include <windows.h>
#include <dxcapi.h>
#endif

namespace Onyx {

#ifdef _WIN32

namespace {

using PFN_DxcCreateInstance = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);

struct DxcModule {
    HMODULE               handle = nullptr;
    PFN_DxcCreateInstance create  = nullptr;
    bool                  ok      = false;
    std::string           reason;
};

const DxcModule& Module() {
    static const DxcModule mod = [] {
        DxcModule m;
        m.handle = ::LoadLibraryW(L"dxcompiler.dll");
        if (!m.handle) {
            m.reason = "dxcompiler.dll was not found next to the executable or "
                       "on PATH. Copy it from the Windows SDK "
                       "(bin/<version>/x64) or the DirectXShaderCompiler "
                       "release to enable disassembly.";
            return m;
        }
        m.create = reinterpret_cast<PFN_DxcCreateInstance>(
            ::GetProcAddress(m.handle, "DxcCreateInstance"));
        if (!m.create) {
            m.reason = "dxcompiler.dll loaded but exports no DxcCreateInstance.";
            return m;
        }
        m.ok = true;
        ONYX_LOGF_INFO("[DxilDisassembler] dxcompiler.dll loaded");
        return m;
    }();
    return mod;
}

// Minimal COM handle that releases on scope exit; the DXC objects are the only
// COM we touch, so a full smart pointer would be overkill.
template <typename T>
struct ComPtr {
    T* p = nullptr;
    ~ComPtr() { if (p) p->Release(); }
    T** operator&() { return &p; }
    T*  operator->() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};

} // namespace

bool DxilDisassembler::Available() { return Module().ok; }

const std::string& DxilDisassembler::UnavailableReason() { return Module().reason; }

bool DxilDisassembler::Disassemble(const uint8_t* dxbc, size_t size,
                                   std::string& out, std::string& error)
{
    out.clear();
    error.clear();

    const DxcModule& mod = Module();
    if (!mod.ok) { error = mod.reason; return false; }
    if (!dxbc || size < 4) { error = "Empty shader container."; return false; }

    ComPtr<IDxcLibrary> library;
    if (FAILED(mod.create(CLSID_DxcLibrary, IID_PPV_ARGS(&library))) || !library) {
        error = "Could not create the DXC library instance.";
        return false;
    }

    // Pinned: the blob borrows our buffer, which outlives the call below.
    ComPtr<IDxcBlobEncoding> source;
    if (FAILED(library->CreateBlobWithEncodingFromPinned(
            dxbc, static_cast<UINT32>(size), 0, &source)) || !source) {
        error = "Could not wrap the shader bytes for DXC.";
        return false;
    }

    ComPtr<IDxcCompiler> compiler;
    if (FAILED(mod.create(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))) || !compiler) {
        error = "Could not create the DXC compiler instance.";
        return false;
    }

    ComPtr<IDxcBlobEncoding> disasm;
    const HRESULT hr = compiler->Disassemble(source.p, &disasm);
    if (FAILED(hr) || !disasm) {
        char buf[128];
        std::snprintf(buf, sizeof(buf),
                      "DXC refused to disassemble the container (hr = 0x%08lX).",
                      static_cast<unsigned long>(hr));
        error = buf;
        return false;
    }

    const char* text = static_cast<const char*>(disasm->GetBufferPointer());
    size_t      len  = disasm->GetBufferSize();
    if (!text || len == 0) { error = "DXC returned an empty disassembly."; return false; }

    // DXC terminates the buffer itself; trim any trailing NULs so the text does
    // not end with stray glyphs when rendered.
    while (len > 0 && text[len - 1] == '\0') --len;
    out.assign(text, len);
    return true;
}

#else // !_WIN32

bool DxilDisassembler::Available() { return false; }

const std::string& DxilDisassembler::UnavailableReason() {
    static const std::string reason =
        "DXIL disassembly needs dxcompiler.dll and is only wired up on Windows.";
    return reason;
}

bool DxilDisassembler::Disassemble(const uint8_t*, size_t,
                                   std::string& out, std::string& error) {
    out.clear();
    error = UnavailableReason();
    return false;
}

#endif

} // namespace Onyx
