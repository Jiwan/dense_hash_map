#ifndef JG_NODE_HPP
#define JG_NODE_HPP

#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace jg::details
{

template <class Key, class T, class Pair = std::pair<Key, T>>
struct node;

template <class Key, class T>
using node_index_t = typename std::vector<node<Key, T>>::size_type;

template <class Key, class T>
constexpr node_index_t<Key, T> node_end_index = std::numeric_limits<node_index_t<Key, T>>::max();

template <class Key, class T>
union key_value_pair
{
    using non_const_pair = std::pair<Key, T>;
    using const_pair = std::pair<const Key, T>;

    template <class... Args>
    constexpr key_value_pair(Args&&... args) : non_const_(std::forward<Args>(args)...)
    {}

    template <class Allocator, class... Args>
    constexpr key_value_pair(std::allocator_arg_t, const Allocator& alloc, Args&&... args)
    {
        auto alloc_copy = alloc;
        std::allocator_traits<Allocator>::construct(
            alloc_copy, &non_const_, std::forward<Args>(args)...);
    }

    constexpr key_value_pair(const key_value_pair& other) noexcept(
        std::is_nothrow_copy_constructible_v<non_const_pair>)
        : non_const_(other.non_const_)
    {}

    constexpr key_value_pair(key_value_pair&& other) noexcept(
        std::is_nothrow_move_constructible_v<non_const_pair>)
        : non_const_(std::move(other.non_const_))
    {}

    constexpr auto operator=(const key_value_pair& other) noexcept(
        std::is_nothrow_copy_assignable_v<non_const_pair>) -> key_value_pair&
    {
        non_const_ = other.non_const_;
        return *this;
    }

    constexpr auto
    operator=(key_value_pair&& other) noexcept(std::is_nothrow_move_assignable_v<non_const_pair>)
        -> key_value_pair&
    {
        non_const_ = std::move(other).non_const_;
        return *this;
    }

    ~key_value_pair() { non_const_.~pair(); }

    non_const_pair non_const_;
    const_pair const_;
};

template <class T, bool = std::is_copy_constructible_v<T>>
struct disable_copy_constructor
{
    constexpr  disable_copy_constructor() = default;
    disable_copy_constructor(const disable_copy_constructor&) = delete;
    constexpr  disable_copy_constructor(disable_copy_constructor&&) = default;
    constexpr  auto operator=(const disable_copy_constructor&) -> disable_copy_constructor& = default;
    constexpr  auto operator=(disable_copy_constructor &&) -> disable_copy_constructor& = default;
};

template <class T>
struct disable_copy_constructor<T, true>
{
};

template <class T, bool = std::is_copy_assignable_v<T>>
struct disable_copy_assignment
{
     constexpr disable_copy_assignment() = default;
    constexpr  disable_copy_assignment(const disable_copy_assignment&) = default;
    constexpr  disable_copy_assignment(disable_copy_assignment&&) = default;
    auto operator=(const disable_copy_assignment&) -> disable_copy_assignment& = delete;
    constexpr  auto operator=(disable_copy_assignment &&) -> disable_copy_assignment& = default;
};

template <class T>
struct disable_copy_assignment<T, true>
{
};

template <class T, bool = std::is_move_constructible_v<T>>
struct disable_move_constructor
{
    constexpr disable_move_constructor() = default;
   constexpr disable_move_constructor(const disable_move_constructor&) = default;
    disable_move_constructor(disable_move_constructor&&) = delete;
    constexpr auto operator=(const disable_move_constructor&) -> disable_move_constructor& = default;
    constexpr  auto operator=(disable_move_constructor &&) -> disable_move_constructor& = default;
};

template <class T>
struct disable_move_constructor<T, true>
{
};

template <class T, bool = std::is_move_assignable_v<T>>
struct disable_move_assignment
{
    disable_move_assignment() = default;
    disable_move_assignment(const disable_move_assignment&) = default;
    disable_move_assignment(disable_move_assignment&&) = default;
    auto operator=(const disable_move_assignment&) -> disable_move_assignment& = default;
    auto operator=(disable_move_assignment &&) -> disable_move_assignment& = delete;
};

template <class T>
struct disable_move_assignment<T, true>
{
};

template <class Key, class T, class Pair>
struct node : disable_copy_constructor<Pair>,
              disable_copy_assignment<Pair>,
              disable_move_constructor<Pair>,
              disable_move_assignment<Pair>
{
    template <class... Args>
    constexpr node(node_index_t<Key, T> next, Args&&... args) : next(next), pair(std::forward<Args>(args)...)
    {}

    template <class Allocator, class... Args>
    constexpr node(std::allocator_arg_t, const Allocator& alloc, node_index_t<Key, T> next, Args&&... args)
        : next(next), pair(std::allocator_arg, alloc, std::forward<Args>(args)...)
    {}

    template <class Allocator, class Node>
    constexpr node(std::allocator_arg_t, const Allocator& alloc, const Node& other)
        : next(other.next), pair(std::allocator_arg, alloc, other.pair.non_const_)
    {}

    template <class Allocator, class Node>
    constexpr node(std::allocator_arg_t, const Allocator& alloc, Node&& other)
        : next(std::move(other.next))
        , pair(std::allocator_arg, alloc, std::move(other.pair.non_const_))
    {}

    node_index_t<Key, T> next = node_end_index<Key, T>;
    key_value_pair<Key, T> pair;
};

} // namespace jg::details

namespace std
{
template <class Key, class T, class Allocator>
struct uses_allocator<jg::details::node<Key, T>, Allocator> : true_type
{
};
} // namespace std
#endif // JG_NODE_HPP
