#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/DomainBuilder.h>
#include <ufd/DomainConfig.h>
#include <ufd/EnvelopeBuilder.h>
#include <ufd/StageComposer.h>

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

    // 3. Build the fluid domain into its own layer
    const std::string domain_path = output_path + ".domain.usda";

    ufd::DomainConfig config;
    ufd::DomainBuilder builder(config);

    auto domain_stage = pxr::UsdStage::CreateNew(domain_path);
    if (!domain_stage) {
        std::cerr << "Error: cannot create domain stage " << domain_path
                  << std::endl;
        return 1;
    }

    builder.build(domain_stage, bounds);

    // 4. Build the watertight envelope into its own layer
    const std::string envelope_path = output_path + ".envelope.usda";

    auto envelope_stage = pxr::UsdStage::CreateNew(envelope_path);
    if (!envelope_stage) {
        std::cerr << "Error: cannot create envelope stage " << envelope_path
                  << std::endl;
        return 1;
    }

    ufd::EnvelopeBuilder(ufd::EnvelopeConfig{}).build(envelope_stage, meshes);

    // 5. Compose all components into a root layer
    ufd::StageComposer composer(output_path);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);

    if (!composer.write()) {
        std::cerr << "Error: cannot write composed stage " << output_path
                  << std::endl;
        return 1;
    }

    std::cout << "Written: " << domain_path << std::endl;
    std::cout << "Written: " << envelope_path << std::endl;
    std::cout << "Written: " << output_path << std::endl;
    return 0;
}
