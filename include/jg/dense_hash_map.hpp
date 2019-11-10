#ifndef JG_DENSE_HASH_MAP_HPP
#define JG_DENSE_HASH_MAP_HPP

#include "details/node.hpp"
#include "details/projection_iterator.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace jg
{
namespace details
{
    template <class Key, class Value, bool isConst>
    struct bucket_iterator
    {
    };
} // namespace details

template <
    class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, T>>>
class dense_hash_map
{
private:
    using entries_container_type = std::vector<details::node<Key, T>>;

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
    using iterator = details::projection_iterator<Key, T, false, true>;
    using const_iterator = details::projection_iterator<Key, T, true, true>;

private:
    std::vector<size_type> buckets_;
    entries_container_type entries_;
};

} // namespace jg

#endif // JG_DENSE_HASH_MAP_HPP