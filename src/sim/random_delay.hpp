#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cunqa {
namespace sim {

using Rng = std::mt19937_64;

// Random non-negative duration D, in seconds, used for every stochastic delay of the
// simulated-time model: EPR generation times and classical-message jitter.
//
// Besides sampling it (exact strategy), each distribution provides the expectations the
// averaged strategy needs to replace a random wait by its averaged relaxation channel:
//
//   laplace(s)            = E[exp(-s D)]
//   excess_laplace(s, a)  = E[exp(-s max(0, D - a))]   time spent waiting for D after a
//   excess_mean(a)        = E[max(0, D - a)]
//   deficit_laplace(s, a) = E[exp(-s max(0, a - D))]   time D spends waiting until a
//   deficit_mean(a)       = E[max(0, a - D)]
//
// With s = 1/T1 and s = 1/T2 these give the amplitude and coherence decays of a qubit that
// idles for the random time, i.e. the Laplace transform of the waiting-time distribution.
class RandomDelay {
public:
    virtual ~RandomDelay() = default;

    virtual double sample(Rng& rng) const = 0;
    virtual double mean() const = 0;
    virtual double min() const = 0;
    virtual bool is_deterministic() const { return false; }

    virtual double laplace(double s) const = 0;
    virtual double excess_laplace(double s, double a) const = 0;
    virtual double excess_mean(double a) const = 0;
    virtual double deficit_laplace(double s, double a) const = 0;
    virtual double deficit_mean(double a) const = 0;

    // Finite set of (value, probability) points approximating the distribution, with at
    // most max_points entries. Used to build the distribution of the maximum of several
    // delays (multi-party entanglement).
    virtual std::vector<std::pair<double, double>> discretize(std::size_t max_points) const = 0;
};

using RandomDelayPtr = std::shared_ptr<const RandomDelay>;

namespace detail {

// (1 - exp(-x)) / x, continuous at x = 0.
inline double one_minus_exp_over(double x)
{
    return (std::abs(x) < 1e-12) ? 1.0 - x / 2.0 : -std::expm1(-x) / x;
}

} // End of detail namespace


class ConstantDelay final : public RandomDelay {
public:
    explicit ConstantDelay(double value) : value_{value}
    {
        if (!(value >= 0.0) || !std::isfinite(value))
            throw std::invalid_argument("A delay must be a finite non-negative number.");
    }

    double sample(Rng&) const override { return value_; }
    double mean() const override { return value_; }
    double min() const override { return value_; }
    bool is_deterministic() const override { return true; }

    double laplace(double s) const override { return std::exp(-s * value_); }
    double excess_laplace(double s, double a) const override { return std::exp(-s * std::max(0.0, value_ - a)); }
    double excess_mean(double a) const override { return std::max(0.0, value_ - a); }
    double deficit_laplace(double s, double a) const override { return std::exp(-s * std::max(0.0, a - value_)); }
    double deficit_mean(double a) const override { return std::max(0.0, a - value_); }

    std::vector<std::pair<double, double>> discretize(std::size_t) const override
    {
        return {{value_, 1.0}};
    }

private:
    double value_;
};


// D uniform in [low, high].
class UniformDelay final : public RandomDelay {
public:
    UniformDelay(double low, double high) : low_{low}, high_{high}
    {
        if (!(low >= 0.0) || !(high > low) || !std::isfinite(high))
            throw std::invalid_argument("A uniform delay needs 0 <= low < high.");
    }

    double sample(Rng& rng) const override
    {
        return std::uniform_real_distribution<double>(low_, high_)(rng);
    }
    double mean() const override { return 0.5 * (low_ + high_); }
    double min() const override { return low_; }

    double laplace(double s) const override
    {
        return std::exp(-s * low_) * detail::one_minus_exp_over(s * width_());
    }

