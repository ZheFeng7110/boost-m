// mcpplibs.mwrap.boost.bimap — user types on both sides: a user key with its
// own ordering and hashing, user comparators and hashers passed to the set
// types, user tags, user info payloads, modify/replace with user functors.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.bimap;
import mcpplibs.mwrap.boost.container_hash;

using namespace boost::bimaps;

namespace user {
struct sku {
    std::string code;
    auto operator<=>(const sku&) const = default;
    friend std::size_t hash_value(const sku& s) { return boost::hash<std::string>()(s.code); }
};
struct by_length {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    }
};
struct ci_hash {
    std::size_t operator()(const std::string& s) const {
        std::string l(s);
        for (auto& c : l) c = char(std::tolower(c));
        return std::hash<std::string>()(l);
    }
};
struct ci_equal {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                   return std::tolower(x) == std::tolower(y);
               });
    }
};
struct product {};
struct shelf {};
struct stock {
    int count = 0;
};
}  // namespace user

TEST_CASE("user key types with operator< and hash_value") {
    bimap<set_of<user::sku>, unordered_set_of<user::sku>> bm;
    bm.insert({user::sku{"b"}, user::sku{"B"}});
    bm.insert({user::sku{"a"}, user::sku{"A"}});
    CHECK(bm.left.begin()->first.code == "a");
    CHECK(bm.right.at(user::sku{"B"}).code == "b");
}

TEST_CASE("user comparator and hasher in the set types") {
    bimap<set_of<std::string, user::by_length>, unordered_set_of<std::string, user::ci_hash, user::ci_equal>> bm;
    bm.insert({"ccc", "X"});
    bm.insert({"a", "y"});
    CHECK(bm.left.begin()->first == "a");
    CHECK(bm.right.count("x") == 1);  // case-insensitive lookup
    CHECK_FALSE(bm.insert({"zz", "Y"}).second);
}

TEST_CASE("user tags and info payload") {
    using inventory = bimap<tagged<std::string, user::product>, tagged<int, user::shelf>, with_info<user::stock>>;
    inventory inv;
    inv.insert(inventory::value_type("bolt", 7, user::stock{40}));
    auto it = inv.by<user::product>().find("bolt");
    REQUIRE(it != inv.by<user::product>().end());
    it->info.count -= 5;
    CHECK(inv.by<user::shelf>().find(7)->info.count == 35);
}

TEST_CASE("modify and replace with user functors") {
    using bm_type = bimap<int, std::string>;
    bm_type bm;
    bm.insert(bm_type::value_type(1, "one"));
    bm.insert(bm_type::value_type(2, "two"));
    struct shout {
        void operator()(std::string& s) const {
            for (auto& c : s) c = char(std::toupper(c));
        }
    };
    auto it = bm.left.find(1);
    CHECK(bm.left.modify_data(it, shout()));
    CHECK(bm.left.at(1) == "ONE");
    CHECK(bm.left.replace_key(it, 10));
    CHECK(bm.left.count(10) == 1);
    CHECK_FALSE(bm.left.replace_data(bm.left.find(2), "ONE"));  // would collide on the right
    CHECK(bm.left.modify_key(bm.left.find(2), _key = 20));
    CHECK(bm.left.count(20) == 1);
}
