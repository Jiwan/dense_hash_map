#ifndef JG_NODE_HPP
#define JG_NODE_HPP

#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace jg::details
{

template <class Key, class T>
struct node;

template <class Key, class T>
using node_index_t = typename std::vector<node<Key, T>>::size_type;

template <class Key, class T>
constexpr node_index_t<Key, T>
    node_end_index = std::numeric_limits<node_index_t<Key, T>>::max();

template <class Key, class T>
union key_value_pair
{
    using non_const_pair = std::pair<Key, T>;
    using const_pair = std::pair<const Key, T>;

    template <class... Args>
    constexpr key_value_pair(Args&&... args)
        : non_const_(std::forward<Args>(args)...)
    {
    }

    template <class Allocator, class... Args>
    key_value_pair(std::allocator_arg_t, Allocator&& alloc, Args&&... args)
    {
        std::allocator_traits<std::remove_cvref_t<Allocator>>::construct(std::forward<Allocator>(alloc), &non_const_, std::forward<Args>(args)...);
    }

    constexpr key_value_pair(const key_value_pair& other) // TODO: noexcept and enable_if_t
        : non_const_(other.non_const_)
    {
    }

    constexpr key_value_pair(key_value_pair&& other) // TODO: noexcept and enable_if_t
        : non_const_(std::move(other.non_const_))
    {
    }

    constexpr auto operator=(const key_value_pair& other) -> key_value_pair& // TODO: noexcept and enable_if_t
    {
        non_const_ = other.non_const_;
    }

    constexpr auto operator=(key_value_pair&& other) -> key_value_pair&& // TODO: noexcept and enable_if_t
    {
        non_const_ = std::move(other).non_const_;
    }

    ~key_value_pair()
    {
        non_const_.~pair();
    }

    non_const_pair non_const_;
    const_pair const_;
};

template <class Key, class T>
struct node
{
    template <class... Args>
    node(node_index_t<Key, T> next, Args&&... args)
        : next(next)
        , pair(std::forward<Args>(args)...)
    {
    }

    template <class Allocator, class... Args>
    node(std::allocator_arg_t, Allocator&& alloc, node_index_t<Key, T> next, Args&&... args)
        : next(next)
        , pair(std::allocator_arg, std::forward<Allocator>(alloc), std::forward<Args>(args)...)
    {
    }

    node_index_t<Key, T> next = node_end_index<Key, T>;
    key_value_pair<Key, T> pair;
};

} // namespace jg::details

namespace std {
    template <class Key, class T, class Allocator>
    struct uses_allocator<jg::details::node<Key, T>, Allocator> : true_type {};
}
#endif // JG_NODE_HPP
