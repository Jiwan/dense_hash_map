#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file

#include <catch2/catch_test_macros.hpp>
#include "jg/dense_hash_map.hpp"
#include "jg/details/type_traits.hpp"

#include <algorithm>
#include <memory_resource>
#include <string>
#include <vector>

namespace
{
struct increase_counter_on_copy_or_move
{
    increase_counter_on_copy_or_move(std::size_t* counter_ptr) : counter_ptr{counter_ptr} {}

    increase_counter_on_copy_or_move(const increase_counter_on_copy_or_move& other)
        : counter_ptr(other.counter_ptr)
    {
        ++(*counter_ptr);
    }

    increase_counter_on_copy_or_move(increase_counter_on_copy_or_move&& other)
        : counter_ptr(std::move(other.counter_ptr))
    {
        ++(*counter_ptr);
    }

    auto operator=(const increase_counter_on_copy_or_move& other)
        -> increase_counter_on_copy_or_move&
    {
        counter_ptr = other.counter_ptr;
        ++(*counter_ptr);
        return *this;
    }

    auto operator=(increase_counter_on_copy_or_move&& other) -> increase_counter_on_copy_or_move&
    {
        counter_ptr = std::move(other.counter_ptr);
        ++(*counter_ptr);
        return *this;
    }

    std::size_t* counter_ptr = nullptr;
};

auto operator==(const increase_counter_on_copy_or_move&, const increase_counter_on_copy_or_move&)
    -> bool
{
    return true;
}

struct collision_hasher
{
    template <class T>
    auto operator()(T&&) const noexcept -> std::size_t
    {
        return 0;
    }
};

template <class T, class propagateSwap = std::false_type>
struct named_allocator
{
    using value_type = T;
    using propagate_on_container_swap = propagateSwap;

    named_allocator() = default;

    named_allocator(std::string name) : name(std::move(name)) {}

    template <class U, class propagateSwap2>
    constexpr named_allocator(const named_allocator<U, propagateSwap2>& other) noexcept
        : name(other.name)
    {}

    auto allocate(std::size_t n) -> T* { return std::allocator<T>{}.allocate(n); }

    void deallocate(T* p, std::size_t s) noexcept { std::allocator<T>{}.deallocate(p, s); }

    std::string name;
};

template <class T, class U, class propagateSwap>
auto operator==(
    const named_allocator<T, propagateSwap>& lhs, const named_allocator<U, propagateSwap>& rhs)
    -> bool
{
    return lhs.name == rhs.name;
}
template <class T, class U, class propagateSwap>
auto operator!=(
    const named_allocator<T, propagateSwap>& lhs, const named_allocator<U, propagateSwap>& rhs)
    -> bool
{
    return lhs.name != rhs.name;
}

struct nested_string
{
    std::string value;
};

struct string_hash
{
    using transparent_key_equal = std::equal_to<>;
    using hash_type = std::hash<std::string>;
    auto operator()(const std::string& s) const -> size_t { return hash_type{}(s); }
    auto operator()(const nested_string& s) const -> size_t { return hash_type{}(s.value); }
};

auto operator==(const std::string& s, const nested_string& ns) -> bool { return s == ns.value; }

template <typename UnnamedType>
struct is_valid_container
{
private:
    template <typename... Params>
    constexpr auto test_validity(int /* unused */)
        -> decltype(std::declval<UnnamedType>()(std::declval<Params>()...), std::true_type())
    {
        return std::true_type();
    }

    template <typename... Params>
    constexpr auto test_validity(...) -> std::false_type
    {
        return std::false_type();
    }

public:
    template <typename... Params>
    constexpr auto operator()(Params&&...)
    {
        return test_validity<Params...>(int());
    }
};

template <typename UnnamedType>
constexpr auto is_valid(UnnamedType&&)
{
    return is_valid_container<UnnamedType>();
}

struct counting_pmr_resource : std::pmr::memory_resource
{
    counting_pmr_resource(int* alloc_counter) : alloc_counter(alloc_counter) {}

    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override
    {
        ++(*alloc_counter);
        return std::pmr::get_default_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
    {
        std::pmr::get_default_resource()->deallocate(p, bytes, alignment);
    }

    auto do_is_equal(const std::pmr::memory_resource&) const noexcept -> bool override
    {
        return true;
    }

    int* alloc_counter = nullptr;
};
} // namespace

namespace std
{
template <>
struct hash<increase_counter_on_copy_or_move>
{
    auto operator()(const increase_counter_on_copy_or_move&) const noexcept -> std::size_t
    {
        return 0;
    }
};
} // namespace std

TEST_CASE("member types")
{
    jg::dense_hash_map<std::string, int> m;

    REQUIRE(std::is_same_v<decltype(m)::key_type, std::string>);
    REQUIRE(std::is_same_v<decltype(m)::mapped_type, int>);
    REQUIRE(std::is_same_v<decltype(m)::value_type, std::pair<const std::string, int>>);
    REQUIRE(sizeof(decltype(m)::size_type) > 1);
    REQUIRE(sizeof(decltype(m)::difference_type) > 1);
    REQUIRE(std::is_same_v<decltype(m)::hasher, std::hash<std::string>>);
    REQUIRE(std::is_same_v<decltype(m)::key_equal, std::equal_to<std::string>>);
    REQUIRE(std::is_same_v<
            decltype(m)::allocator_type, std::allocator<std::pair<const std::string, int>>>);
    REQUIRE(std::is_same_v<decltype(m)::reference, std::pair<const std::string, int>&>);
    REQUIRE(std::is_same_v<decltype(m)::const_reference, const std::pair<const std::string, int>&>);
    REQUIRE(std::is_pointer_v<decltype(m)::pointer>);
    REQUIRE(std::is_pointer_v<decltype(m)::const_pointer>);
    REQUIRE(std::is_same_v<decltype(m)::iterator, decltype(m.begin())>);
    REQUIRE(std::is_same_v<decltype(m)::const_iterator, decltype(m.cbegin())>);
    REQUIRE(std::is_same_v<decltype(m)::local_iterator, decltype(m.begin(0u))>);
    REQUIRE(std::is_same_v<decltype(m)::const_local_iterator, decltype(m.cbegin(0u))>);
}

TEST_CASE("constructors")
{
    jg::dense_hash_map<std::string, int> m = {{"pikachu", 40}, {"raichu", 43}};

    SECTION("empty")
    {
        REQUIRE(std::is_nothrow_default_constructible_v<decltype(m)>);

        jg::dense_hash_map<std::string, int> m1;
        REQUIRE(m1.size() == 0);
        REQUIRE(m1.bucket_count() == 8);
    }

    SECTION("bucket_count / hash / equal / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{32};
        REQUIRE(m1.size() == 0);
        REQUIRE(m1.bucket_count() == 32);

        jg::dense_hash_map<std::string, int> m2{32, std::hash<std::string>{}};
        REQUIRE(m2.size() == 0);
        REQUIRE(m2.bucket_count() == 32);

        jg::dense_hash_map<std::string, int> m3{6, std::hash<std::string>{},
                                                std::equal_to<std::string>{}};
        REQUIRE(m3.size() == 0);
        REQUIRE(m3.bucket_count() == 8);

        jg::dense_hash_map<std::string, int> m4{
            0, std::hash<std::string>{}, std::equal_to<std::string>{},
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m4.size() == 0);
        REQUIRE(m4.bucket_count() == 8);
    }

    SECTION("bucket_count / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            64, std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 0);
        REQUIRE(m1.bucket_count() == 64);
    }

    SECTION("bucket_count / hash / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            9, std::hash<std::string>{}, std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 0);
        REQUIRE(m1.bucket_count() == 16);
    }

    SECTION("alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 0);
        REQUIRE(m1.bucket_count() == 8);
    }

    SECTION("input iterator / bucket_count / hash / equal / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{m.begin(), m.end()};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("pikachu"));
        REQUIRE(m1["pikachu"] == 40);

        jg::dense_hash_map<std::string, int> m2{m.begin(), m.end(), 32};
        REQUIRE(m2.size() == 2);
        REQUIRE(m2.bucket_count() == 32);
        REQUIRE(m2.contains("pikachu"));
        REQUIRE(m2["pikachu"] == 40);

        jg::dense_hash_map<std::string, int> m3{m.begin(), m.end(), 32, std::hash<std::string>{}};
        REQUIRE(m3.size() == 2);
        REQUIRE(m3.bucket_count() == 32);
        REQUIRE(m3.contains("raichu"));
        REQUIRE(m3["raichu"] == 43);

        jg::dense_hash_map<std::string, int> m4{m.begin(), m.end(), 6, std::hash<std::string>{},
                                                std::equal_to<std::string>{}};
        REQUIRE(m4.size() == 2);
        REQUIRE(m4.bucket_count() == 8);
        REQUIRE(m4.contains("pikachu"));
        REQUIRE(m4["pikachu"] == 40);

        jg::dense_hash_map<std::string, int> m5{
            m.begin(),
            m.end(),
            0,
            std::hash<std::string>{},
            std::equal_to<std::string>{},
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m5.size() == 2);
        REQUIRE(m5.bucket_count() == 8);
        REQUIRE(m5.contains("pikachu"));
        REQUIRE(m5["pikachu"] == 40);
    }

    SECTION("input iterator / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            m.begin(), m.end(), std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);
    }

    SECTION("input iterator / bucket_count / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            m.begin(), m.end(), 8, std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);
    }

    SECTION("input iterator / bucket_count / hash / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            m.begin(), m.end(), 8, std::hash<std::string>{},
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("pikachu"));
        REQUIRE(m1["pikachu"] == 40);
    }

