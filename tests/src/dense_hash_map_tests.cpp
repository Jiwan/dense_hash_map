#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file

#include "catch2/catch.hpp"
#include "jg/dense_hash_map.hpp"
#include "jg/details/type_traits.hpp"

#include <string>
#include <vector>

namespace {
struct increase_counter_on_copy_or_move
{
    increase_counter_on_copy_or_move(std::size_t* counter_ptr)
        : counter_ptr{counter_ptr}
    {
    }

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

    auto operator=(const increase_counter_on_copy_or_move& other) -> increase_counter_on_copy_or_move&
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

auto operator==(const increase_counter_on_copy_or_move&, const increase_counter_on_copy_or_move&) -> bool
{
    return true;
}
}

namespace std {
    template<>
    struct hash<increase_counter_on_copy_or_move> {
        auto operator()(const increase_counter_on_copy_or_move& ) const noexcept -> std::size_t
        {
            return 0;
        }
    };
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

    SECTION("no_except")
    {
        REQUIRE(noexcept(m.clear()));
    }
}

template <class T, class V>
using has_insert = decltype(std::declval<T>().insert(std::declval<V>()));

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
        REQUIRE_FALSE(jg::details::is_detected<has_insert, jg::dense_hash_map<std::vector<int>, int>, std::pair<bool, std::string>>::value);
    }

    // TODO: all other insert functions.
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
        const auto& [it, result] = m.emplace(std::pair("test", 42)); // key: const char* ==> std::string
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
    }

    SECTION("piecewise_construct")
    {
        const auto& [it, result] = m.emplace(std::piecewise_construct, std::forward_as_tuple("test"), std::forward_as_tuple(42));
        REQUIRE(result);
        REQUIRE(it->first == "test");
        REQUIRE(it->second == 42);
        REQUIRE(m.size() == 1);
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
        increase_counter_on_copy_or_move key {&counter};
        m.emplace(key, 42);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("pair's key copied moved if not inserted")
    {
        increase_counter_on_copy_or_move key {&counter};
        std::pair<increase_counter_on_copy_or_move&, int> p(key, 42);
        m.emplace(p);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("key not moved if not inserted")
    {
        increase_counter_on_copy_or_move key {&counter};
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
        increase_counter_on_copy_or_move key {&counter};
        m.try_emplace(key, 42);

        REQUIRE(counter_after_insertion == counter);
    }

    SECTION("key not moved if not inserted")
    {
        increase_counter_on_copy_or_move key {&counter};
        m.try_emplace(std::move(key), 42);

        REQUIRE(counter_after_insertion == counter);
    }
}

TEST_CASE("Insertions trigger a rehash")
{

}

TEST_CASE("Move only types")
{
}

TEST_CASE("rehash")
{

}
