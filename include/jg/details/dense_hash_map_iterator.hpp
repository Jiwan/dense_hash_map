#ifndef JG_DENSE_HASH_MAP_ITERATOR_HPP
#define JG_DENSE_HASH_MAP_ITERATOR_HPP

#include "node.hpp"

#include <type_traits>
#include <vector>

namespace jg::details
{

template <class Key, class T, bool isConst, bool projectToConstKey>
class dense_hash_map_iterator
{
private:
    using entries_container_type = std::vector<node<Key, T>>;
    using sub_iterator_type = typename std::conditional<
        isConst, typename entries_container_type::const_iterator,
        typename entries_container_type::iterator>::type;
    using sub_iterator_type_traits = std::iterator_traits<sub_iterator_type>;
    using projected_type =
        std::pair<typename std::conditional<projectToConstKey, Key, const Key>::type, T>;

public:
    using iterator_category = typename sub_iterator_type_traits::iterator_category;
    using value_type = std::conditional<isConst, const projected_type, projected_type>;
    using difference_type = typename sub_iterator_type_traits::difference_type;
    using reference = value_type&;
    using pointer = value_type*;

    constexpr dense_hash_map_iterator() noexcept : sub_iterator_(sub_iterator_type{}) {}

    explicit constexpr dense_hash_map_iterator(const dense_hash_map_iterator& other) noexcept
        : sub_iterator_(other.sub_iterator_)
    {}

    template <bool DepIsConst = isConst, std::enable_if_t<DepIsConst, int> = 0>
    constexpr dense_hash_map_iterator(
        const dense_hash_map_iterator<Key, T, false, projectToConstKey>& other) noexcept
        : sub_iterator_(other.sub_iterator_)
    {}

    constexpr reference operator*() const noexcept
    {
        if constexpr (projectToConstKey) { return sub_iterator_->pair.const_; }
        else
        {
            return sub_iterator_->pair.non_const_;
        }
    }

    constexpr pointer operator->() const noexcept
    {
        if constexpr (projectToConstKey) { return &(sub_iterator_->pair.const_); }
        else
        {
            return &(sub_iterator_->pair.non_const_);
        }
    }

    constexpr dense_hash_map_iterator& operator++() noexcept
    {
        ++sub_iterator_;
        return *this;
    }

    constexpr dense_hash_map_iterator operator++(int) noexcept { return {sub_iterator_++}; }

    constexpr dense_hash_map_iterator& operator--() noexcept
    {
        --sub_iterator_;
        return *this;
    }

    constexpr dense_hash_map_iterator operator--(int) noexcept { return {sub_iterator_--}; }

    constexpr reference operator[](difference_type index) const noexcept
    {
        if constexpr (projectToConstKey) { return sub_iterator_[index]->pair.const_; }
        else
        {
            return sub_iterator_[index]->pair.non_const_;
        }
    }

    constexpr dense_hash_map_iterator& operator+=(difference_type n) noexcept
    {
        sub_iterator_ += n;
        return *this;
    }

    constexpr dense_hash_map_iterator operator+(difference_type n) const noexcept
    {
        return {sub_iterator_ + n};
    }

    constexpr dense_hash_map_iterator& operator-=(difference_type n) noexcept
    {
        sub_iterator_ -= n;
        return *this;
    }

    constexpr dense_hash_map_iterator operator-(difference_type n) const noexcept
    {
        return {sub_iterator_ - n};
    }

    const sub_iterator_type& sub_iterator() const {
        return sub_iterator_;
    }

private:
    sub_iterator_type sub_iterator_;
};

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator==(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() == rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator!=(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() != rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator<(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() < rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator>(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() > rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator<=(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() <= rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator>=(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() >= rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr typename dense_hash_map_iterator<Key, T, isConst, projectToConstKey>::difference_type
operator-(
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator() - rhs.sub_iterator();
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr dense_hash_map_iterator<Key, T, isConst, projectToConstKey> operator+(
    typename dense_hash_map_iterator<Key, T, isConst, projectToConstKey>::difference_type n,
    const dense_hash_map_iterator<Key, T, isConst, projectToConstKey>& it) noexcept
{
    return {n + it.sub_iterator()};
}

} // namespace jg::details

#endif// JG_DENSE_HASH_MAP_ITERATOR_HPP