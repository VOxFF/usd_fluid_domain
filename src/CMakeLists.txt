add_library(ufd STATIC
    StageReader.cpp
    SurfaceExtractor.cpp
    DomainBuilder.cpp
    DomainConfig.cpp
    StageComposer.cpp
    EnvelopeBuilder.cpp
)

target_include_directories(ufd
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ufd
    SYSTEM PUBLIC ${PXR_INCLUDE_DIRS}
)

target_compile_definitions(ufd
    PUBLIC TBB_SUPPRESS_DEPRECATED_MESSAGES=1
)

target_compile_options(ufd
    PUBLIC -Wno-deprecated -Wno-cpp
)

target_link_libraries(ufd
    PUBLIC usd usdGeom usdShade sdf tf vt gf arch
           OpenVDB::openvdb
)

