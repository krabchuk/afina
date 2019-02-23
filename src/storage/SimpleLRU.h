#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {}

    ~SimpleLRU() {
        _lru_index.clear();
        _lru_head.reset(); // TODO: Here is stack overflow
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) const override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        lru_node(const std::string &key_arg, const std::string &value_arg, lru_node *prev_arg = nullptr);

        std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;

    // Actual number of bytes stored in this cache
    // Always less than _max_size
    std::size_t _actual_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<const std::reference_wrapper<const std::string>,
             const std::reference_wrapper<lru_node>> _lru_index;

    void FreeSpaceForNewItem (std::size_t additional_size);
    void RemoveOldestItem();
    bool PutItem(const std::string &key, const std::string &value);
    lru_node *StorageSearch(const std::string &key);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