    double excess_laplace(double s, double a) const override
    {
        const double b = std::clamp(a, low_, high_);
        // P(D <= a) + integral_b^high exp(-s (x - a)) dx / w
        return (b - low_) / width_()
             + std::exp(-s * (b - a)) * (high_ - b) / width_() * detail::one_minus_exp_over(s * (high_ - b));
    }

    double excess_mean(double a) const override
    {
        const double b = std::clamp(a, low_, high_);
        return ((high_ - a) * (high_ - a) - (b - a) * (b - a)) / (2.0 * width_());
    }

    double deficit_laplace(double s, double a) const override
    {
        const double c = std::clamp(a, low_, high_);
        // P(D >= a) + integral_low^c exp(-s (a - x)) dx / w
        return (high_ - c) / width_()
             + std::exp(-s * (a - c)) * (c - low_) / width_() * detail::one_minus_exp_over(s * (c - low_));
    }

    double deficit_mean(double a) const override
    {
        const double c = std::clamp(a, low_, high_);
        return ((a - low_) * (a - low_) - (a - c) * (a - c)) / (2.0 * width_());
    }

    std::vector<std::pair<double, double>> discretize(std::size_t max_points) const override
    {
        const std::size_t n = std::max<std::size_t>(1, max_points);
        std::vector<std::pair<double, double>> points;
        points.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
            points.emplace_back(low_ + (i + 0.5) * width_() / n, 1.0 / n);
        return points;
    }

private:
    double width_() const { return high_ - low_; }

    double low_;
    double high_;
};


// D = offset + E, with E exponentially distributed with the given mean.
class ExponentialDelay final : public RandomDelay {
public:
    ExponentialDelay(double offset, double scale) : offset_{offset}, scale_{scale}
    {
        if (!(offset >= 0.0) || !(scale > 0.0) || !std::isfinite(scale))
            throw std::invalid_argument("An exponential delay needs offset >= 0 and mean > 0.");
    }

    double sample(Rng& rng) const override
    {
        return offset_ + std::exponential_distribution<double>(1.0 / scale_)(rng);
    }
    double mean() const override { return offset_ + scale_; }
    double min() const override { return offset_; }

    double laplace(double s) const override
    {
        return std::exp(-s * offset_) / (1.0 + s * scale_);
    }

    double excess_laplace(double s, double a) const override
    {
        const double ap = a - offset_;
        if (ap <= 0.0)
            return std::exp(s * ap) / (1.0 + s * scale_);
        // Memoryless: either E <= a', or the remaining wait is again exponential.
        const double tail = std::exp(-ap / scale_);
        return (1.0 - tail) + tail / (1.0 + s * scale_);
    }

    double excess_mean(double a) const override
    {
        const double ap = a - offset_;
        return (ap <= 0.0) ? scale_ - ap : scale_ * std::exp(-ap / scale_);
    }

    double deficit_laplace(double s, double a) const override
    {
        const double ap = a - offset_;
        if (ap <= 0.0)
            return 1.0;
        // P(E >= a') + E[exp(-s (a' - E)); E < a']
        const double tail = std::exp(-ap / scale_);
        const double x = ap * (s - 1.0 / scale_);
        return tail * (1.0 + (ap / scale_) * detail::one_minus_exp_over(x));
    }

    double deficit_mean(double a) const override
    {
        const double ap = a - offset_;
        return (ap <= 0.0) ? 0.0 : ap + scale_ * std::expm1(-ap / scale_);
    }

