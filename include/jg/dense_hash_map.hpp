#ifndef JG_DENSE_HASH_MAP_HPP
#define JG_DENSE_HASH_MAP_HPP

#include "details/bucket_iterator.hpp"
#include "details/dense_hash_map_iterator.hpp"
#include "details/node.hpp"
#include "details/power_of_two_growth_policy.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

namespace jg
{
namespace details
{
    static constexpr const float default_max_load_factor = 0.875f;
} // namespace details

template <
    class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>,
    class GrowthPolicy = details::power_of_two_growth_policy
    >
class dense_hash_map
{
private:
    using entries_container_type = std::vector<details::node<Key, T>>;
    using node_index_type = details::node_index_type<Key, T>;
    using GrowthPolicy::compute_closest_capacity;
    using GrowthPolicy::compute_index;
    using GrowthPolicy::minimum_capacity;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = typename entries_container_type::size_type;
    using difference_type = typename entries_container_type::difference_type;
    using hasher = Hash;
    using key_equal = KeyEqual; // TODO: do the proper thing from cppref.
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using iterator = details::dense_hash_map_iterator<Key, T, false, true>;
    using const_iterator = details::dense_hash_map_iterator<Key, T, true, true>;
    using local_iterator = details::bucket_iterator<Key, T, false, true>;
    using const_local_iterator = details::bucket_iterator<Key, T, true, true>;


    template<class... Args>
    std::pair<iterator,bool> emplace(Args&&... args)
    {

    }

    [[nodiscard]] bool empty() const noexcept
    {
        return entries_.empty();
    }

    size_type size() const noexcept 
    {
        return entries_.size();
    }

    size_type max_size() const noexcept
    {
        return entries_.max_size();
    }

    void clear() noexcept
    {
        entries_.clear();
        buckets_.clear();
        // TODO: re-init the container.
    }

    size_type bucket_count() const 
    {
        return buckets_.size();
    }

    float load_factor() const 
    {
        return size() /  bucket_count();
    }

    float max_load_factor() const
    {
        return max_load_factor_;
    }
    
    void max_load_factor(float ml)
    {
        max_load_factor_ = ml;
    }

    void rehash(size_type count)
    {

    }

private:

    node_index_type key_to_index(const Key& k)
    {
        return compute_index(hash_(k), buckets_.size()); 
    }


    template<class... ValueArgs>
    void do_emplace(Key&& key, ValueArgs... args)
    {
        


        for ()
        
    }

    std::vector<size_type> buckets_;
    entries_container_type entries_;
    float max_load_factor_;
    Hash hash_;
};

} // namespace jg

#endif // JG_DENSE_HASH_MAP_HPP