#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "sim/random_delay.hpp"

using namespace cunqa::sim;

namespace {

struct NamedDelay {
    std::string name;
    RandomDelayPtr delay;
    double scale; // typical size of the delay, used to pick the evaluation points
};

std::vector<NamedDelay> distributions()
{
    return {
        {"constant", std::make_shared<ConstantDelay>(2e-6), 2e-6},
        {"uniform", std::make_shared<UniformDelay>(1e-6, 5e-6), 3e-6},
        {"exponential", std::make_shared<ExponentialDelay>(1e-6, 3e-6), 4e-6},
        {"geometric", std::make_shared<GeometricAttemptsDelay>(0.1, 1e-6), 1e-5},
        {"rare_geometric", std::make_shared<GeometricAttemptsDelay>(0.002, 1e-6), 5e-4},
        {"certain_geometric", std::make_shared<GeometricAttemptsDelay>(1.0, 1e-6), 1e-6},
        {"discrete", std::make_shared<DiscreteDelay>(std::vector<std::pair<double, double>>{
            {1e-6, 0.2}, {2e-6, 0.5}, {7e-6, 0.3}}), 3e-6},
    };
}

class RandomDelayTest : public ::testing::TestWithParam<NamedDelay> {
protected:
    static constexpr std::size_t SAMPLES = 400000;

    std::vector<double> samples_() const
    {
        Rng rng(1234);
        std::vector<double> values(SAMPLES);
        for (auto& value : values)
            value = GetParam().delay->sample(rng);
        return values;
    }

    template <typename F>
    static double average_(const std::vector<double>& values, F&& f)
    {
        double acc = 0.0;
        for (double value : values)
            acc += f(value);
        return acc / values.size();
    }
};

TEST_P(RandomDelayTest, MomentsMatchMonteCarlo)
{
    const auto& delay = *GetParam().delay;
    const double scale = GetParam().scale;
    const auto values = samples_();

    const double mean = average_(values, [](double d) { return d; });
    const double second = average_(values, [](double d) { return d * d; });
    const double std_error = std::sqrt(std::max(0.0, second - mean * mean) / SAMPLES);
    EXPECT_NEAR(delay.mean(), mean, 5.0 * std_error + 1e-15);

    for (double a : {-0.5 * scale, 0.0, 0.3 * scale, scale, 3.0 * scale}) {
        const double excess = average_(values, [a](double d) { return std::max(0.0, d - a); });
        const double deficit = average_(values, [a](double d) { return std::max(0.0, a - d); });
        EXPECT_NEAR(delay.excess_mean(a), excess, 5.0 * std_error + 1e-15) << "a = " << a;
        EXPECT_NEAR(delay.deficit_mean(a), deficit, 5.0 * std_error + 1e-15) << "a = " << a;
    }
}

TEST_P(RandomDelayTest, LaplaceTransformsMatchMonteCarlo)
{
    const auto& delay = *GetParam().delay;
    const double scale = GetParam().scale;
    const auto values = samples_();
    // An average of values in [0, 1] has a standard error below 0.5 / sqrt(N).
    const double tolerance = 5.0 * 0.5 / std::sqrt(static_cast<double>(SAMPLES));

    for (double s : {0.1 / scale, 1.0 / scale, 10.0 / scale}) {
        EXPECT_NEAR(delay.laplace(s), average_(values, [s](double d) { return std::exp(-s * d); }), tolerance)
            << "s = " << s;
        for (double a : {-0.5 * scale, 0.0, 0.3 * scale, scale, 3.0 * scale}) {
            const double excess = average_(values, [s, a](double d) { return std::exp(-s * std::max(0.0, d - a)); });
            const double deficit = average_(values, [s, a](double d) { return std::exp(-s * std::max(0.0, a - d)); });
            EXPECT_NEAR(delay.excess_laplace(s, a), excess, tolerance) << "s = " << s << ", a = " << a;
            EXPECT_NEAR(delay.deficit_laplace(s, a), deficit, tolerance) << "s = " << s << ", a = " << a;
        }
    }
}

TEST_P(RandomDelayTest, ExcessMinusDeficitIsMeanShift)
{
    // max(0, D - a) - max(0, a - D) = D - a for every realization.
    const auto& delay = *GetParam().delay;
    const double scale = GetParam().scale;
    for (double a : {-scale, 0.0, 0.25 * scale, 0.5 * scale, 2.0 * scale, 10.0 * scale})
        EXPECT_NEAR(delay.excess_mean(a) - delay.deficit_mean(a), delay.mean() - a, 1e-9 * scale) << "a = " << a;
}

TEST_P(RandomDelayTest, LaplaceAtZeroIsOne)
{
    const auto& delay = *GetParam().delay;
    const double scale = GetParam().scale;
    EXPECT_NEAR(delay.laplace(0.0), 1.0, 1e-12);
    EXPECT_NEAR(delay.excess_laplace(0.0, scale), 1.0, 1e-12);
    EXPECT_NEAR(delay.deficit_laplace(0.0, scale), 1.0, 1e-12);
}

TEST_P(RandomDelayTest, DiscretizationKeepsMassAndMean)
{
    const auto& delay = *GetParam().delay;
    const auto points = delay.discretize(256);
    ASSERT_FALSE(points.empty());
    ASSERT_LE(points.size(), 256u);

    double mass = 0.0, mean = 0.0;
    for (const auto& [value, probability] : points) {
        mass += probability;
        mean += value * probability;
    }
    EXPECT_NEAR(mass, 1.0, 1e-9);
    EXPECT_NEAR(mean, delay.mean(), 1e-6 * delay.mean() + 1e-15);
}

INSTANTIATE_TEST_SUITE_P(
    Distributions, RandomDelayTest, ::testing::ValuesIn(distributions()),
    [](const ::testing::TestParamInfo<NamedDelay>& info) { return info.param.name; });

} // End of anonymous namespace

