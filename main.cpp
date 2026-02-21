#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/DomainGenerator.h>
#include <ufd/DomainConfig.h>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: usd_fluid_domain <input.usd> <output.usd>"
                  << std::endl;
        return 1;
    }

    const std::string inputPath  = argv[1];
    const std::string outputPath = argv[2];

    // 1. Read the input stage
    ufd::StageReader reader;
    if (!reader.Open(inputPath)) {
        std::cerr << "Error: cannot open stage " << inputPath << std::endl;
        return 1;
    }

    auto meshes = reader.CollectMeshes();
    if (meshes.empty()) {
        std::cerr << "Warning: no meshes found in stage." << std::endl;
    }

    // 2. Extract the combined surface and compute its bounding box
    ufd::SurfaceExtractor extractor;
    auto surface = extractor.Extract(meshes);
    auto bounds  = extractor.ComputeBoundingBox(surface);

    // 3. Generate the fluid domain
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto outputStage = pxr::UsdStage::CreateNew(outputPath);
    if (!outputStage) {
        std::cerr << "Error: cannot create output stage " << outputPath
                  << std::endl;
        return 1;
    }

    auto domainPath = generator.Generate(outputStage, bounds);
    outputStage->GetRootLayer()->Save();

    std::cout << "Domain written to " << outputPath
              << " at " << domainPath << std::endl;
    return 0;
}
