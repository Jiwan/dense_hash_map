#ifndef JG_DENSE_HASH_MAP_HPP
#define JG_DENSE_HASH_MAP_HPP

#include "details/bucket_iterator.hpp"
#include "details/dense_hash_map_iterator.hpp"
#include "details/node.hpp"
#include "details/power_of_two_growth_policy.hpp"

#include <algorithm>
#include <cassert>
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
    [[nodiscard]] constexpr auto bucket_iterator_to_iterator(
        const bucket_iterator<Key, T, isConst, projectToConstKey>& bucket_it,
        std::vector<node<Key, T>>& vec)
        -> dense_hash_map_iterator<Key, T, isConst, projectToConstKey>
    {
        return dense_hash_map_iterator<Key, T, isConst, projectToConstKey>{std::next(vec.begin(), bucket_it.current_node_index())};
    }

    template <class Key, class T, bool isConst, bool projectToConstKey>
    [[nodiscard]] constexpr auto const_iterator_to_iterator(
        const dense_hash_map_iterator<Key, T, true, true>& const_it, std::vector<node<Key, T>>& vec)
        -> dense_hash_map_iterator<Key, T, false, true>
    {
        const auto distance = std::distance(vec.cbegin(), const_it);
        return dense_hash_map_iterator<Key, T, isConst, projectToConstKey>{std::next(vec.begin(), distance)};
    }

    template <class Alloc, class T>
    using rebind_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
} // namespace details

template <
    class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>,
    class GrowthPolicy = details::power_of_two_growth_policy>
class dense_hash_map : private GrowthPolicy
{
private:
    using node_type = details::node<Key, T>;
    using entries_container_type =
        std::vector<node_type, details::rebind_alloc<Allocator, node_type>>;
    using node_index_type = details::node_index_t<Key, T>;
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
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
    using iterator = details::dense_hash_map_iterator<Key, T, false, true>;
    using const_iterator = details::dense_hash_map_iterator<Key, T, true, true>;
    using local_iterator = details::bucket_iterator<Key, T, false, true>;
    using const_local_iterator = details::bucket_iterator<Key, T, true, true>;

    constexpr dense_hash_map() : dense_hash_map(minimum_capacity()) {}

    constexpr explicit dense_hash_map(
        size_type bucket_count, const Hash& hash = Hash(), const key_equal& equal = key_equal(),
        const allocator_type& alloc = allocator_type())
        : buckets_(alloc), nodes_(alloc), hash_(hash), key_equal_(equal), alloc_(alloc)
    {
        rehash(bucket_count);
    }

    constexpr dense_hash_map(size_type bucket_count, const allocator_type& alloc)
        : dense_hash_map(bucket_count, hasher(), key_equal(), alloc)
    {}

    constexpr dense_hash_map(
        size_type bucket_count, const hasher& hash, const allocator_type& alloc)
        : dense_hash_map(bucket_count, hash, key_equal(), alloc)
    {}

    constexpr explicit dense_hash_map(const allocator_type& alloc)
        : dense_hash_map(minimum_capacity(), hasher(), key_equal(), alloc)
    {}

    template <class InputIt>
    constexpr dense_hash_map(
        InputIt first, InputIt last, size_type bucket_count = minimum_capacity(),
        const Hash& hash = Hash(), const key_equal& equal = key_equal(),
        const allocator_type& alloc = allocator_type())
        : dense_hash_map(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }

    template <class InputIt>
    constexpr dense_hash_map(
        InputIt first, InputIt last, size_type bucket_count, const allocator_type& alloc)
        : dense_hash_map(first, last, bucket_count, hasher(), key_equal(), alloc)
    {}

    template <class InputIt>
    constexpr dense_hash_map(
        InputIt first, InputIt last, size_type bucket_count, const hasher& hash,
        const allocator_type& alloc)
        : dense_hash_map(first, last, bucket_count, hash, key_equal(), alloc)
    {}

    constexpr dense_hash_map(const dense_hash_map& other) /* TODO: noexcept */
        : dense_hash_map(
              other, std::allocator_traits<allocator_type>::select_on_container_copy_construction(
                         other.get_allocator()))
    {}

