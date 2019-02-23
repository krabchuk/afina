#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

SimpleLRU::lru_node::lru_node(const std::string &key_arg, const std::string &value_arg,
                              Afina::Backend::SimpleLRU::lru_node *prev_arg) {
    key = key_arg;
    value = value_arg;
    prev = prev_arg;
}

void SimpleLRU::RemoveOldestItem() {

  if (_lru_head) {
    _actual_size -= (_lru_head->key.size() + _lru_head->value.size());
    _lru_head = std::move(_lru_head->next);
  }
}

void SimpleLRU::FreeSpaceForNewItem(std::size_t additional_size) {
  while (_actual_size + additional_size > _max_size)
    RemoveOldestItem();
}

bool SimpleLRU::PutItem(const std::string &key, const std::string &value) {
  std::size_t put_size = key.size() + value.size();
  if (put_size > _max_size)
    return false;

  FreeSpaceForNewItem(put_size);

  auto last = _lru_head.get();
  // Consider two cases: existing head and empty one
  if (last) {
    while (last->next)
      last = last->next.get();
    last->next.reset(new lru_node (key, value, last));
    _lru_index.insert(std::make_pair(last->next->key, std::ref(*last->next)));
  }
  else {
    _lru_head.reset(new lru_node (key, value));
    _lru_index.insert(std::make_pair(_lru_head->key, std::ref(*_lru_head)));
  }

}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (_lru_index.find(std::ref(key)) != _lru_index.end())
      return Set(key, value);

    return PutItem(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
  if (_lru_index.find(key) != _lru_index.end())
    return false;

  return PutItem(key, value);
}

SimpleLRU::lru_node *SimpleLRU::StorageSearch(const std::string &key) {
  // At this point we suppose, that item exists
  auto curr = _lru_head.get();
  while (curr->key != key)
    curr = curr->next.get();
  return curr;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
  if (_lru_index.find(key) == _lru_index.end())
    return false;

  auto item = StorageSearch(key);
  item->value = value;

  if (item->prev) {
    // Item isn't in head, so we have to move it
    if (item->next)
      item->next->prev = item->prev;
    _lru_head->prev = item;

    auto item_storage = std::move(item->prev->next);
    item->prev->next = std::move(item->next);
    item->next = std::move(_lru_head);
    _lru_head = std::move(item_storage);

    _lru_head->prev = nullptr;
  }
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  if (_lru_index.find(key) == _lru_index.end())
    return false;

  auto item = StorageSearch(key);

  if (item->prev) {
    if (item->next)
      item->next->prev = item->prev;
    item->prev->next = std::move(item->next);
  }
  else {
    _lru_head.reset();
  }
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
  auto item = _lru_index.find(key);
  if (item == _lru_index.end())
    return false;

  std::copy(item->second.get().value.begin(), item->second.get().value.end(), value.begin());
  return true;
}

} // namespace Backend
} // namespace Afina