    std::vector<std::pair<double, double>> discretize(std::size_t max_points) const override
    {
        // Equal-probability bins, each represented by its conditional mean, so the
        // discretization keeps the mean of the distribution.
        const std::size_t n = std::max<std::size_t>(1, max_points);
        std::vector<std::pair<double, double>> points;
        points.reserve(n);
        double previous_partial = 0.0; // integral_0^x t f(t) dt, for the bin lower edge
        for (std::size_t i = 0; i < n; ++i) {
            const double upper_cdf = static_cast<double>(i + 1) / n;
            double partial;
            if (i + 1 == n) {
                partial = scale_;
            } else {
                const double x = -scale_ * std::log1p(-upper_cdf);
                partial = scale_ - (x + scale_) * std::exp(-x / scale_);
            }
            points.emplace_back(offset_ + (partial - previous_partial) * n, 1.0 / n);
            previous_partial = partial;
        }
        return points;
    }

private:
    double offset_;
    double scale_;
};


// Heralded entanglement generation: D = N * attempt_time, with N the number of attempts
// until the first success, N ~ Geom(p) on {1, 2, ...}.
class GeometricAttemptsDelay final : public RandomDelay {
public:
    GeometricAttemptsDelay(double success_probability, double attempt_time)
        : p_{success_probability}
        , tau_{attempt_time}
    {
        if (!(success_probability > 0.0) || success_probability > 1.0)
            throw std::invalid_argument("The EPR success probability must be in (0, 1].");
        if (!(attempt_time >= 0.0) || !std::isfinite(attempt_time))
            throw std::invalid_argument("The EPR attempt time must be a finite non-negative number.");
        log_q_ = std::log1p(-p_);
    }

    double sample(Rng& rng) const override
    {
        if (p_ >= 1.0)
            return tau_;
        // std::geometric_distribution counts the failures before the first success.
        const auto failures = std::geometric_distribution<std::uint64_t>(p_)(rng);
        return (static_cast<double>(failures) + 1.0) * tau_;
    }

    double mean() const override { return tau_ / p_; }
    double min() const override { return tau_; }
    bool is_deterministic() const override { return p_ >= 1.0 || tau_ == 0.0; }

    double laplace(double s) const override
    {
        return attempts_laplace_(s);
    }

    double excess_laplace(double s, double a) const override
    {
        if (a <= 0.0)
            return std::exp(s * a) * attempts_laplace_(s);
        const double m = completed_attempts_(a);
        const double q_m = q_pow_(m);
        return (1.0 - q_m) + q_m * std::exp(s * (a - m * tau_)) * attempts_laplace_(s);
    }

    double excess_mean(double a) const override
    {
        if (a <= 0.0)
            return mean() - a;
        const double m = completed_attempts_(a);
        return q_pow_(m) * (m * tau_ - a + tau_ / p_);
    }

    double deficit_laplace(double s, double a) const override
    {
        const double m = completed_attempts_(a);
        if (m < 1.0)
            return 1.0;
        const double q_m = q_pow_(m);
        // sum_{n=1}^{m} p q^(n-1) exp(-s (a - n tau)): a geometric series of ratio q e^(s tau),
        // summed from its largest end so that no term overflows.
        const double log_ratio = log_q_ + s * tau_;
        double sum;
        if (log_ratio <= 0.0) {
            const double first = p_ * std::exp(-s * (a - tau_));
            sum = (log_ratio == 0.0) ? first * m : first * std::expm1(m * log_ratio) / std::expm1(log_ratio);
        } else {
            const double last = p_ * std::exp((m - 1.0) * log_q_ - s * (a - m * tau_));
            sum = last * std::expm1(-m * log_ratio) / std::expm1(-log_ratio);
        }
        return sum + q_m;
    }

    double deficit_mean(double a) const override
    {
        const double m = completed_attempts_(a);
        if (m < 1.0)
            return 0.0;
        const double q_m = q_pow_(m);
        const double one_minus_q_m = -std::expm1(m * log_q_);
        // E[N; N <= m] = (1 - q^m) / p - m q^m
        return a * one_minus_q_m - tau_ * (one_minus_q_m / p_ - m * q_m);
    }

