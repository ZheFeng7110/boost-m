// Experiment: the daubechies_scaling evaluation of tests/math/doc_sf.cpp.
#pragma once
TEST_CASE("repro: daubechies_scaling") {
    boost::math::daubechies_scaling<double, 2> phi(10);
    CHECK(std::abs(phi(1.0) - (1 + std::sqrt(3.0)) / 2) < 1e-12);
}
