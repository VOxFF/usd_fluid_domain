#include <ufd/StageReader.h>

#include <pxr/usd/usd/primRange.h>

namespace ufd {

bool StageReader::open(const std::string& usd_file_path) {
    stage_ = UsdStage::Open(usd_file_path);
    return static_cast<bool>(stage_);
}

std::vector<UsdGeomMesh> StageReader::collect_meshes() const {
    std::vector<UsdGeomMesh> meshes;
    if (!stage_) {
        return meshes;
    }

    for (const auto& prim : stage_->Traverse()) {
        if (prim.IsA<UsdGeomMesh>()) {
            meshes.emplace_back(prim);
        }
    }
    return meshes;
}

} // namespace ufd
