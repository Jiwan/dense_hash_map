#ifndef JG_POWER_OF_TWO_GROWTH_POLICY_HPP
#define JG_POWER_OF_TWO_GROWTH_POLICY_HPP

#include <cstddef>
#include <limits>

namespace jg::details
{

struct power_of_two_growth_policy
{
    static constexpr auto compute_index(std::size_t hash, std::size_t capacity) -> std::size_t
    {
        return hash & (capacity - 1);
    }

    static constexpr auto compute_closest_capacity(std::size_t min_capacity) -> std::size_t
    {
        --min_capacity;

        min_capacity |= min_capacity >> 1;
        min_capacity |= min_capacity >> 2;
        min_capacity |= min_capacity >> 4;
        min_capacity |= min_capacity >> 8;
        min_capacity |= min_capacity >> 16;

        if constexpr (std::numeric_limits<decltype(min_capacity)>::digits >= 64)
        {
            min_capacity |= min_capacity >> 32;
        }

        return ++min_capacity;
    }

    static constexpr auto minimum_capacity() -> std::size_t { return 8u; }
};

} // namespace jg::details

#endif // JG_POWER_OF_TWO_GROWTH_POLICY_HPP