    constexpr dense_hash_map(
        const dense_hash_map& other, const allocator_type& alloc) /* TODO: noexcept */
        : buckets_(other.buckets_, alloc)
        , nodes_(other.nodes_, alloc)
        , hash_(other.hash_, alloc)
        , key_equal_(other.key_equal_, alloc)
        , alloc_(alloc)
    {}

    constexpr dense_hash_map(dense_hash_map&& other) = default; /* TODO: noexcept */
    constexpr dense_hash_map(
        dense_hash_map&& other, const allocator_type& alloc) /* TODO: noexcept */
        : buckets_(std::move(other.buckets_), alloc)
        , nodes_(std::move(other.nodes_), alloc)
        , hash_(std::move(other.hash_), alloc)
        , key_equal_(std::move(other.key_equal_), alloc)
        , alloc_(alloc)
    {}

    constexpr dense_hash_map(
        std::initializer_list<value_type> init, size_type bucket_count = minimum_capacity(),
        const hasher& hash = hasher(), const key_equal& equal = key_equal(),
        const allocator_type& alloc = allocator_type())
        : dense_hash_map(init.begin(), init.end(), bucket_count, hash, equal, alloc)
    {}

    constexpr dense_hash_map(
        std::initializer_list<value_type> init, size_type bucket_count, const allocator_type& alloc)
        : dense_hash_map(init, bucket_count, hasher(), key_equal(), alloc)
    {}

    constexpr dense_hash_map(
        std::initializer_list<value_type> init, size_type bucket_count, const hasher& hash,
        const allocator_type& alloc)
        : dense_hash_map(init, bucket_count, hash, key_equal(), alloc)
    {}

    ~dense_hash_map() = default;

    constexpr auto get_allocator() const -> allocator_type { return alloc_; }

    constexpr auto insert(const value_type& value) -> std::pair<iterator, bool>
    {
        return emplace(value);
    }

    constexpr auto insert(value_type&& value) -> std::pair<iterator, bool>
    {
        return emplace(std::move(value));
    }

    template <class P, std::enable_if_t<std::is_constructible_v<value_type, P&&>, int> = 0>
    constexpr auto insert(P&& value) -> std::pair<iterator, bool>
    {
        return emplace(std::forward<P>(value));
    }

    constexpr auto insert(const_iterator /*hint*/, const value_type& value) -> iterator
    {
        return insert(value).first;
    }

    constexpr auto insert(const_iterator /*hint*/, value_type&& value) -> iterator
    {
        return insert(std::move(value)).first;
    }

    template<class P, std::enable_if_t<std::is_constructible_v<value_type, P&&>, int> = 0>
    constexpr auto insert(const_iterator /*hint*/, P&& value) -> iterator
    {
        return insert(std::move(value)).first;
    }

    template <class InputIt>
    constexpr void insert(InputIt first, InputIt last)
    {
        for (; first != last; ++first)
        {
            insert(*first);
        }
    }

    constexpr void insert(std::initializer_list<value_type> ilist)
    {
        insert(ilist.begin(), ilist.end());
    }

    template <class... Args>
    auto emplace(Args&&... args) -> std::pair<iterator, bool>
    {
        return dispatch_emplace(std::forward<Args>(args)...);
    }

