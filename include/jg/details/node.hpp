#ifndef JG_NODE_HPP
#define JG_NODE_HPP

#include <limits>
#include <utility>
#include <vector>

namespace jg::details
{

template <class Key, class T>
struct node;

template <class Key, class T>
using node_index_type = typename std::vector<node<Key, T>>::size_type;

template <class Key, class T>
union key_value_pair
{
    std::pair<Key, T> non_const_;
    std::pair<const Key, T> const_;

    ~key_value_pair()
    {
        using namespace std;
        non_const_.~pair();
    }
};

template <class Key, class T>
struct node
{
    key_value_pair<Key, T> pair;
    node_index_type<Key, T> next;
};

template <class Key, class T>
constexpr node_index_type<Key, T>
    node_end_index = std::numeric_limits<node_index_type<Key, T>>::max();

} // namespace jg::details
#endif // JG_NODE_HPP
