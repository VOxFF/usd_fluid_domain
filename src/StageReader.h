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
    bool Open(const std::string& usdFilePath);

    // Traverse the stage and collect all UsdGeomMesh prims.
    std::vector<UsdGeomMesh> CollectMeshes() const;

    // Access the underlying stage.
    UsdStageRefPtr GetStage() const { return _stage; }

private:
    UsdStageRefPtr _stage;
};

} // namespace ufd
