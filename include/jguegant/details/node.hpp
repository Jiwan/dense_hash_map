#ifndef JGUEGANT_NODE_HPP
#define JGUEGANT_NODE_HPP

#include <utility>

namespace jguegant::details
{
template <class Key, class Value>
union key_value_pair 
{
    std::pair<Key, Value> non_const_;
    std::pair<const Key, Value> const_; 
};

template <class Key, class Value>
struct node
{
    key_value_pair<Key, Value> pair;
    node* next;
};
}
#endif // JGUEGANT_NODE_HPP