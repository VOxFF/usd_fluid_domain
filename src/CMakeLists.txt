add_library(usd_fluid_domain_lib STATIC
    StageReader.cpp
    SurfaceExtractor.cpp
    DomainGenerator.cpp
    DomainConfig.cpp
)

target_include_directories(usd_fluid_domain_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${PXR_INCLUDE_DIRS}
)

target_link_libraries(usd_fluid_domain_lib
    PUBLIC usd usdGeom sdf tf vt gf arch
)

add_executable(usd_fluid_domain main.cpp)

target_link_libraries(usd_fluid_domain
    PRIVATE usd_fluid_domain_lib
)
