#ifndef JG_POWER_OF_TWO_GROWTH_POLICY_HPP
#define JG_POWER_OF_TWO_GROWTH_POLICY_HPP

#include <cstddef>

namespace jg::details
{

struct power_of_two_growth_policy {
    static constexpr std::size_t compute_index(std::size_t hash, std::size_t capacity)
    {
        return hash & (capacity - 1);
    }

    static constexpr std::size_t compute_closest_capacity(std::size_t min_capacity)
    {
        --min_capacity;

        min_capacity |= min_capacity >> 1;
        min_capacity |= min_capacity >> 2;
        min_capacity |= min_capacity >> 4;
        min_capacity |= min_capacity >> 8;
        min_capacity |= min_capacity >> 16;

        if constexpr(sizeof(min_capacity) == 8) {
            min_capacity |= min_capacity >> 32;
        }

        return ++min_capacity;
    }

    static constexpr std::size_t minimum_capacity()
    {
        return 8z;
    } 
};

}

#endif // JG_POWER_OF_TWO_GROWTH_POLICY_HPP