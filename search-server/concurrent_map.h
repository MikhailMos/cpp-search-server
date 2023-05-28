#pragma once

#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Buckets {
        std::mutex m;
        std::map<Key, Value> part;
    };

    struct Access {
        Access(const Key& key, Buckets& bucket)
            : guard(bucket.m)
            , ref_to_value(bucket.part[key])
        {}

        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : bucket_count_(bucket_count)
        , vec_buckets_(bucket_count) {}

    Access operator[](const Key& key) {
        size_t index_bucket = GetIndexBucket(key);
        return { key, vec_buckets_[index_bucket] };
    }
    
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> map_result;

        for (Buckets& part_of_map : vec_buckets_) {
            std::lock_guard<std::mutex> g(part_of_map.m);
            map_result.insert(part_of_map.part.begin(), part_of_map.part.end());
        }

        return map_result;
    }

    size_t Erase(const Key& key) {
        size_t index_bucket = GetIndexBucket(key);
        return (vec_buckets_[index_bucket].part.erase(key));
    }

private:
    size_t bucket_count_;
    std::vector<Buckets> vec_buckets_;

    // Возвращает индек "корзины" для конкретного ключа
    size_t GetIndexBucket(const Key& key) const {
        return (static_cast<uint64_t>(key) % bucket_count_);
    }

};