    template <class... Args>
    constexpr auto try_emplace(const key_type& key, Args&&... args) -> std::pair<iterator, bool>
    {
        return do_emplace(
            key, std::piecewise_construct, std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template <class... Args>
    constexpr auto try_emplace(key_type&& key, Args&&... args) -> std::pair<iterator, bool>
    {
        return do_emplace(
            key, std::piecewise_construct, std::forward_as_tuple(std::move(key)),
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template <class... Args>
    constexpr auto try_emplace(const_iterator /*hint*/, const key_type& key, Args&&... args) -> iterator
    {
        return try_emplace(key, std::forward<Args>(args)...).iterator;
    }

    template <class... Args>
    constexpr auto try_emplace(const_iterator /*hint*/, key_type&& key, Args&&... args) -> iterator
    {
        return try_emplace(std::move(key), std::forward<Args>(args)...).iterator;
    }

    [[nodiscard]] auto empty() const noexcept -> bool { return nodes_.empty(); }

    auto size() const noexcept -> size_type { return nodes_.size(); }

    auto max_size() const noexcept -> size_type { return nodes_.max_size(); }

    void clear() noexcept
    {
        nodes_.clear();
        buckets_.clear();
        rehash(0u);
    }

    auto begin() noexcept -> iterator { return iterator{nodes_.begin()}; }

    auto begin() const noexcept -> const_iterator { return const_iterator{nodes_.begin()}; }

    auto cbegin() const noexcept -> const_iterator { return const_iterator{nodes_.cbegin()}; }

    auto end() noexcept -> iterator { return iterator{nodes_.end()}; }

    auto end() const noexcept -> const_iterator { return const_iterator{nodes_.end()}; }

    auto cend() const noexcept -> const_iterator { return const_iterator{nodes_.cend()}; }

    auto begin(size_type n) -> local_iterator { return {buckets_[n], nodes_}; }

    auto begin(size_type n) const -> const_local_iterator { return {buckets_[n], nodes_}; }

    auto cbegin(size_type n) const -> const_local_iterator { return {buckets_[n], nodes_}; }

    auto end(size_type /*n*/) -> local_iterator { return {nodes_}; }

    auto end(size_type /*n*/) const -> const_local_iterator { return {nodes_}; }

    auto cend(size_type /*n*/) const -> const_local_iterator { return {nodes_}; }

    auto bucket_count() const -> size_type { return buckets_.size(); }

    auto bucket_size(size_type n) const -> size_type
    {
        return static_cast<size_t>(std::distance(begin(n), end(n)));
    }

    auto bucket(const key_type& key) const -> size_type
    {
        return compute_index(hash_(key), buckets_.size());
    }

    auto load_factor() const -> float { return size() / bucket_count(); }

    auto max_load_factor() const -> float { return max_load_factor_; }

    void max_load_factor(float ml)
    { 
        assert(ml > 0.0f);    
        max_load_factor_ = ml; 
    }

    void rehash(size_type count)
    {
        count = std::max(minimum_capacity(), count);
        count = std::max(count, static_cast<size_type>(size() / max_load_factor()));

        if (count == buckets_.size()) {
            return;
        }

        buckets_.resize(count);

        std::fill(buckets_.begin(), buckets_.end(), node_end_index);

        node_index_type index{0u};

        for (auto& entry : nodes_)
        {
            entry.next = node_end_index;
            reinsert_entry(entry, index);
            index++;
        }
    }

    auto erase(const_iterator pos) -> iterator
    {
        auto it = details::const_iterator_to_iterator(pos);
        auto position = std::distance(nodes_.begin(), it);
        auto previous_next = find_previous_next_using_position(it->first, position);
        do_erase(previous_next, it);
    }

    auto erase(const_iterator first, const_iterator last) -> iterator;
    auto erase(const key_type& key) -> size_type
    {
        // We have to find out the node we look for and the pointer to it.
        auto bucket_index = bucket(key);

        std::size_t* previous_next = &buckets_[bucket_index];

        auto bucket_it = begin(buckets_[bucket_index]);
        auto bucket_end = end(0u);

        while (key_equal(bucket_it->const_.first, key) && bucket_it != bucket_end)
        {
            previous_next = &bucket_it->next;
            ++bucket_it;
        }

        // The key was never in the map to start with.
        if (bucket_it == bucket_end)
        {
            return 0;
        }

        auto it = bucket_iterator_to_iterator(bucket_it);

        do_erase(previous_next, it);

        return 1;
    }

    auto find(const Key& key) -> iterator { return find_in_bucket(key, bucket(key)); }

    auto find(const Key& key) const -> const_iterator { return find_in_bucket(key, bucket(key)); }

private:
    auto find_in_bucket(const key_type& key, std::size_t bucket_index) -> local_iterator
    {
        auto b = begin(bucket_index);
        auto e = end(0u);
        auto it =
            std::find_if(b, e, [&key, this](auto& p) { return key_equal_(p.first, key); });
        return it;
    }

    auto do_erase(std::size_t* previous_next, iterator it) -> std::pair<iterator, bool>
    {
        // Skip the node by pointing the previous "next" to the one it currently point to.
        *previous_next = it->next;

        auto last = std::prev(end());

        // No need to do anything if the node was at the end of the vector.
        if (it == last)
        {
            nodes_.pop_back();
            return {end(), true};
        }

        // Swap last node and the one we want to delete, and pop the last one.
        using std::swap;
        swap(*it, *last);
        nodes_.pop_back();

        // Now it points to the one we swapped with. We have to readjust it.
        previous_next = find_previous_next_using_position(it->first, nodes_.size());

        *previous_next = std::distance(begin(), it);

        return {it, true};
    }

    auto find_previous_next_using_position(const key_type& key, std::size_t position)
        -> std::size_t*
    {
        std::size_t* bucket_index = bucket(key);

        auto previous_next = &buckets_[bucket_index];
        while (*previous_next != position)
        {
            previous_next = &nodes_[*previous_next].next;
        }

        return previous_next;
    }

    void reinsert_entry(node_type& entry, node_index_type index)
    {
        auto bucket_index = bucket(entry.pair.const_.first);
        auto old_index = std::exchange(buckets_[bucket_index], index);
        buckets_[bucket_index] = old_index;
    }

    void check_for_rehash()
    {
        if (size() + 1 > bucket_count() * max_load_factor())
        {
            rehash(bucket_count() * 2);
        }
    }

    constexpr auto dispatch_emplace() -> std::pair<iterator, bool>
    {
        return do_emplace(key_type{});
    }

    template <class Key2, class T2>
    auto dispatch_emplace(Key2&& key, T2&& t) -> std::pair<iterator, bool>
    {
        if constexpr (std::is_same_v<std::decay_t<Key2>, key_type>)
        {
            return do_emplace(key, std::forward<Key2>(key), std::forward<T2>(t));
        }
        else
        {
            key_type new_key{std::forward<Key2>(key)};
            // TODO: double check that I am allowed to optimized by sending new_key here and not key https://eel.is/c++draft/unord.req#lib:emplace,unordered_associative_containers
            return do_emplace(new_key, std::move(new_key), std::forward<T2>(t));
        }
    }

    template <class Pair>
    auto dispatch_emplace(Pair&& p) -> std::pair<iterator, bool>
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(p.first)>, key_type>)
        {
            return do_emplace(p.first, std::forward<Pair>(p));
        }
        else
        {
            key_type new_key{std::forward<Pair>(p).first};
            return do_emplace(new_key, std::move(new_key), std::forward<Pair>(p).second);
        }
    }

    template <class... Args1, class... Args2>
    auto dispatch_emplace(
        std::piecewise_construct_t, std::tuple<Args1...> first_args,
        std::tuple<Args2...> second_args) -> std::pair<iterator, bool>
    {
        std::pair<Key, T> p{std::piecewise_construct, std::move(first_args), std::move(second_args)};
        return dispatch_emplace(std::move(p));
    }

    template <class... Args>
    auto do_emplace(const key_type& key, Args&&... args) -> std::pair<iterator, bool>
    {
        check_for_rehash();

        auto bucket_index = bucket(key);
        auto local_it = find_in_bucket(key, bucket_index);

        if (local_it != end(0u))
        {
            return std::pair{details::bucket_iterator_to_iterator(local_it, nodes_), false};
        }

        nodes_.emplace_back(buckets_[bucket_index], std::forward<Args>(args)...);
        buckets_[bucket_index] = nodes_.size() - 1;

        return std::pair{std::prev(end()), true};
    }

    std::vector<size_type, details::rebind_alloc<Allocator, size_type>> buckets_;
    entries_container_type nodes_;
    float max_load_factor_ = details::default_max_load_factor;

    // TODO: EBO
    hasher hash_;
    key_equal key_equal_;
    allocator_type alloc_;
};

} // namespace jg

#endif // JG_DENSE_HASH_MAP_HPP
