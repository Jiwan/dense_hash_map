#ifndef JG_BUCKET_ITERATOR_HPP
#define JG_BUCKET_ITERATOR_HPP

#include "node.hpp"

#include <vector>

namespace jg::details
{

template <class Key, class T, bool isConst, bool projectToConstKey>
class bucket_iterator
{
    using entries_container_type = std::vector<node<Key, T>>;
    using node_index_type = node_index_type<Key, T>;

public:
    bucket_iterator() = default;
    bucket_iterator(entries_container_type& entries_container)
        : entries_container_(&entries_container)
    {

    }


    node_index_type current_node_index() const {
        return current_node_index_;
    }

private:
    entries_container_type* entries_container_;
    node_index_type current_node_index_ = node_end_index<Key, T>;
};
}

#endif // JG_BUCKET_ITERATOR_HPP