#pragma once
#include <Onyx/Vfs/IVirtualFileSystem.h>
#include <Onyx/Vfs/IFile.h>
#include <Onyx/Schema/NodeInstance.h>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

#include <Onyx/Domain/Wad.h>
#include <Onyx/Types/GameVersion.h>
#include <Onyx/Types/TypeId.h>

namespace Onyx::Domain {

class IAssetProfile {
public:
    virtual ~IAssetProfile() = default;

    // Nome descritivo (ex: "God of War I (PS2)")
    virtual std::string GetName() const = 0;

    // Verifica se este perfil tem capacidade de ler essa ISO ou arquivo
    virtual bool Detect(const std::filesystem::path& path) const = 0;

    // Monta uma ISO inteira em um sistema de diretórios navegável
    virtual std::shared_ptr<Vfs::IVirtualFileSystem> MountArchive(const std::filesystem::path& path) = 0;

    // Abre uma WAD/Pak do jogo e povoa a AssetContainer com o conteúdo / nós base disponíveis
    virtual bool ParseContainer(std::shared_ptr<Vfs::IFile> file, AssetContainer& outWad) = 0;

    // Dado um VFS (ex: ISO montada), o profile procura por seus arquivos base (TOC/PAK)
    // e popula a estrutura do jogo no AssetContainer
    virtual bool LoadFromArchive(std::shared_ptr<Vfs::IVirtualFileSystem> vfs, AssetContainer& outWad) = 0;

    // True if this entry represents an openable container (e.g. a WAD/archive
    // node the user can drill into). Default: false. Profiles override.
    virtual bool IsContainerEntry(const AssetEntry& entry) const { return false; }

};

} // namespace Onyx::Domain