    std::vector<std::pair<double, double>> discretize(std::size_t max_points) const override
    {
        if (is_deterministic())
            return {{mean(), 1.0}};

        // Attempts needed to leave a tail below 1e-12; past max_points the attempts are
        // grouped, each group represented by its conditional mean.
        const double n_tail = std::ceil(std::log(1e-12) / log_q_);
        const std::size_t points_wanted = std::max<std::size_t>(1, max_points) - 1;
        const double group = std::max(1.0, std::ceil(n_tail / static_cast<double>(points_wanted)));

        std::vector<std::pair<double, double>> points;
        double start = 1.0; // first attempt of the current group
        while (start <= n_tail && points.size() < points_wanted) {
            const double end = start + group - 1.0;
            // P(start <= N <= end) and E[N; start <= N <= end]
            const double mass = q_pow_(start - 1.0) - q_pow_(end);
            const double partial = expected_attempts_upto_(end) - expected_attempts_upto_(start - 1.0);
            if (mass > 0.0)
                points.emplace_back(tau_ * partial / mass, mass);
            start = end + 1.0;
        }
        // Remaining tail, memoryless: E[N | N > n] = n + 1/p.
        const double n_done = start - 1.0;
        const double tail = q_pow_(n_done);
        if (tail > 0.0)
            points.emplace_back(tau_ * (n_done + 1.0 / p_), tail);
        return points;
    }

private:
    double p_;
    double tau_;
    double log_q_;

    double q_pow_(double n) const { return (p_ >= 1.0) ? (n > 0.0 ? 0.0 : 1.0) : std::exp(n * log_q_); }

    // Number of attempts that have finished by time a.
    double completed_attempts_(double a) const
    {
        if (tau_ <= 0.0)
            return (a >= 0.0) ? std::numeric_limits<double>::infinity() : 0.0;
        return std::floor(a / tau_ + 1e-12);
    }

    // E[exp(-s N tau)] = p e^(-s tau) / (1 - q e^(-s tau))
    double attempts_laplace_(double s) const
    {
        const double e = std::exp(-s * tau_);
        const double denominator = -std::expm1(-s * tau_) + p_ * e;
        return (denominator > 0.0) ? p_ * e / denominator : 1.0;
    }

    // E[N; N <= n] = (1 - q^n) / p - n q^n
    double expected_attempts_upto_(double n) const
    {
        if (n <= 0.0)
            return 0.0;
        return -std::expm1(n * log_q_) / p_ - n * q_pow_(n);
    }
};


// Distribution with finite support, e.g. tabulated generation times precomputed offline
// with a network simulator, or the maximum of several delays.
class DiscreteDelay final : public RandomDelay {
public:
    explicit DiscreteDelay(std::vector<std::pair<double, double>> points)
    {
        double total = 0.0;
        for (const auto& [value, weight] : points) {
            if (!(value >= 0.0) || !std::isfinite(value))
                throw std::invalid_argument("Tabulated delays must be finite non-negative numbers.");
            if (!(weight >= 0.0) || !std::isfinite(weight))
                throw std::invalid_argument("Tabulated weights must be finite non-negative numbers.");
            total += weight;
        }
        if (!(total > 0.0))
            throw std::invalid_argument("A tabulated delay needs at least one point with positive weight.");

        std::sort(points.begin(), points.end());
        for (const auto& [value, weight] : points) {
            if (weight <= 0.0)
                continue;
            if (!points_.empty() && points_.back().first == value)
                points_.back().second += weight / total;
            else
                points_.emplace_back(value, weight / total);
        }
        cumulative_.reserve(points_.size());
        double acc = 0.0;
        for (const auto& point : points_) {
            acc += point.second;
            cumulative_.push_back(acc);
        }
        cumulative_.back() = 1.0;
    }

    // Index of the point drawn, so that quantities tabulated together with the delay
    // (e.g. the Bell coefficients of the delivered pair) can be read for the same draw.
    std::size_t sample_index(Rng& rng) const
    {
        const double u = std::uniform_real_distribution<double>(0.0, 1.0)(rng);
        const auto it = std::upper_bound(cumulative_.begin(), cumulative_.end(), u);
        return std::min<std::size_t>(it - cumulative_.begin(), points_.size() - 1);
    }

    const std::vector<std::pair<double, double>>& points() const { return points_; }

    double sample(Rng& rng) const override { return points_[sample_index(rng)].first; }

