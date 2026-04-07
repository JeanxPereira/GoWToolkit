#include "MDL.h"

namespace GOW {
namespace Formats {
namespace GOW2 {

std::unique_ptr<StructDef> MDL::CreateSchema() {
    auto schema = std::make_unique<StructDef>("GOW2_MDL");
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
} // namespace GOW