    SECTION("copy constructor")
    {
        jg::dense_hash_map<std::string, int> m1{m};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);

        REQUIRE(m.size() == 2);
        REQUIRE(m.bucket_count() == 8);
        REQUIRE(m.contains("raichu"));
        REQUIRE(m["raichu"] == 43);
    }

    SECTION("copy constructor / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            m, std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);
    }

    SECTION("move constructor")
    {
        jg::dense_hash_map<std::string, int> m1{std::move(m)};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);

        REQUIRE(m.size() == 0);
        REQUIRE(m.bucket_count() == 0);

        // Moved dense_hash_map usable after clear.
        m.clear();
        REQUIRE(m.bucket_count() == 8);
        REQUIRE(m.size() == 0);
        m["spiderman"] = 43;
        REQUIRE(m.contains("spiderman"));
    }

    SECTION("move constructor alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            std::move(m), std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);

        REQUIRE(m.size() == 0);
        REQUIRE(m.bucket_count() == 0);
    }

    SECTION("initializer_list / bucket_count / hash / equal / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{{{"pikachu", 40}, {"raichu", 43}}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("pikachu"));
        REQUIRE(m1["pikachu"] == 40);

        jg::dense_hash_map<std::string, int> m2{{{"pikachu", 40}, {"raichu", 43}}, 32};
        REQUIRE(m2.size() == 2);
        REQUIRE(m2.bucket_count() == 32);
        REQUIRE(m2.contains("pikachu"));
        REQUIRE(m2["pikachu"] == 40);

        jg::dense_hash_map<std::string, int> m3{
            {{"pikachu", 40}, {"raichu", 43}}, 32, std::hash<std::string>{}};
        REQUIRE(m3.size() == 2);
        REQUIRE(m3.bucket_count() == 32);
        REQUIRE(m3.contains("raichu"));
        REQUIRE(m3["raichu"] == 43);

        jg::dense_hash_map<std::string, int> m4{{{"pikachu", 40}, {"raichu", 43}},
                                                6,
                                                std::hash<std::string>{},
                                                std::equal_to<std::string>{}};
        REQUIRE(m4.size() == 2);
        REQUIRE(m4.bucket_count() == 8);
        REQUIRE(m4.contains("pikachu"));
        REQUIRE(m4["pikachu"] == 40);

        jg::dense_hash_map<std::string, int> m5{
            {{"pikachu", 40}, {"raichu", 43}},
            0,
            std::hash<std::string>{},
            std::equal_to<std::string>{},
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m5.size() == 2);
        REQUIRE(m5.bucket_count() == 8);
        REQUIRE(m5.contains("pikachu"));
        REQUIRE(m5["pikachu"] == 40);
    }

    SECTION("initializer_list / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            {{"pikachu", 40}, {"raichu", 43}}, std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);
    }

    SECTION("initializer_list / bucket_count / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            {{"pikachu", 40}, {"raichu", 43}},
            8,
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("raichu"));
        REQUIRE(m1["raichu"] == 43);
    }

    SECTION("initializer_list / bucket_count / hash / alloc")
    {
        jg::dense_hash_map<std::string, int> m1{
            {{"pikachu", 40}, {"raichu", 43}},
            8,
            std::hash<std::string>{},
            std::allocator<std::pair<const std::string, int>>{}};
        REQUIRE(m1.size() == 2);
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.contains("pikachu"));
        REQUIRE(m1["pikachu"] == 40);
    }
}

TEST_CASE("assignment operator")
{
    jg::dense_hash_map<std::string, int> m1 = {{"pikachu", 40}, {"raichu", 43}};
    jg::dense_hash_map<std::string, int> m2 = {{"picsou", 13}};

    SECTION("copy")
    {
        m2 = m1;
        REQUIRE(m2.size() == 2);
        REQUIRE(m2.bucket_count() == 8);
        REQUIRE(m2.contains("pikachu"));
        REQUIRE(m2["pikachu"] == 40);
        REQUIRE(!m2.contains("picsou"));
    }

    SECTION("move")
    {
        REQUIRE(std::is_nothrow_move_assignable_v<decltype(m2)>);

        m2 = std::move(m1);
        REQUIRE(m2.size() == 2);
        REQUIRE(m2.bucket_count() == 8);
        REQUIRE(m2.contains("pikachu"));
        REQUIRE(m2["pikachu"] == 40);
        REQUIRE(!m2.contains("picsou"));

        REQUIRE(m1.size() == 0);
        REQUIRE(m1.bucket_count() == 0);

        // Moved dense_hash_map usable after clear.
        m1.clear();
        REQUIRE(m1.bucket_count() == 8);
        REQUIRE(m1.size() == 0);
        m1["spiderman"] = 43;
        REQUIRE(m1.contains("spiderman"));
    }

    SECTION("assign initializer_list")
    {
        m2 = {{"pikachu", 40}, {"raichu", 43}};
        REQUIRE(m2.size() == 2);
        REQUIRE(m2.bucket_count() == 8);
        REQUIRE(m2.contains("pikachu"));
        REQUIRE(m2["pikachu"] == 40);
        REQUIRE(!m2.contains("picsou"));
    }
}

