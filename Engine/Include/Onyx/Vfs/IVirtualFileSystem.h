#pragma once
#include <Onyx/Vfs/IFile.h>
#include <memory>
#include <string>
#include <vector>

namespace Onyx::Vfs {

class IVirtualFileSystem {
public:
    virtual ~IVirtualFileSystem() = default;

    // Retorna true se o VFS Ã© vÃ¡lido e foi montado com sucesso
    virtual bool IsValid() const = 0;

    // Lista arquivos em um diretÃ³rio virtual
    virtual std::vector<std::string> ListDirectory(const std::string& path) = 0;

    // Abre um arquivo virtual devolvendo um stream gerenciado
    virtual std::unique_ptr<IFile> OpenFile(const std::string& path) = 0;
    
    // Verifica se um arquivo/diretÃ³rio existe
    virtual bool Exists(const std::string& path) = 0;
};

} // namespace Onyx::Vfs
