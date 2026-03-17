#pragma once
#include "../../../schema/StructDef.h"
#include <memory>

namespace GOW {
namespace Formats {
namespace GOW2 {

class MDL {
public:
    // Factory que retorna a definição reflexiva de dados para uma Malha (MDL) de GOW2
    static std::unique_ptr<StructDef> CreateSchema();
};

} // namespace GOW2
} // namespace Formats
} // namespace GOW