TEST(RandomDelay, GeometricLaplaceIsClosedForm)
{
    const double p = 0.05, tau = 2e-6, s = 1e4;
    const GeometricAttemptsDelay delay(p, tau);
    // sum_n p (1-p)^(n-1) e^(-s n tau)
    double expected = 0.0;
    for (int n = 1; n < 20000; ++n)
        expected += p * std::pow(1.0 - p, n - 1) * std::exp(-s * n * tau);
    EXPECT_NEAR(delay.laplace(s), expected, 1e-12);
    EXPECT_NEAR(delay.mean(), tau / p, 1e-18);
}

TEST(RandomDelay, GeometricDeficitDoesNotOverflow)
{
    // Long prefetch leads with a fast decay rate: the geometric series has ratio above one.
    const GeometricAttemptsDelay delay(1e-3, 1e-6);
    const double value = delay.deficit_laplace(5e3, 0.5);
    EXPECT_TRUE(std::isfinite(value));
    EXPECT_GE(value, 0.0);
    EXPECT_LE(value, 1.0);
}

TEST(RandomDelay, MaximumOfDelaysMatchesMonteCarlo)
{
    const std::vector<RandomDelayPtr> delays{
        std::make_shared<GeometricAttemptsDelay>(0.2, 1e-6),
        std::make_shared<GeometricAttemptsDelay>(0.05, 1e-6),
        std::make_shared<UniformDelay>(2e-6, 4e-6)
    };
    const auto maximum = DiscreteDelay::max_of(delays);

    Rng rng(7);
    const std::size_t samples = 200000;
    double mean = 0.0, laplace = 0.0;
    const double s = 5e4;
    for (std::size_t i = 0; i < samples; ++i) {
        double value = 0.0;
        for (const auto& delay : delays)
            value = std::max(value, delay->sample(rng));
        mean += value / samples;
        laplace += std::exp(-s * value) / samples;
    }
    EXPECT_NEAR(maximum->mean(), mean, 0.01 * mean);
    EXPECT_NEAR(maximum->laplace(s), laplace, 0.005);
}

TEST(RandomDelay, BinningRoundsToTheGrid)
{
    EXPECT_DOUBLE_EQ(bin_delay(3.7e-6, 0.0), 3.7e-6);
    EXPECT_NEAR(bin_delay(3.7e-6, 1e-6), 4e-6, 1e-18);
    EXPECT_NEAR(bin_delay(3.2e-6, 1e-6), 3e-6, 1e-18);
    // Attempt-time lattices are kept when the bin divides the attempt time.
    EXPECT_NEAR(bin_delay(7 * 1e-6, 0.25e-6), 7e-6, 1e-18);
}

TEST(RandomDelay, InvalidParametersAreRejected)
{
    EXPECT_THROW(GeometricAttemptsDelay(0.0, 1e-6), std::invalid_argument);
    EXPECT_THROW(GeometricAttemptsDelay(1.5, 1e-6), std::invalid_argument);
    EXPECT_THROW(GeometricAttemptsDelay(0.5, -1.0), std::invalid_argument);
    EXPECT_THROW(UniformDelay(2.0, 1.0), std::invalid_argument);
    EXPECT_THROW(ExponentialDelay(0.0, 0.0), std::invalid_argument);
    EXPECT_THROW(ConstantDelay(-1.0), std::invalid_argument);
    EXPECT_THROW(DiscreteDelay({}), std::invalid_argument);
    EXPECT_THROW(DiscreteDelay({{1.0, -1.0}}), std::invalid_argument);
}
