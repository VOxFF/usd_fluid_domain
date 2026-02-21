#include <ufd/StageReader.h>

#include <pxr/usd/usd/primRange.h>

namespace ufd {

bool StageReader::Open(const std::string& usdFilePath) {
    _stage = UsdStage::Open(usdFilePath);
    return static_cast<bool>(_stage);
}

std::vector<UsdGeomMesh> StageReader::CollectMeshes() const {
    std::vector<UsdGeomMesh> meshes;
    if (!_stage) {
        return meshes;
    }

    for (const auto& prim : _stage->Traverse()) {
        if (prim.IsA<UsdGeomMesh>()) {
            meshes.emplace_back(prim);
        }
    }
    return meshes;
}

} // namespace ufd
