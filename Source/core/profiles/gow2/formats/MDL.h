#pragma once
#include <Onyx/Schema/StructDef.h>
#include <memory>

namespace Onyx {
namespace Formats {
namespace GOW2 {

class MDL {
public:
    // Factory que retorna a definiÃ§Ã£o reflexiva de dados para uma Malha (MDL) de GOW2
    static std::unique_ptr<Onyx::Schema::StructDef> CreateSchema();
};

} // namespace GOW2
} // namespace Formats
} // namespace Onyx
