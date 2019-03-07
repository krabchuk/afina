#include <algorithm>
#include <iostream>
#include <fstream>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

void SimpleLRU::PopBack() {
  lru_node *_tail_prev = _lru_tail->_prev;
  if (_tail_prev) {
    _lru_tail->_prev->_next.reset();
  }
  else {
    _lru_head.reset();
  }
  _lru_tail = _tail_prev;
}

bool SimpleLRU::PutItem(const std::string &key, const std::string &value) {
  std::size_t additional_size = key.size() + value.size();
  if (additional_size > _max_size)
    return false;

  while (_actual_size + additional_size > _max_size) {
    Delete(_lru_tail->_key);
  }

  _actual_size += additional_size;

  if (_lru_head)
  {
    std::unique_ptr<lru_node> old_head (std::move(_lru_head));
    _lru_head.reset(new lru_node (key, value));
    _lru_head->_next = std::move(old_head);
    if (_lru_head->_next)
      _lru_head->_next->_prev = _lru_head.get();
    if (_lru_tail == nullptr)
      _lru_tail = _lru_head.get();
  }
  else
  {
    _lru_head.reset(new lru_node (key, value));
    _lru_tail = _lru_head.get();
  }


  _lru_index.insert(std::make_pair(std::ref(_lru_head->_key), std::ref(*_lru_head)));
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) != _lru_index.end())
      return Set(key, value);

    return PutItem(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
  if (_lru_index.find(key) != _lru_index.end())
    return false;

  return PutItem(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
  auto item = _lru_index.find(key);
  if (item == _lru_index.end())
    return false;

  if (_actual_size - item->second.get()._value.size() + value.size() > _max_size)
    return false;

  _actual_size = _actual_size - item->second.get()._value.size() + value.size();
  item->second.get()._value.assign(value);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  auto item = _lru_index.find(key);
  if (item == _lru_index.end())
    return false;

  _actual_size = _actual_size - key.size() - item->second.get()._value.size();
  auto &curr = item->second.get()._prev ? item->second.get()._prev->_next : _lru_head;
  if (curr->_next) {
    curr->_next->_prev = curr->_prev;
    if (curr->_prev) {
      curr->_prev->_next = std::move(curr->_next);
    }
    else {
      _lru_head = std::move(curr->_next);
    }
  }
  else {
    PopBack();
  }

  _lru_index.erase(item);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
  auto item = _lru_index.find(key);
  if (item == _lru_index.end())
    return false;

  lru_node &curr = item->second.get();
  value.assign(curr._value);

  if (curr._key != _lru_head->_key) {
    if (curr._next) {
      std::unique_ptr<lru_node> tmp = std::move(curr._next);
      tmp->_prev = curr._prev;
      curr._next = std::move(_lru_head);
      _lru_head = std::move(curr._prev->_next);
      curr._prev->_next = std::move(tmp);
      _lru_head->_next->_prev = _lru_head.get();
    }
    else {
      _lru_tail = curr._prev;
      curr._next = std::move(_lru_head);
      _lru_head = std::move(curr._prev->_next);
      _lru_head->_next->_prev = _lru_head.get();
    }
    _lru_head->_prev = nullptr;
  }

  return true;
}

} // namespace Backend
} // namespace Afina
