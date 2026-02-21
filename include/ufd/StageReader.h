#pragma once

#include <string>
#include <vector>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

class StageReader {
public:
    // Open a USD stage from a file path.
    bool open(const std::string& usd_file_path);

    // Traverse the stage and collect all UsdGeomMesh prims.
    std::vector<UsdGeomMesh> collect_meshes() const;

    // Access the underlying stage.
    UsdStageRefPtr get_stage() const { return stage_; }

private:
    UsdStageRefPtr stage_;
};

} // namespace ufd
