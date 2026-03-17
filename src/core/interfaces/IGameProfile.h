#pragma once
#include "../vfs/IVirtualFileSystem.h"
#include "../vfs/IFile.h"
#include "../schema/NodeInstance.h"
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

#include "../WadTypes.h"

namespace GOW {

class IGameProfile {
public:
    virtual ~IGameProfile() = default;

    // Nome descritivo (ex: "God of War I (PS2)")
    virtual std::string GetName() const = 0;

    // Verifica se este perfil tem capacidade de ler essa ISO ou arquivo
    virtual bool Detect(const std::filesystem::path& path) const = 0;

    // Monta uma ISO inteira em um sistema de diretórios navegável
    virtual std::shared_ptr<IVirtualFileSystem> MountArchive(const std::filesystem::path& path) = 0;

    // Abre uma WAD/Pak do jogo e povoa a OpenWad com o conteúdo / nós base disponíveis
    virtual bool ParseWad(std::shared_ptr<IFile> file, OpenWad& outWad) = 0;

    // Dado um VFS (ex: ISO montada), o profile procura por seus arquivos base (TOC/PAK)
    // e popula a estrutura do jogo no OpenWad
    virtual bool LoadFromArchive(std::shared_ptr<IVirtualFileSystem> vfs, OpenWad& outWad) = 0;

    // Cria uma NodeInstance (ex: MDL, TXR) se o schema existir e carrega os dados
    virtual std::shared_ptr<NodeInstance> CreateNodeInstance(const std::string& typeName, std::shared_ptr<IFile> fileData) = 0;
};

} // namespace GOW