TEST_CASE("iterator")
{
    jg::dense_hash_map<std::string, int> m = {{"pierre", 1}, {"paul", 2}, {"jacques", 3}};

    std::vector<std::pair<const std::string, int>> expected = {
        {"pierre", 1}, {"paul", 2}, {"jacques", 3}};

    SECTION("begin/end")
    {
        static_assert(std::is_same_v<decltype((m.begin()->second)), int&>);
        REQUIRE(std::equal(expected.begin(), expected.end(), m.begin(), m.end()));
    }

    SECTION("const begin/end")
    {
        const auto& m2 = m;
        static_assert(std::is_same_v<decltype((m2.begin()->second)), const int&>);
        REQUIRE(std::equal(expected.begin(), expected.end(), m2.begin(), m2.end()));
    }

    SECTION("cbegin/cend")
    {
        static_assert(std::is_same_v<decltype((m.cbegin()->second)), const int&>);
        REQUIRE(std::equal(expected.cbegin(), expected.cend(), m.cbegin(), m.cend()));
    }
}
TEST_CASE("clear")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("empty")
    {
        m.clear();
        REQUIRE(m.empty());
        REQUIRE(m.size() == 0);
        REQUIRE(m.bucket_count() == 8u);
        REQUIRE(m.load_factor() == 0.0f);
    }

    SECTION("not_empty")
    {
        m.try_emplace("sponge bob", 10);
        m.try_emplace("sponge bob2", 10);
        REQUIRE(m.size() == 2);

        m.clear();
        REQUIRE(m.empty());
        REQUIRE(m.size() == 0);
        REQUIRE(m.bucket_count() == 8u);
        REQUIRE(m.load_factor() == 0.0f);
    }

    SECTION("no_except") { REQUIRE(noexcept(m.clear())); }
}

template <class T, class V>
using has_insert = decltype(std::declval<T>().insert(std::declval<V>()));

template <class T, class V>
using has_insert_hint =
    decltype(std::declval<T>().insert(std::declval<T>().begin(), std::declval<V>()));

