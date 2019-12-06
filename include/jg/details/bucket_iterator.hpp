#ifndef JG_BUCKET_ITERATOR_HPP
#define JG_BUCKET_ITERATOR_HPP

#include "node.hpp"

#include <iterator>
#include <vector>

namespace jg::details
{

template <class Key, class T, bool isConst, bool projectToConstKey>
class bucket_iterator
{
    using entries_container_type = std::vector<node<Key, T>>;
    using node_index_type = node_index_t<Key, T>;
    using projected_type =
        std::pair<typename std::conditional<projectToConstKey, const Key, Key>::type, T>;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::conditional_t<isConst, const projected_type, projected_type>;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using pointer = value_type*;

    bucket_iterator() = default;
    bucket_iterator(entries_container_type& entries_container)
        : entries_container_(&entries_container)
    {}

    bucket_iterator(node_index_type index, entries_container_type& entries_container)
        : entries_container_(&entries_container), current_node_index_(index)
    {}

    constexpr auto operator*() const noexcept -> reference
    {
        if constexpr (projectToConstKey)
        {
            return (*entries_container_)[current_node_index_].pair.const_;
        }
        else
        {
            return (*entries_container_)[current_node_index_].pair.non_const_;
        }
    }

    constexpr auto operator++() noexcept -> bucket_iterator&
    {
        current_node_index_ = (*entries_container_)[current_node_index_].next;
        return *this;
    }

    constexpr auto operator++(int) noexcept -> bucket_iterator
    {
        auto old = (*this);
        ++(*this);
        return old;
    }

    constexpr auto operator-> () const noexcept -> pointer
    {
        if constexpr (projectToConstKey)
        {
            return &(*entries_container_)[current_node_index_].pair.const_;
        }
        else
        {
            return &(*entries_container_)[current_node_index_].pair.non_const_;
        }
    }

    auto current_node_index() const -> node_index_type { return current_node_index_; }

private:
    entries_container_type* entries_container_;
    node_index_type current_node_index_ = node_end_index<Key, T>;
};

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr auto operator!=(
    const bucket_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const bucket_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept -> bool
{
    return lhs.current_node_index() != rhs.current_node_index();
}

} // namespace jg::details

#endif // JG_BUCKET_ITERATOR_HPP
