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
#include <tuple>
#include <utility>
#include <vector>

namespace jg
{
namespace details
{
    static constexpr const float default_max_load_factor = 0.875f;

    template <class Key, class T, bool isConst, bool projectToConstKey>
    dense_hash_map_iterator<Key, T, isConst, projectToConstKey> bucket_iterator_to_iterator(
        const bucket_iterator<Key, T, isConst, projectToConstKey>& bucket_it,
        std::vector<node<Key, T>>& vec)
    {
        return {bucket_it.current_node_index(),
                std::next(vec.begin(), bucket_it.current_node_index())};
    }
} // namespace details

template <
    class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>,
    class GrowthPolicy = details::power_of_two_growth_policy>
class dense_hash_map
{
private:
    using node_type = details::node<Key, T>;
    using entries_container_type = std::vector<node_type>;
    using node_index_type = details::node_index_type<Key, T>;
    using GrowthPolicy::compute_closest_capacity;
    using GrowthPolicy::compute_index;
    using GrowthPolicy::minimum_capacity;
    
    static constexpr node_index_type node_end_index = details::node_end_index<Key, T>;

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

    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {}

    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }

    size_type size() const noexcept { return nodes_.size(); }

    size_type max_size() const noexcept { return nodes_.max_size(); }

    void clear() noexcept
    {
        nodes_.clear();
        buckets_.clear();
        // TODO: re-init the container.
    }

    iterator begin() noexcept { return {nodes_.begin()}; }

    const_iterator begin() const noexcept { return {nodes_.begin()}; }

    const_iterator cbegin() const noexcept { return {nodes_.cbegin()}; }

    iterator end() noexcept { return {nodes_.end()}; }

    const_iterator end() const noexcept { return {nodes_.end()}; }

    const_iterator cend() const noexcept { return {nodes_.cend()}; }

    local_iterator begin(size_type n) { return {buckets_[n], nodes_}; }

    const_local_iterator begin(size_type n) const { return {buckets_[n], nodes_}; }

    const_local_iterator cbegin(size_type n) const { return {buckets_[n], nodes_}; }

    local_iterator end(size_type /*n*/) { return {nodes_}; }

    const_local_iterator end(size_type /*n*/) const { return {nodes_}; }

    const_local_iterator cend(size_type /*n*/) const { return {nodes_}; }

    size_type bucket_count() const { return buckets_.size(); }

    size_type bucket_size(size_type n) const
    {
        return static_cast<size_t>(std::distance(begin(n), end(n)));
    }

    size_type bucket(const key_type& key) const
    {
        return compute_index(hash_(key), buckets_.size());
    }

    float load_factor() const { return size() / bucket_count(); }

    float max_load_factor() const { return max_load_factor_; }

    void max_load_factor(float ml) { max_load_factor_ = ml; }

    void rehash(size_type count) 
    {
        count = std::max(minimum_capacity(), count);
        count = std::max(count, (size() / max_load_factor()));

        std::fill(buckets_.begin(), buckets_.end(), node_end_index);

        node_index_type index{0u};

        for (auto& entry : nodes_) 
        {
            entry->next = 
            reinsert_entry(entry, index);
            index++;
        }
    }

    iterator erase(const_iterator pos)
    {
        // TODO:
    }
    
    iterator erase(const_iterator first, const_iterator last);
    size_type erase(const key_type& key)
    {
        // We have to find out the node we look for and the pointer to it.
        auto bucket_index = bucket(key);

        std::size_t* previous_next = &buckets_[bucket_index]; 
        
        auto bucket_it = begin(buckets_[bucket_index]);
        auto bucket_end = end(0u);

        while (key_equal(bucket_it->const_.first, key) && bucket_it != bucket_end) {
            previous_next = &bucket_it->next;
            ++bucket_it; 
        }

        // The key was never in the map to start with.
        if (bucket_it == bucket_end) {
            return 0;
        }

        auto it = bucket_iterator_to_iterator(bucket_it);
        
        do_erase(previous_next, it);

        return 1;
    }

private:
    local_iterator find_in_bucket(const key_type& key, std::size_t bucket_index)
    {
        auto b = begin(buckets_[bucket_index]);
        auto e = end(0u);
        auto it = std::find(b, e, [&key, this](auto& p) { return key_equal_(p.const_.first, key); });
        return it;
    }

    std::pair<iterator, bool> do_erase(std::size_t* previous_next, iterator it) 
    {
        // Skip the node by pointing the previous "next" to the one it currently point to. 
        *previous_next = it->next;

        auto last = std::prev(end());

        // No need to do anything if the node was at the end of the vector.
        if (it == last) {
            nodes_.pop_back();
            return {end(), true};            
        }
        
        // Swap last node and the one we want to delete, and pop the last one.
        using std::swap;
        swap(*it, *last);
        nodes_.pop_back();
        
        // Now it points to the one we swapped with. We have to readjust it.
        std::size_t* bucket_index = bucket(it->first);

        // We can just base ourselves on the position in the nodes.
        auto old_position = nodes_.size();

        previous_next = &buckets_[bucket_index];
        while (*previous_next != old_position) {
            previous_next = &nodes_[*previous_next].next; 
        }

        *previous_next = std::distance(begin(), it);

        return {it, true};
    }

    void get_prev_pointer_and_bucket_it() {
    }

    void reinsert_entry(node_type& entry, node_index_type index)
    {
        auto bucket_index = bucket(entry.pair.const_.first);
        auto old_index = std::exchange(buckets_[bucket_index], index); 
        buckets_[bucket_index] = old_index; 
    }

    void check_for_rehash() 
    {
        if (size() + 1 >  bucket_count() * max_load_factor()) {
            rehash(bucket_count() * 2);
        }
    }

    template <class... ValueArgs>
    std::pair<iterator, bool> do_emplace(key_type&& key, ValueArgs... args)
    {
        check_for_rehash();

        auto bucket_index = bucket(key);
        auto local_it = find_in_bucket(key, bucket_index);

        if (local_it != end(0u)) { return {details::bucket_iterator_to_iterator(local_it, nodes_), false}; }

        nodes_.emplace_back(
            {std::move(key), std::forward_as_tuple(std::forward<ValueArgs>(args)...)},
            buckets_[bucket_index]
        );
        buckets_[bucket_index] = nodes_.size() - 1;

        return std::prev(end());
    }

    std::vector<size_type> buckets_;
    entries_container_type nodes_;
    float max_load_factor_;

    // TODO: EBO
    hasher hash_;
    key_equal key_equal_;
};

} // namespace jg

#endif // JG_DENSE_HASH_MAP_HPP