TEST_CASE("insert")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("insert - lvalue")
    {
        auto pair = std::pair(std::string("test"), 42);
        const auto& [it, result] = m.insert(pair);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - const lvalue")
    {
        const auto pair = std::pair(std::string("test"), 42);
        const auto& [it, result] = m.insert(pair);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - rvalue")
    {
        const auto& [it, result] = m.insert(std::pair(std::string("test"), 42));
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - conversion")
    {
        auto pair = std::pair("test", 42);
        const auto& [it, result] = m.insert(pair);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - conversion is SFINAE friendly")
    {
        REQUIRE_FALSE(jg::details::is_detected<
                      has_insert, jg::dense_hash_map<std::vector<int>, int>,
                      std::pair<bool, std::string>>::value);
    }

    SECTION("insert - lvalue - hint")
    {
        auto pair = std::pair(std::string("test"), 42);
        const auto& it = m.insert(m.begin(), pair);
        REQUIRE(it != m.end());
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - const lvalue - hint")
    {
        const auto pair = std::pair(std::string("test"), 42);
        const auto& it = m.insert(m.begin(), pair);
        REQUIRE(it != m.end());
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - rvalue - hint")
    {
        const auto& it = m.insert(m.begin(), std::pair(std::string("test"), 42));
        REQUIRE(it != m.end());
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - conversion - hint")
    {
        auto pair = std::pair("test", 42);
        const auto& it = m.insert(m.begin(), pair);
        REQUIRE(it != m.end());
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("insert - conversion is SFINAE friendly - hint")
    {
        REQUIRE_FALSE(jg::details::is_detected<
                      has_insert_hint, jg::dense_hash_map<std::vector<int>, int>,
                      std::pair<bool, std::string>>::value);
    }

    SECTION("insert - iterator")
    {
        std::vector<std::pair<std::string, int>> v{{"test", 42}, {"test2", 1337}};
        m.insert(v.begin(), v.end());

        REQUIRE(m.size() == 2);

        auto it = m.find("test");
        REQUIRE(it != m.end());
        REQUIRE(it->second == 42);

        it = m.find("test2");
        REQUIRE(it != m.end());
        REQUIRE(it->second == 1337);
    }

    SECTION("insert - initializer_list")
    {
        std::initializer_list<std::pair<const std::string, int>> l{{"test", 42}, {"test2", 1337}};
        m.insert(l);

        REQUIRE(m.size() == 2);

        auto it = m.find("test");
        REQUIRE(it != m.end());
        REQUIRE(it->second == 42);

        it = m.find("test2");
        REQUIRE(it != m.end());
        REQUIRE(it->second == 1337);
    }
}

TEST_CASE("insert_or_assign")
{
    jg::dense_hash_map<std::string, int> m1;
    jg::dense_hash_map<std::unique_ptr<int>, int> m2;

    SECTION("l-value key")
    {
        const auto& [it, result] = m1.insert_or_assign("test", 42);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);

        const auto& [it2, result2] = m1.insert_or_assign("test", 1337);
        REQUIRE_FALSE(result2);
        REQUIRE(it2 == it);
        REQUIRE(it2->first == "test");
        REQUIRE(it2->second == 1337);
    }

    SECTION("r-value key")
    {
        const auto& [it, result] = m2.insert_or_assign(nullptr, 42);
        REQUIRE(result);
        REQUIRE(it->first == nullptr);
        REQUIRE(it->second == 42);

        std::unique_ptr<int> p = nullptr;
        const auto& [it2, result2] = m2.insert_or_assign(std::move(p), 1337);
        REQUIRE_FALSE(result2);
        REQUIRE(it2 == it);
        REQUIRE(it2->first == nullptr);
        REQUIRE(it2->second == 1337);
    }

    SECTION("l-value key - hint")
    {
        auto it = m1.insert_or_assign(m1.begin(), "test", 42);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);

        auto it2 = m1.insert_or_assign(m1.begin(), "test", 1337);
        REQUIRE(it == it);
        REQUIRE(it2->first == "test");
        REQUIRE(it2->second == 1337);
    }

    SECTION("r-value key - hint")
    {
        auto it = m2.insert_or_assign(m2.begin(), nullptr, 42);
        REQUIRE(it->first == nullptr);
        REQUIRE(it->second == 42);

        std::unique_ptr<int> p = nullptr;
        auto it2 = m2.insert_or_assign(m2.begin(), std::move(p), 1337);
        REQUIRE(it2 == it);
        REQUIRE(it2->first == nullptr);
        REQUIRE(it2->second == 1337);
    }
}

TEST_CASE("emplace", "[emplace]")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("default")
    {
        const auto& [it, result] = m.emplace();
        REQUIRE(result);
        REQUIRE(it->first == std::string());
        REQUIRE(it->second == int{});
        REQUIRE(m.size() == 1);
    }

    SECTION("once - rvalues")
    {
        const auto& [it, result] = m.emplace("test", 42);

        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("once - l-values")
    {
        std::string key = "test";
        int value = 42;
        const auto& [it, result] = m.emplace(key, value);

        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("once - const l-values")
    {
        const std::string key = "test";
        const int value = 42;
        const auto& [it, result] = m.emplace(key, value);

        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("pair - lvalue")
    {
        auto pair = std::pair(std::string("test"), 42);
        const auto& [it, result] = m.emplace(pair);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("pair - const lvalue")
    {
        const auto pair = std::pair(std::string("test"), 42);
        const auto& [it, result] = m.emplace(pair);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("pair - rvalue")
    {
        const auto& [it, result] = m.emplace(std::pair(std::string("test"), 42));
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("conversion")
    {
        const auto& [it, result] = m.emplace("test", 42); // key: const char* ==> std::string
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("conversion pair")
    {
        const auto& [it, result] =
            m.emplace(std::pair("test", 42)); // key: const char* ==> std::string
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("piecewise_construct")
    {
        const auto& [it, result] = m.emplace(
            std::piecewise_construct, std::forward_as_tuple("test"), std::forward_as_tuple(42));
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }
}

TEST_CASE("emplace key rvalue")
{
    jg::dense_hash_map<std::unique_ptr<int>, int> m;

    SECTION("Successfull emplace")
    {
        auto ptr = std::make_unique<int>(37);
        const auto& [it, result] = m.emplace(std::move(ptr), 42);
        REQUIRE(result);
        REQUIRE(*it->first == 37);
        REQUIRE(it->second == 42);
        REQUIRE(ptr == nullptr);
    }
}

TEST_CASE("emplace twice", "[emplace]")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("twice same")
    {
        const auto& [it, result] = m.emplace("test", 42);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);

        const auto& [it2, result2] = m.emplace("test", 42);
        REQUIRE_FALSE(result2);
        REQUIRE(it == it2);
        REQUIRE(it2->first == "test");
        REQUIRE(it2->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("twice different")
    {
        const auto& [it, result] = m.emplace("test", 42);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);

        const auto& [it2, result2] = m.emplace("test2", 1337);
        REQUIRE(result2);
        REQUIRE(it != it2);
        REQUIRE(it2->first == "test2");
        REQUIRE(it2->second == 1337);
        REQUIRE(m.size() == 2);
    }
}

TEST_CASE("emplace optimization", "[emplace]")
{
    std::size_t counter{};

    jg::dense_hash_map<increase_counter_on_copy_or_move, int> m;
    const auto& [it, result] = m.emplace(&counter, 42);

    REQUIRE(result);
    REQUIRE(counter > 0);

    auto counter_after_insertion = counter;

    SECTION("key not copied if not inserted")
    {
        increase_counter_on_copy_or_move key{&counter};
        m.emplace(key, 42);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("pair's key copied moved if not inserted")
    {
        increase_counter_on_copy_or_move key{&counter};
        std::pair<increase_counter_on_copy_or_move&, int> p(key, 42);
        m.emplace(p);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("key not moved if not inserted")
    {
        increase_counter_on_copy_or_move key{&counter};
        m.emplace(std::move(key), 42);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("pair's key not moved if not inserted")
    {
        std::pair<increase_counter_on_copy_or_move, int> p(&counter, 42);
        m.emplace(std::move(p));

        REQUIRE(counter_after_insertion == counter);
    }
}

TEST_CASE("emplace_hint")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("one")
    {
        const auto& it = m.emplace_hint(m.begin(), "bob", 666);
        REQUIRE(it != m.end());
        REQUIRE(it->first == "bob");
        REQUIRE(it->second == 666);
        REQUIRE(m.size() == 1);
    }

    SECTION("twice")
    {
        const auto& it1 = m.emplace_hint(m.begin(), "bob", 666);
        const auto& it2 = m.emplace_hint(m.begin(), "bob", 444);
        REQUIRE(it1 == it2);
        REQUIRE(it2->first == "bob");
        REQUIRE(it2->second == 666);
        REQUIRE(m.size() == 1);
    }
}

TEST_CASE("try emplace", "[try_emplace]")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("rvalues")
    {
        const auto& [it, result] = m.try_emplace("test", 42);

        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("l-values")
    {
        std::string key = "test";
        int value = 42;
        const auto& [it, result] = m.try_emplace(key, value);

        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("const l-values")
    {
        const std::string key = "test";
        const int value = 42;
        const auto& [it, result] = m.try_emplace(key, value);

        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }
}

TEST_CASE("try_emplace twice", "[try_emplace]")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("twice same")
    {
        const auto& [it, result] = m.try_emplace("test", 42);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);

        const auto& [it2, result2] = m.try_emplace("test", 42);
        REQUIRE_FALSE(result2);
        REQUIRE(it == it2);
        REQUIRE(it2->first == "test");
        REQUIRE(it2->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("twice different")
    {
        const auto& [it, result] = m.try_emplace("test", 42);
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);

        const auto& [it2, result2] = m.try_emplace("test2", 1337);
        REQUIRE(result2);
        REQUIRE(it != it2);
        REQUIRE(it2->first == "test2");
        REQUIRE(it2->second == 1337);
        REQUIRE(m.size() == 2);
    }
}

TEST_CASE("try_emplace effects guarantees", "[try_emplace]")
{
    std::size_t counter{};

    jg::dense_hash_map<increase_counter_on_copy_or_move, int> m;
    const auto& [it, result] = m.emplace(&counter, 42);

    REQUIRE(result);
    REQUIRE(counter > 0);

    auto counter_after_insertion = counter;

    SECTION("key not copied if not inserted")
    {
        increase_counter_on_copy_or_move key{&counter};
        m.try_emplace(key, 42);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("key not moved if not inserted")
    {
        increase_counter_on_copy_or_move key{&counter};
        m.try_emplace(std::move(key), 42);

        REQUIRE(counter_after_insertion == counter);
    }
}

TEST_CASE("erase iterator", "[erase]")
{
    jg::dense_hash_map<std::string, int> m;
    const auto& [useless1, result1] = m.emplace("bob", 42);
    const auto& [useless2, result2] = m.emplace("jacky", 42);
    const auto& [useless3, result3] = m.emplace("snoop", 42);

    REQUIRE(result1);
    REQUIRE(result2);
    REQUIRE(result3);
    REQUIRE(m.find("bob") != m.end());
    REQUIRE(m.find("jacky") != m.end());
    REQUIRE(m.find("snoop") != m.end());
    REQUIRE(m.size() == 3);

    SECTION("using first iterator")
    {
        auto new_it = m.erase(m.begin());
        REQUIRE(new_it != m.end());
        REQUIRE(m.size() == 2);
        REQUIRE(new_it->first == "snoop");
        REQUIRE(m.find("bob") == m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") != m.end());
    }

    SECTION("using middle iterator")
    {
        auto it = m.find("jacky");
        REQUIRE(it != m.end());
        auto new_it = m.erase(it);
        REQUIRE(new_it != m.end());
        REQUIRE(m.size() == 2);
        REQUIRE(new_it->first == "snoop");
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") == m.end());
        REQUIRE(m.find("snoop") != m.end());
    }

    SECTION("using last iterator")
    {
        auto new_it = m.erase(std::prev(m.end()));
        REQUIRE(new_it == m.end());
        REQUIRE(m.size() == 2);
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") == m.end());
    }
}

TEST_CASE("erase key", "[erase]")
{
    jg::dense_hash_map<std::string, int> m;
    const auto& [useless1, result1] = m.emplace("bob", 42);
    const auto& [useless2, result2] = m.emplace("jacky", 42);
    const auto& [useless3, result3] = m.emplace("snoop", 42);

    REQUIRE(result1);
    REQUIRE(result2);
    REQUIRE(result3);
    REQUIRE(m.find("bob") != m.end());
    REQUIRE(m.find("jacky") != m.end());
    REQUIRE(m.find("snoop") != m.end());
    REQUIRE(m.size() == 3);

    SECTION("success")
    {
        REQUIRE(m.erase("bob") > 0);
        REQUIRE(m.size() == 2);
        REQUIRE(m.find("bob") == m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") != m.end());
    }

    SECTION("failure")
    {
        REQUIRE(m.erase("bobby") == 0);
        REQUIRE(m.size() == 3);
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") != m.end());
    }
}

TEST_CASE("erase with collisions", "[erase]")
{
    jg::dense_hash_map<std::string, int, collision_hasher> m;

    const auto& [useless1, result1] = m.emplace("bob", 42);
    const auto& [useless2, result2] = m.emplace("jacky", 42);
    const auto& [useless3, result3] = m.emplace("snoop", 42);

    REQUIRE(result1);
    REQUIRE(result2);
    REQUIRE(result3);
    auto bob_it = m.find("bob");
    auto jacky_it = m.find("jacky");
    auto snoop_it = m.find("snoop");
    REQUIRE(bob_it != m.end());
    REQUIRE(jacky_it != m.end());
    REQUIRE(snoop_it != m.end());
    REQUIRE(m.size() == 3);

    SECTION("remove first in bucket")
    {
        REQUIRE(m.erase("snoop") > 0);
        REQUIRE(m.size() == 2);
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") == m.end());
    }

    SECTION("remove mid in bucket")
    {
        REQUIRE(m.erase("jacky") > 0);
        REQUIRE(m.size() == 2);
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") == m.end());
        REQUIRE(m.find("snoop") != m.end());
    }

    SECTION("remove last in bucket")
    {
        REQUIRE(m.erase("bob") > 0);
        REQUIRE(m.size() == 2);
        REQUIRE(m.find("bob") == m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") != m.end());
    }
}

TEST_CASE("range erase")
{
    jg::dense_hash_map<std::string, int> m;
    const auto& [useless1, result1] = m.emplace("bob", 42);
    const auto& [useless2, result2] = m.emplace("jacky", 42);
    const auto& [useless3, result3] = m.emplace("snoop", 42);

    REQUIRE(result1);
    REQUIRE(result2);
    REQUIRE(result3);
    REQUIRE(m.find("bob") != m.end());
    REQUIRE(m.find("jacky") != m.end());
    REQUIRE(m.find("snoop") != m.end());
    REQUIRE(m.size() == 3);

    SECTION("all")
    {
        auto it = m.erase(m.begin(), m.end());
        REQUIRE(it == m.end());
        REQUIRE(m.size() == 0);
        REQUIRE(m.find("bob") == m.end());
        REQUIRE(m.find("jacky") == m.end());
        REQUIRE(m.find("snoop") == m.end());
    }

    SECTION("two first")
    {
        auto it = m.erase(m.begin(), std::prev(m.end()));
        REQUIRE(it->first == "snoop");
        REQUIRE(m.size() == 1);
        REQUIRE(m.find("bob") == m.end());
        REQUIRE(m.find("jacky") == m.end());
        REQUIRE(m.find("snoop") != m.end());
    }

    SECTION("two last")
    {
        auto it = m.erase(std::next(m.begin()), m.end());
        REQUIRE(it == m.end());
        REQUIRE(m.size() == 1);
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") == m.end());
        REQUIRE(m.find("snoop") == m.end());
    }

    SECTION("none")
    {
        auto it = m.erase(m.begin(), m.begin());
        REQUIRE(it == m.begin());
        REQUIRE(m.size() == 3);
        REQUIRE(m.find("bob") != m.end());
        REQUIRE(m.find("jacky") != m.end());
        REQUIRE(m.find("snoop") != m.end());
    }
}

TEST_CASE("rehash")
{
    jg::dense_hash_map<std::string, int> m;

    SECTION("by insertion")
    {
        const int amount = 1000;
        const auto old_bucket_count = m.bucket_count();

        for (int i = 0; i < amount; ++i)
        {
            m.emplace("test" + std::to_string(i), i);
        }

        REQUIRE(m.size() == amount);
        REQUIRE(m.bucket_count() != old_bucket_count);
        REQUIRE(m.bucket_count() >= std::ceil(1000 / m.max_load_factor()));

        for (int i = 0; i < amount; ++i)
        {
            const std::string key = "test" + std::to_string(i);
            auto it = m.find(key);
            REQUIRE(it != m.end());
            REQUIRE(it->first == key);
            REQUIRE(it->second == i);
        }
    }

    SECTION("rehash member function")
    {
        m.try_emplace("tarzan", 42);
        m.try_emplace("spirou", 1337);

        const auto old_load_factor = m.load_factor();

        m.rehash(1000);

        REQUIRE(m.load_factor() < old_load_factor);
        REQUIRE(m.bucket_count() == 1024);

        auto it = m.find("tarzan");
        REQUIRE(it != m.end());
        REQUIRE(it->first == "tarzan");
        REQUIRE(it->second == 42);

        it = m.find("spirou");
        REQUIRE(it != m.end());
        REQUIRE(it->first == "spirou");
        REQUIRE(it->second == 1337);
    }

    SECTION("reserve")
    {
        m.try_emplace("tarzan", 42);
        m.try_emplace("spirou", 1337);

        const auto old_load_factor = m.load_factor();

        m.reserve(1000);

        REQUIRE(m.load_factor() < old_load_factor);
        REQUIRE(m.bucket_count() >= std::ceil(1000 / m.max_load_factor()));

        auto it = m.find("tarzan");
        REQUIRE(it != m.end());
        REQUIRE(it->first == "tarzan");
        REQUIRE(it->second == 42);

        it = m.find("spirou");
        REQUIRE(it != m.end());
        REQUIRE(it->first == "spirou");
        REQUIRE(it->second == 1337);
    }

    SECTION("max_load_factor")
    {
        m.try_emplace("tarzan", 42);
        m.try_emplace("spirou", 1337);

        REQUIRE(m.bucket_count() == 8);
        REQUIRE(m.load_factor() == 0.25); // We have 2/8 == 0.25 load factor.

        m.max_load_factor(0.2f); // So this should trigger a regrowth.

        REQUIRE(m.max_load_factor() == 0.2f);
        REQUIRE(m.bucket_count() == 16); // Doubling the size will work.

        auto it = m.find("tarzan");
        REQUIRE(it != m.end());
        REQUIRE(it->first == "tarzan");
        REQUIRE(it->second == 42);

        it = m.find("spirou");
        REQUIRE(it != m.end());
        REQUIRE(it->first == "spirou");
        REQUIRE(it->second == 1337);
    }
}

TEST_CASE("swap", "[swap]")
{
    jg::dense_hash_map<std::string, int> m1 = {{"batman", 42}, {"robin", 666}};
    jg::dense_hash_map<std::string, int> m2 = {{"superman", 64}};

    SECTION("member")
    {
        m1.swap(m2);

        REQUIRE(m1.size() == 1);
        REQUIRE(m2.size() == 2);

        auto it = m1.find("superman");
        REQUIRE(it != m1.end());
        REQUIRE(it->first == "superman");
        REQUIRE(it->second == 64);
        it = m1.find("batman");
        REQUIRE(it == m1.end());

        it = m2.find("robin");
        REQUIRE(it != m2.end());
        REQUIRE(it->first == "robin");
        REQUIRE(it->second == 666);
    }

    SECTION("std::swap")
    {
        REQUIRE(std::is_nothrow_swappable_v<decltype(m1)>);

        std::swap(m1, m2);

        REQUIRE(m1.size() == 1);
        REQUIRE(m2.size() == 2);

        auto it = m1.find("superman");
        REQUIRE(it != m1.end());
        REQUIRE(it->first == "superman");
        REQUIRE(it->second == 64);
        it = m1.find("batman");
        REQUIRE(it == m1.end());

        it = m2.find("robin");
        REQUIRE(it != m2.end());
        REQUIRE(it->first == "robin");
        REQUIRE(it->second == 666);
    }
}

TEST_CASE("swap allocator", "[swap]")
{
    SECTION("no swap")
    {
        using alloc = named_allocator<int>;
        jg::dense_hash_map<int, int, std::hash<int>, std::equal_to<int>, alloc> m1{alloc{"a1"}};
        jg::dense_hash_map<int, int, std::hash<int>, std::equal_to<int>, alloc> m2{alloc{"a2"}};

        REQUIRE(m1.get_allocator().name == "a1");
        REQUIRE(m2.get_allocator().name == "a2");

        std::swap(m1, m2);

        REQUIRE(m1.get_allocator().name == "a1");
        REQUIRE(m2.get_allocator().name == "a2");
    }

    SECTION("swap")
    {
        using alloc = named_allocator<int, std::true_type>;
        jg::dense_hash_map<int, int, std::hash<int>, std::equal_to<int>, alloc> m1{alloc{"a1"}};
        jg::dense_hash_map<int, int, std::hash<int>, std::equal_to<int>, alloc> m2{alloc{"a2"}};

        REQUIRE(m1.get_allocator().name == "a1");
        REQUIRE(m2.get_allocator().name == "a2");

        std::swap(m1, m2);

        REQUIRE(m1.get_allocator().name == "a2");
        REQUIRE(m2.get_allocator().name == "a1");
    }
}

TEST_CASE("at")
{
    jg::dense_hash_map<std::string, int> m1 = {{"batman", 42}, {"robin", 666}};

    SECTION("non-const")
    {
        static_assert(std::is_same_v<decltype(m1.at("")), int&>);
        REQUIRE(m1.at("batman") == 42);

        m1.at("batman") = 1337;

        REQUIRE(m1.at("batman") == 1337);

#if !JG_NO_EXCEPTION
        REQUIRE_THROWS(m1.at("winnie"));
#endif
    }

    SECTION("const")
    {
        const auto& m2 = m1;
        static_assert(std::is_same_v<decltype(m2.at("")), const int&>);
        REQUIRE(m2.at("batman") == 42);

#if !JG_NO_EXCEPTION
        REQUIRE_THROWS(m1.at("the pooh"));
#endif
    }
}

TEST_CASE("subscript operator")
{
    jg::dense_hash_map<std::string, int> m1 = {};

    SECTION("copy-key")
    {
        std::string key = "RPC";
        m1[key] = 4;
        REQUIRE(m1.size() == 1);
        auto it = m1.find("RPC");
        REQUIRE(it != m1.end());
        REQUIRE(it->second == 4);
    }

    SECTION("move-key")
    {
        std::string key = "PRC";
        m1[std::move(key)] = 748;
        REQUIRE(m1.size() == 1);
        auto it = m1.find("PRC");
        REQUIRE(it != m1.end());
        REQUIRE(it->second == 748);
        REQUIRE(key.empty());
    }

    SECTION("twice")
    {
        m1["CRP"] = 1;
        REQUIRE(m1.size() == 1);
        auto it = m1.find("CRP");
        REQUIRE(it != m1.end());
        REQUIRE(it->second == 1);

        m1["CRP"] = 2;
        REQUIRE(it->second == 2);
    }
}

TEST_CASE("count simple hash", "[count]")
{
    jg::dense_hash_map<std::string, int> m1 = {};

    SECTION("key")
    {
        REQUIRE(m1.count("queen") == 0);

        m1.try_emplace("queen", 42);
        REQUIRE(m1.count("queen") == 1);
    }

    SECTION("no generic count")
    {
        auto v = is_valid([](const auto& m) -> decltype(m.count(nested_string{"queen"})) {});
        REQUIRE_FALSE(v(m1));
    }
}

TEST_CASE("count transparent hash", "[count]")
{
    jg::dense_hash_map<std::string, int, string_hash> m1 = {};

    SECTION("key")
    {
        REQUIRE(m1.count(nested_string{"pink floyd"}) == 0);

        m1.try_emplace("pink floyd", 42);
        REQUIRE(m1.count(nested_string{"pink floyd"}) == 1);
    }
}

TEST_CASE("find simple hash", "[find]")
{
    jg::dense_hash_map<std::string, int> m1 = {};

    SECTION("non-const")
    {
        auto it = m1.find("queen");
        REQUIRE(it == m1.end());

        m1.try_emplace("queen", 42);
        it = m1.find("queen");
        REQUIRE(it != m1.end());
        REQUIRE(it->second == 42);
        static_assert(std::is_same_v<decltype((it->second)), int&>);
    }

    SECTION("const")
    {
        const auto& m2 = m1;
        auto it = m2.find("queen");
        REQUIRE(it == m2.end());

        m1.try_emplace("queen", 42);
        it = m2.find("queen");
        REQUIRE(it != m2.end());
        REQUIRE(it->second == 42);
        static_assert(std::is_same_v<decltype((it->second)), const int&>);
    }

    SECTION("no generic find")
    {
        auto v = is_valid([](const auto& m) -> decltype(m.find(nested_string{"queen"})) {});
        REQUIRE_FALSE(v(m1));
    }
}

TEST_CASE("find transparent hash", "[find]")
{
    jg::dense_hash_map<std::string, int, string_hash> m1 = {};

    SECTION("non-const")
    {
        auto it = m1.find(nested_string{"queen"});
        REQUIRE(it == m1.end());

        m1.try_emplace("queen", 42);
        it = m1.find(nested_string{"queen"});
        REQUIRE(it != m1.end());
        REQUIRE(it->second == 42);
        static_assert(std::is_same_v<decltype((it->second)), int&>);
    }

    SECTION("const")
    {
        const auto& m2 = m1;
        auto it = m2.find(nested_string{"queen"});
        REQUIRE(it == m2.end());

        m1.try_emplace("queen", 42);
        it = m2.find(nested_string{"queen"});
        REQUIRE(it != m2.end());
        REQUIRE(it->second == 42);
        static_assert(std::is_same_v<decltype((it->second)), const int&>);
    }
}

TEST_CASE("contains simple hash", "[contains]")
{
    jg::dense_hash_map<std::string, int> m1 = {};

    SECTION("key")
    {
        REQUIRE_FALSE(m1.contains("queen"));

        m1.try_emplace("queen", 42);
        REQUIRE(m1.contains("queen"));
    }

    SECTION("no generic contains")
    {
        auto v = is_valid([](const auto& m) -> decltype(m.contains(nested_string{"queen"})) {});
        REQUIRE_FALSE(v(m1));
    }
}

TEST_CASE("contains transparent hash", "[contains]")
{
    jg::dense_hash_map<std::string, int, string_hash> m1 = {};

    SECTION("key")
    {
        REQUIRE_FALSE(m1.contains(nested_string{"pink floyd"}));

        m1.try_emplace("pink floyd", 42);
        REQUIRE(m1.contains(nested_string{"pink floyd"}));
    }
}

TEST_CASE("equal_range simple hash", "[equal_range]")
{
    jg::dense_hash_map<std::string, int> m1 = {};

    SECTION("non-const")
    {
        auto [it, end] = m1.equal_range("queen");
        REQUIRE(it == m1.end());
        REQUIRE(end == m1.end());

        m1.try_emplace("queen", 42);
        auto [it2, end2] = m1.equal_range("queen");
        REQUIRE(it2 != m1.end());
        REQUIRE(it2->second == 42);
        REQUIRE(std::distance(it2, end2) == 1);
        static_assert(std::is_same_v<decltype((it2->second)), int&>);
    }

    SECTION("const")
    {
        const auto& m2 = m1;
        auto [it, end] = m2.equal_range("queen");
        REQUIRE(it == m2.end());
        REQUIRE(end == m1.end());

        m1.try_emplace("queen", 42);
        auto [it2, end2] = m2.equal_range("queen");
        REQUIRE(it2 != m2.end());
        REQUIRE(it2->second == 42);
        REQUIRE(std::distance(it2, end2) == 1);
        static_assert(std::is_same_v<decltype((it2->second)), const int&>);
    }

    SECTION("no generic equal_range")
    {
        auto v = is_valid([](const auto& m) -> decltype(m.equal_range(nested_string{"queen"})) {});
        REQUIRE_FALSE(v(m1));
    }
}

TEST_CASE("equal_range transparent hash", "[equal_range]")
{
    jg::dense_hash_map<std::string, int, string_hash> m1 = {};

    SECTION("non-const")
    {
        auto [it, end] = m1.equal_range(nested_string{"queen"});
        REQUIRE(it == m1.end());
        REQUIRE(end == m1.end());

        m1.try_emplace("queen", 42);
        auto [it2, end2] = m1.equal_range(nested_string{"queen"});
        REQUIRE(it2 != m1.end());
        REQUIRE(it2->second == 42);
        REQUIRE(std::distance(it2, end2) == 1);
        static_assert(std::is_same_v<decltype((it2->second)), int&>);
    }

    SECTION("const")
    {
        const auto& m2 = m1;
        auto [it, end] = m2.equal_range(nested_string{"queen"});
        REQUIRE(it == m2.end());

        m1.try_emplace("queen", 42);
        auto [it2, end2] = m2.equal_range(nested_string{"queen"});
        REQUIRE(it2 != m2.end());
        REQUIRE(it2->second == 42);
        REQUIRE(std::distance(it2, end2) == 1);
        static_assert(std::is_same_v<decltype((it2->second)), const int&>);
    }
}

TEST_CASE("bucket iterator")
{
    jg::dense_hash_map<std::string, int, collision_hasher> m = {
        {"pierre", 1}, {"paul", 2}, {"jacques", 3}};

    std::vector<std::pair<const std::string, int>> expected = {
        {"jacques", 3}, {"paul", 2}, {"pierre", 1}};

    SECTION("begin/end")
    {
        static_assert(std::is_same_v<decltype((m.begin(0u)->second)), int&>);
        REQUIRE(std::equal(expected.begin(), expected.end(), m.begin(0), m.end(0)));
    }

    SECTION("const begin/end")
    {
        const auto& m2 = m;
        static_assert(std::is_same_v<decltype((m2.begin(0u)->second)), const int&>);
        REQUIRE(std::equal(expected.begin(), expected.end(), m2.begin(0), m2.end(0)));
    }

    SECTION("cbegin/cend")
    {
        static_assert(std::is_same_v<decltype((m.cbegin(0u)->second)), const int&>);
        REQUIRE(std::equal(expected.cbegin(), expected.cend(), m.cbegin(0), m.cend(0)));
    }
}

TEST_CASE("bucket_count")
{
    jg::dense_hash_map<int, int> m;

    const auto& m2 = m;
    REQUIRE(m2.bucket_count() > 0);
    REQUIRE(m2.bucket_count() < m2.max_bucket_count());
}

TEST_CASE("bucket index")
{
    jg::dense_hash_map<std::string, int, collision_hasher> m = {
        {"pierre", 1}, {"paul", 2}, {"jacques", 3}};

    REQUIRE(m.bucket("pierre") == 0);
    REQUIRE(m.bucket("paul") == 0);
    REQUIRE(m.bucket("jacques") == 0);
}

TEST_CASE("load_factor")
{
    jg::dense_hash_map<std::string, int> m = {{"pierre", 1}, {"paul", 2}, {"jacques", 3}};
    const auto& cm = m;

    REQUIRE(cm.load_factor() < cm.max_load_factor());
    REQUIRE(cm.load_factor() < cm.max_load_factor());
}

TEST_CASE("observers")
{
    const jg::dense_hash_map<std::string, int> m = {};

    SECTION("hash_function")
    {
        REQUIRE(std::is_same_v<decltype(m.hash_function()), std::hash<std::string>>);
    }

    SECTION("key_eq") { REQUIRE(std::is_same_v<decltype(m.key_eq()), std::equal_to<std::string>>); }
}

TEST_CASE("comparison")
{
    jg::dense_hash_map<std::string, int> m1 = {{"pierre", 1}, {"paul", 2}, {"jacques", 3}};

    SECTION("equal")
    {
        auto m2 = m1;
        REQUIRE(m2 == m1);
        REQUIRE_FALSE(m2 != m1);
    }

    SECTION("not the same value")
    {
        auto m2 = m1;
        m2["pierre"] = 42;
        REQUIRE_FALSE(m2 == m1);
        REQUIRE(m2 != m1);
    }

    SECTION("extra entry")
    {
        auto m2 = m1;
        m2["santa"] = 666;
        REQUIRE_FALSE(m2 == m1);
        REQUIRE(m2 != m1);
    }
}

TEST_CASE("erase_if")
{
    jg::dense_hash_map<std::string, int> m1 = {{"tintin", 42}, {"milou", 666}, {"haddock", 13}};

    SECTION("on value")
    {
        std::erase_if(m1, [](auto& pair) {
            if (pair.second > 40)
            {
                return true;
            }

            return false;
        });

        std::vector<std::pair<const std::string, int>> expected = {{"haddock", 13}};

        REQUIRE(std::equal(m1.begin(), m1.end(), expected.begin(), expected.end()));
    }

    SECTION("on key")
    {
        std::erase_if(m1, [](auto& pair) {
            if (pair.first[0] == 'h')
            {
                return true;
            }

            return false;
        });

        std::vector<std::pair<const std::string, int>> expected = {{"tintin", 42}, {"milou", 666}};

        REQUIRE(std::equal(m1.begin(), m1.end(), expected.begin(), expected.end()));
    }
}

TEST_CASE("deduction guides")
{
    jg::dense_hash_map<std::string, int> m;

    jg::dense_hash_map m1(m.begin(), m.end());
    REQUIRE(std::is_same_v<decltype(m1), jg::dense_hash_map<std::string, int>>);

    jg::dense_hash_map m2({std::pair{"foo", 2}, {"bar", 3}});
    REQUIRE(std::is_same_v<decltype(m2), jg::dense_hash_map<const char*, int>>);

    jg::dense_hash_map m3(m.begin(), m.end(), 42u, named_allocator<const char*>{"test"});
    REQUIRE(std::is_same_v<
            decltype(m3), jg::dense_hash_map<
                              std::string, int, std::hash<std::string>, std::equal_to<std::string>,
                              named_allocator<const char*>>>);

    // Rule 4 works thanks to https://cplusplus.github.io/LWG/issue2713
    jg::dense_hash_map m4(m.begin(), m.end(), named_allocator<const char*>{"test"});
    REQUIRE(std::is_same_v<
            decltype(m4), jg::dense_hash_map<
                              std::string, int, std::hash<std::string>, std::equal_to<std::string>,
                              named_allocator<const char*>>>);

    jg::dense_hash_map m5(
        m.begin(), m.end(), 42u, collision_hasher{}, named_allocator<const char*>{"test"});
    REQUIRE(std::is_same_v<
            decltype(m5), jg::dense_hash_map<
                              std::string, int, collision_hasher, std::equal_to<std::string>,
                              named_allocator<const char*>>>);

    jg::dense_hash_map m6(
        {std::pair{"foo", 2}, {"bar", 3}}, 42u, named_allocator<std::string>{"test"});
    REQUIRE(std::is_same_v<
            decltype(m6), jg::dense_hash_map<
                              const char*, int, std::hash<const char*>, std::equal_to<const char*>,
                              named_allocator<std::string>>>);

    // Rule 7 also works thanks to https://cplusplus.github.io/LWG/issue2713
    jg::dense_hash_map m7({std::pair{"foo", 2}, {"bar", 3}}, named_allocator<std::string>{"test"});
    REQUIRE(std::is_same_v<
            decltype(m7), jg::dense_hash_map<
                              const char*, int, std::hash<const char*>, std::equal_to<const char*>,
                              named_allocator<std::string>>>);

    jg::dense_hash_map m8(
        {std::pair{"foo", 2}, {"bar", 3}}, 42u, collision_hasher{},
        named_allocator<std::string>{"test"});
    REQUIRE(std::is_same_v<
            decltype(m7), jg::dense_hash_map<
                              const char*, int, std::hash<const char*>, std::equal_to<const char*>,
                              named_allocator<std::string>>>);
}

TEST_CASE("allocator propagation")
{
    int counter = 0;
    auto r = counting_pmr_resource(&counter);

    jg::pmr::dense_hash_map<std::pmr::string, std::pmr::string> m(8u, &r);
    m.reserve(100);

    int old_counter_value =
        counter; // buckets_ creation + reserve of buckets_ and nodes_ == 3 allocations
    REQUIRE(counter >= 3);

    SECTION("nested other container")
    {
        m.try_emplace(
            "a_super_long_string_to_disable_short_string_optimization",
            "a_super_long_string_to_disable_short_string_optimization");

        REQUIRE((counter - old_counter_value) == 2); // two strings == 2 allocations.
        REQUIRE(
            m["a_super_long_string_to_disable_short_string_optimization"] ==
            "a_super_long_string_to_disable_short_string_optimization");

        std::pmr::string s("lalalalalahihohohohohohohohohoho", &r);

        m.insert_or_assign(
            "a_super_long_string_to_disable_short_string_optimization", std::move(s));
        REQUIRE(s.empty());
    }

    SECTION("nested dense_hash_map")
    {
        jg::pmr::dense_hash_map<
            std::string, jg::pmr::dense_hash_map<std::pmr::string, std::pmr::string>>
            m2(8u, &r);  // one alloc
        m2.reserve(100); // 2 allocs for reserve of buckets_ and nodes_

        m2.try_emplace("test", m); // 1 alloc for the copy of buckets_ from m. nodes_ is not copied
                                   // as empty... so no allocation.

        REQUIRE((counter - old_counter_value) == 4); // string + dense_hash_map == 2 allocations.
    }
}

TEST_CASE("Move only types")
{
    struct key
    {
        std::string value;

        key(std::string value) : value(std::move(value)) {}

        key(const key& other) = delete;

        key(key&& other) : value(std::move(other.value)) {}

        auto operator=(const key& other) -> key& = default;

        auto operator=(key&& other) -> key&
        {
            value = std::move(other.value);
            return *this;
        }

        auto operator==(const key& other) const -> bool { return other.value == value; }
    };

    struct key_hasher
    {
        auto operator()(const key& k) const noexcept -> std::size_t
        {
            return std::hash<std::string>()(k.value);
        }
    };

    jg::dense_hash_map<key, std::unique_ptr<std::string>, key_hasher> m{};

    m.emplace(std::pair{key{"test1"}, std::make_unique<std::string>("1")});
    m.try_emplace(key{"test2"}, std::make_unique<std::string>("2"));

    REQUIRE(m.contains(key{"test1"}));
    auto it = m.find(key{"test1"});
    REQUIRE(it != m.end());
    REQUIRE(*it->second == "1");
    REQUIRE(m.contains(key{"test2"}));

    REQUIRE(m[key{"test2"}]);

    m[key{"test3"}] = std::make_unique<std::string>("3");
    it = m.find(key{"test3"});
    REQUIRE(it != m.end());
    REQUIRE(*it->second == "3");

    m[key{"test1"}] = std::make_unique<std::string>("11");
    it = m.find(key{"test1"});
    REQUIRE(*it->second == "11");

    m.erase(key{"test3"});

    REQUIRE_FALSE(m.contains(key{"test3"}));

    jg::dense_hash_map<key, std::unique_ptr<std::string>, key_hasher> m2{};
    m2 = std::move(m);

    REQUIRE(m2.size() == 2);
    REQUIRE(m.size() == 0);
    REQUIRE(m != m2);

    std::swap(m2, m);
    REQUIRE(m2.size() == 0);
    REQUIRE(m.size() == 2);
    REQUIRE(m != m2);
}

TEST_CASE("growth policy")
{
    struct dumb_growth_policy
    {
        static constexpr auto compute_index(std::size_t hash, std::size_t capacity) -> std::size_t
        {
            return hash % capacity;
        }

        static constexpr auto compute_closest_capacity(std::size_t min_capacity) -> std::size_t
        {
            return min_capacity;
        }

        static constexpr auto minimum_capacity() -> std::size_t { return 2u; }
    };

    jg::dense_hash_map<
        int, int, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<const int, int>>,
        dumb_growth_policy>
        m{};

    REQUIRE(m.bucket_count() == 2);

    m.try_emplace(1, 1);
    m.try_emplace(2, 2);

    REQUIRE(m.bucket_count() == 4);

    m.rehash(500);
    REQUIRE(m.bucket_count() == 500);
}
