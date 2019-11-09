#ifndef JGUEGANT_PROJECTION_ITERATOR_HPP
#define JGUEGANT_PROJECTION_ITERATOR_HPP

#include "node.hpp"

#include <type_traits>
#include <vector>

namespace jguegant::details
{
template <class Key, class T, bool isConst, bool projectToConstKey>
class projection_iterator;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator==(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator!=(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator<(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator>(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator<=(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator>=(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr typename projection_iterator<Key, T, isConst, projectToConstKey>::difference_type
operator-(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr projection_iterator<Key, T, isConst, projectToConstKey> operator+(
    typename projection_iterator<Key, T, isConst, projectToConstKey>::difference_type n,
    const projection_iterator<Key, T, isConst, projectToConstKey>& it) noexcept;

template <class Key, class T, bool isConst, bool projectToConstKey>
class projection_iterator
{
private:
    using entries_container_type = std::vector<node<Key, T>>;
    using sub_iterator_type = typename std::conditional<
        isConst, typename entries_container_type::const_iterator,
        typename entries_container_type::iterator>::type;
    using sub_iterator_type_traits = std::iterator_traits<sub_iterator_type>;
    using projectedType =
        std::pair<typename std::conditional<projectToConstKey, Key, const Key>::type, T>;

public:
    using iterator_category = typename sub_iterator_type_traits::iterator_category;
    using value_type = std::conditional<isConst, const projectedType, projectedType>;
    using difference_type = typename sub_iterator_type_traits::difference_type;
    using reference = value_type&;
    using pointer = value_type*;

    constexpr projection_iterator() noexcept : sub_iterator_(sub_iterator_type{}) {}

    explicit constexpr projection_iterator(const projection_iterator& other) noexcept
        : sub_iterator_(other.sub_iterator_)
    {}

    template <bool DepIsConst = isConst, std::enable_if_t<DepIsConst, int> = 0>
    constexpr projection_iterator(
        const projection_iterator<Key, T, false, projectToConstKey>& other) noexcept
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

    constexpr projection_iterator& operator++() noexcept
    {
        ++sub_iterator_;
        return *this;
    }

    constexpr projection_iterator operator++(int) noexcept { return {sub_iterator_++}; }

    constexpr projection_iterator& operator--() noexcept
    {
        --sub_iterator_;
        return *this;
    }

    constexpr projection_iterator operator--(int) noexcept { return {sub_iterator_--}; }

    constexpr reference operator[](difference_type index) const noexcept
    {
        if constexpr (projectToConstKey) { return sub_iterator_[index]->pair.const_; }
        else
        {
            return sub_iterator_[index]->pair.non_const_;
        }
    }

    constexpr projection_iterator& operator+=(difference_type n) noexcept
    {
        sub_iterator_ += n;
        return *this;
    }

    constexpr projection_iterator operator+(difference_type n) const noexcept
    {
        return {sub_iterator_ + n};
    }

    constexpr projection_iterator& operator-=(difference_type n) noexcept
    {
        sub_iterator_ -= n;
        return *this;
    }

    constexpr projection_iterator operator-(difference_type n) const noexcept
    {
        return {sub_iterator_ - n};
    }

    friend constexpr bool operator== <Key, T, isConst, projectToConstKey>(
        const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr bool operator!= <Key, T, isConst, projectToConstKey>(
        const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr bool operator< <Key, T, isConst, projectToConstKey>(const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr bool operator> <Key, T, isConst, projectToConstKey>(
        const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr bool operator<= <Key, T, isConst, projectToConstKey>(
        const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr bool operator>= <Key, T, isConst, projectToConstKey>(
        const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr difference_type operator- <Key, T, isConst, projectToConstKey>(const projection_iterator& lhs, const projection_iterator& rhs) noexcept;

    friend constexpr projection_iterator operator+ <Key, T, isConst, projectToConstKey>(difference_type n, const projection_iterator& it) noexcept;

private:
    sub_iterator_type sub_iterator_;
};

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator==(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ == rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator!=(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ != rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator<(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ < rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator>(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ > rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator<=(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ <= rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr bool operator>=(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ >= rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr typename projection_iterator<Key, T, isConst, projectToConstKey>::difference_type
operator-(
    const projection_iterator<Key, T, isConst, projectToConstKey>& lhs,
    const projection_iterator<Key, T, isConst, projectToConstKey>& rhs) noexcept
{
    return lhs.sub_iterator_ - rhs.sub_iterator_;
}

template <class Key, class T, bool isConst, bool projectToConstKey>
constexpr projection_iterator<Key, T, isConst, projectToConstKey> operator+(
    typename projection_iterator<Key, T, isConst, projectToConstKey>::difference_type n,
    const projection_iterator<Key, T, isConst, projectToConstKey>& it) noexcept
{
    return {n + it.sub_iterator_};
}

} // namespace jguegant::details

#endif// JGUEGANT_PROJECTION_ITERATOR_HPP