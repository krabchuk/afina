#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <unordered_map>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementation!!
 */
class SimpleLRU : public Afina::Storage {
public:
    explicit SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {}

    ~SimpleLRU() override {
        _lru_index.clear();
        if (_lru_head) {
          while (_lru_tail->_prev) {
            _lru_tail = _lru_tail->_prev;
            _lru_tail->_next.reset();
          }
          _lru_head.reset();
        }
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
  struct lru_node {
    lru_node *_prev = nullptr;
    std::unique_ptr<lru_node> _next;

    const std::string _key;
    std::string _value;

    lru_node(const std::string &key, const std::string &value) : _key (key), _value (value) {}
  };

  // Maximum number of bytes could be stored in this cache.
  // i.e all (keys+values) must be less the _max_size
  std::size_t _max_size;

  // Actual number of bytes stored in this cache
  // Always less than _max_size
  std::size_t _actual_size = 0;

  // Main data index for fast search
  std::unordered_map<std::reference_wrapper<const std::string>,
                     std::reference_wrapper<lru_node>,
                     std::hash<std::string>,
                     std::equal_to<const std::string>> _lru_index;

  // Data storage.
  // New elements go to head
  mutable std::unique_ptr<lru_node> _lru_head;
  mutable lru_node *_lru_tail = nullptr;

  bool PutItem(const std::string &key, const std::string &value);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
