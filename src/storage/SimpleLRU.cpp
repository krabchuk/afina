#include <algorithm>
#include <iostream>
#include <fstream>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

void SimpleLRU::deque::pop_back() {
  deque_list *_tail_prev = _tail->_prev;
  if (_tail_prev) {
    _tail->_prev->_next.reset();
  }
  else {
    _head.reset();
  }
  _tail = _tail_prev;
}

void SimpleLRU::deque::push_front(const std::string &key) {
  std::unique_ptr<deque_list> old_head (std::move(_head));
  _head.reset(new deque_list (key));
  _head->_next = std::move(old_head);
  if (_head->_next)
    _head->_next->_prev = _head.get();
  if (_tail == nullptr)
    _tail = _head.get();
}

void SimpleLRU::deque::erase(const std::string &key) {
  // We suppose, that this key exists
  deque_list *curr = _head.get();
  int flag = 0;
  while (curr) {
    if (curr->_key == key) {
      flag = 1;
      break;
    }
    curr = curr->_next.get();
  }
  if (!flag)
    throw std::invalid_argument("Key doesn't exist " + key);

  if (curr->_next) {
    curr->_next->_prev = curr->_prev;
    if (curr->_prev) {
      curr->_prev->_next = std::move(curr->_next);
    }
    else {
      _head = std::move(curr->_next);
    }
  }
  else {
    pop_back();
  }
}

void SimpleLRU::deque::update(const std::string &key) {
  if (_head->_key == key)
    return;
  erase(key);
  push_front(key);
}

bool SimpleLRU::PutItem(const std::string &key, const std::string &value) {
  std::size_t additional_size = key.size() + value.size();
  if (additional_size > _max_size)
    return false;

  while (_actual_size + additional_size > _max_size) {
    auto &key_for_delete = _lru_history->back();
    _actual_size = _actual_size - key_for_delete.size() - _lru_storage[key_for_delete].size();
    _lru_storage.erase(key_for_delete);
    _lru_history->pop_back();
  }

  _actual_size += additional_size;
  _lru_storage.insert(std::make_pair(key, value));
  _lru_history->push_front(_lru_storage.find(key)->first);
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

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  if (_actual_size - item->second.size() + value.size() > _max_size)
    return false;

  _actual_size = _actual_size - item->second.size() + value.size();
  _lru_storage.erase(key);
  _lru_storage.insert(std::make_pair(key, value));
  _lru_history->push_front(_lru_storage.find(key)->first);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  _actual_size = _actual_size - item->first.size() - item->second.size();
  _lru_history->erase(key);
  _lru_storage.erase(item);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  _lru_history->update(item->first);

  value.assign(item->second);
  return true;
}

} // namespace Backend
} // namespace Afina
