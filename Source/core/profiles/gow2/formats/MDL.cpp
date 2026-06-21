#include "MDL.h"

namespace Onyx {
namespace Formats {
namespace GOW2 {

std::unique_ptr<Onyx::Schema::StructDef> MDL::CreateSchema() {
    auto schema = std::make_unique<Onyx::Schema::StructDef>("GOW2_MDL");
    schema->AddUInt32("Magic")
           .AddUInt32("Version")
           .AddUInt32("VertexCount")
           .AddUInt32("IndexCount")
           .AddVector3("BoundingBoxMin")
           .AddVector3("BoundingBoxMax");
           
    return schema;
}

} // namespace GOW2
} // namespace Formats
} // namespace Onyx
