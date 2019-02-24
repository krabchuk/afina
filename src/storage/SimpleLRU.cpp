#include <algorithm>
#include <iostream>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

bool SimpleLRU::PutItem(const std::string &key, const std::string &value) {
  std::size_t additional_size = key.size() + value.size();
  if (additional_size > _max_size)
    return false;

  while (_actual_size + additional_size > _max_size)
    Delete(_lru_history.back());

  _lru_storage.insert(std::make_pair(key, value));
  _lru_history.push_front(std::ref(_lru_storage[key]));
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (_lru_storage.find(key) != _lru_storage.end())
      return Set(key, value);

    return PutItem(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
  if (_lru_storage.find(key) != _lru_storage.end())
    return false;

  return PutItem(key, value);
}

void SimpleLRU::RemoveKey(const std::string &key) {
  for (unsigned int i = 0; i < _lru_history.size(); ++i) {
    if (_lru_history[i].get() == key) {
      _lru_history.erase(_lru_history.begin() + i);
      return;
    }
  }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  if (_actual_size - item->second.size() + value.size() > _max_size)
    return false;

  RemoveKey(item->first);

  _actual_size = _actual_size - item->second.size() + value.size();
  _lru_storage.erase(key);
  _lru_storage.insert(std::make_pair(key, value));

  _lru_history.push_front(std::ref(_lru_storage.find(key)->first));
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  RemoveKey(item->first);

  _actual_size = _actual_size - item->first.size() - item->second.size();
  _lru_storage.erase(key);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  for (unsigned int i = 0; i < _lru_history.size(); ++i) {
    if (_lru_history[i].get() == key) {
      _lru_history.erase(_lru_history.begin() + i);
      break;
    }
  }
  _lru_history.push_front(item->first);

  value.assign(item->second);
  return true;
}

} // namespace Backend
} // namespace Afina
