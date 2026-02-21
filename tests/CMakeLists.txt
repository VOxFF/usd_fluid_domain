include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

FetchContent_MakeAvailable(googletest)

add_executable(usd_fluid_domain_tests
    test_StageReader.cpp
    test_SurfaceExtractor.cpp
    test_DomainGenerator.cpp
)

target_link_libraries(usd_fluid_domain_tests
    PRIVATE usd_fluid_domain_lib
    PRIVATE GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(usd_fluid_domain_tests)
