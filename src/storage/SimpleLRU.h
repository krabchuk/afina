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
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {
      _lru_history.reset(new lru_deque ());
    }

    ~SimpleLRU() {
        _lru_history.reset();
        _lru_storage.clear();
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
    struct lru_deque {
      struct lru_node {
        lru_node *_prev = nullptr;
        std::unique_ptr<lru_node> _next;

        std::string _key;
        std::string _value;

        lru_node(const std::string &key, const std::string &value) {
          _key = key;
          _value = value;
          _prev = nullptr;
        }
        lru_node(const lru_node &node) {}
        ~lru_node() {}
      };

      ~lru_deque() {
        if (_head) {
          while (_tail->_prev) {
            _tail = _tail->_prev;
            _tail->_next.reset();
          }
          _head.reset();
        }

      }

      std::unique_ptr<lru_node> _head;
      lru_node *_tail = nullptr;

      void pop_back();
      void push_front(const std::string &key, const std::string &value);
      void erase(const std::string &key);
      const std::string &back() const { return _tail->_key; }
      const std::string &front() const { return _head->_key; }
      lru_node &head() const { return *_head; }
      void update(const std::string &key, const std::string &value);

      void print();

      lru_node *find(lru_node *first, const std::string &key) const;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;

    // Actual number of bytes stored in this cache
    // Always less than _max_size
    std::size_t _actual_size = 0;

    // Main data storage
    std::unordered_map<std::reference_wrapper<const std::string>,
                       std::reference_wrapper<lru_deque::lru_node>,
                       std::hash<std::string>,
                       std::equal_to<const std::string>> _lru_storage;

    // Usage history storage.
    // New elements go to head
    std::unique_ptr<lru_deque> _lru_history;

    bool PutItem(const std::string &key, const std::string &value);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
