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

    const std::string input_path  = argv[1];
    const std::string output_path = argv[2];

    // 1. Read the input stage
    ufd::StageReader reader;
    if (!reader.open(input_path)) {
        std::cerr << "Error: cannot open stage " << input_path << std::endl;
        return 1;
    }

    auto meshes = reader.collect_meshes();
    if (meshes.empty()) {
        std::cerr << "Warning: no meshes found in stage." << std::endl;
    }

    // 2. Extract the combined surface and compute its bounding box
    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    auto bounds  = extractor.compute_bounding_box(surface);

    // 3. Generate the fluid domain
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto output_stage = pxr::UsdStage::CreateNew(output_path);
    if (!output_stage) {
        std::cerr << "Error: cannot create output stage " << output_path
                  << std::endl;
        return 1;
    }

    auto domain_path = generator.generate(output_stage, bounds);
    output_stage->GetRootLayer()->Save();

    std::cout << "Domain written to " << output_path
              << " at " << domain_path << std::endl;
    return 0;
}