    double mean() const override
    {
        return sum_([](double d) { return d; });
    }
    double min() const override { return points_.front().first; }
    bool is_deterministic() const override { return points_.size() == 1; }

    double laplace(double s) const override
    {
        return sum_([s](double d) { return std::exp(-s * d); });
    }
    double excess_laplace(double s, double a) const override
    {
        return sum_([s, a](double d) { return std::exp(-s * std::max(0.0, d - a)); });
    }
    double excess_mean(double a) const override
    {
        return sum_([a](double d) { return std::max(0.0, d - a); });
    }
    double deficit_laplace(double s, double a) const override
    {
        return sum_([s, a](double d) { return std::exp(-s * std::max(0.0, a - d)); });
    }
    double deficit_mean(double a) const override
    {
        return sum_([a](double d) { return std::max(0.0, a - d); });
    }

    std::vector<std::pair<double, double>> discretize(std::size_t max_points) const override
    {
        if (points_.size() <= std::max<std::size_t>(1, max_points))
            return points_;
        // Merge consecutive points into groups of equal size, keeping mass and mean.
        const std::size_t group = (points_.size() + max_points - 1) / max_points;
        std::vector<std::pair<double, double>> merged;
        for (std::size_t i = 0; i < points_.size(); i += group) {
            double mass = 0.0, first_moment = 0.0;
            for (std::size_t j = i; j < std::min(i + group, points_.size()); ++j) {
                mass += points_[j].second;
                first_moment += points_[j].first * points_[j].second;
            }
            merged.emplace_back(first_moment / mass, mass);
        }
        return merged;
    }

    // Distribution of max(D_1, ..., D_n) for independent delays.
    static std::shared_ptr<const DiscreteDelay> max_of(
        const std::vector<RandomDelayPtr>& delays,
        std::size_t max_points = 2048)
    {
        if (delays.empty())
            throw std::invalid_argument("max_of needs at least one delay.");

        std::vector<std::vector<std::pair<double, double>>> parts;
        std::vector<double> support;
        for (const auto& delay : delays) {
            parts.push_back(delay->discretize(max_points));
            for (const auto& point : parts.back())
                support.push_back(point.first);
        }
        std::sort(support.begin(), support.end());
        support.erase(std::unique(support.begin(), support.end()), support.end());

        // P(max <= x) = prod_k P(D_k <= x), evaluated on the merged support.
        std::vector<std::size_t> cursor(parts.size(), 0);
        std::vector<double> cdf_k(parts.size(), 0.0);
        std::vector<std::pair<double, double>> points;
        double previous_cdf = 0.0;
        for (double x : support) {
            double cdf = 1.0;
            for (std::size_t k = 0; k < parts.size(); ++k) {
                while (cursor[k] < parts[k].size() && parts[k][cursor[k]].first <= x)
                    cdf_k[k] += parts[k][cursor[k]++].second;
                cdf *= cdf_k[k];
            }
            if (cdf > previous_cdf)
                points.emplace_back(x, cdf - previous_cdf);
            previous_cdf = cdf;
        }
        return std::make_shared<const DiscreteDelay>(std::move(points));
    }

private:
    std::vector<std::pair<double, double>> points_;
    std::vector<double> cumulative_;

    template <typename F>
    double sum_(F&& f) const
    {
        double acc = 0.0;
        for (const auto& [value, probability] : points_)
            acc += probability * f(value);
        return acc;
    }
};


// Quantizes a sampled delay to the nearest multiple of the bin width, so that shots whose
// delays fall in the same bin share one realization. Rounding keeps continuous delays
// unbiased and leaves the lattice of attempt times untouched when the width divides the
// attempt time. A width of zero leaves the delay as sampled.
inline double bin_delay(double delay, double bin_width)
{
    if (bin_width <= 0.0)
        return delay;
    return std::round(delay / bin_width) * bin_width;
}

} // End of sim namespace
} // End of cunqa namespace
