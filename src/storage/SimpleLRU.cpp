#include <algorithm>
#include <iostream>
#include <fstream>
#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

void SimpleLRU::lru_deque::pop_back() {
  lru_node *_tail_prev = _tail->_prev;
  if (_tail_prev) {
    _tail->_prev->_next.reset();
  }
  else {
    _head.reset();
  }
  _tail = _tail_prev;
}

void SimpleLRU::lru_deque::push_front(const std::string &key, const std::string &value) {
  std::unique_ptr<lru_node> old_head (std::move(_head));
  _head.reset(new lru_node (key, value));
  _head->_next = std::move(old_head);
  if (_head->_next)
    _head->_next->_prev = _head.get();
  if (_tail == nullptr)
    _tail = _head.get();
}

SimpleLRU::lru_deque::lru_node *SimpleLRU::lru_deque::find(lru_deque::lru_node *first, const std::string &key) const {
  lru_node *curr = first;
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
  return curr;
}

void SimpleLRU::lru_deque::erase(const std::string &key) {
  // We suppose, that this key exists
  lru_node *curr = find(_head.get(), key);

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

void SimpleLRU::lru_deque::print() {
  auto curr = _head.get();
  while (curr) {
    std::cout << "|" << curr->_key << "| : |" << curr->_value << "|" << std::endl;
    curr = curr->_next.get();
    if (curr && curr->_prev->_next->_key != curr->_key)
      throw std::runtime_error("Error");
  }
  curr = _tail;
  if (curr)
    std::cout << "TAIL = |" << curr->_key << "| : |" << curr->_value << "|" << std::endl;
  std::cout << "-----------------------------------------" << std::endl;
}

void SimpleLRU::lru_deque::update(const std::string &key, const std::string &value) {
  if (_head->_key == key)
    return;

  lru_node *curr = find(_head->_next.get(), key);

  if (curr->_next) {
    std::unique_ptr<lru_node> tmp = std::move(curr->_next);
    tmp->_prev = curr->_prev;
    curr->_next = std::move(_head);
    _head = std::move(curr->_prev->_next);
    curr->_prev->_next = std::move(tmp);
    _head->_next->_prev = _head.get();
    _head->_prev = nullptr;
  }
  else {
    _tail = curr->_prev;
    curr->_next = std::move(_head);
    curr->_next->_prev = curr;
    _head = std::move(curr->_prev->_next);
    _head->_prev = nullptr;
  }
}

bool SimpleLRU::PutItem(const std::string &key, const std::string &value) {
  std::size_t additional_size = key.size() + value.size();
  if (additional_size > _max_size)
    return false;

  while (_actual_size + additional_size > _max_size) {
    const std::string &key_for_delete = _lru_history->back();
    auto item = _lru_storage.find(key_for_delete);
    _actual_size = _actual_size - item->second.get()._key.size() - item->second.get()._value.size();
    _lru_storage.erase(key_for_delete);
    _lru_history->pop_back();
  }

  _actual_size += additional_size;
  _lru_history->push_front(key, value);
  _lru_storage.insert(std::make_pair(std::ref(_lru_history->front()), std::ref(_lru_history->head())));
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

  if (_actual_size - item->second.get()._value.size() + value.size() > _max_size)
    return false;

  _actual_size = _actual_size - item->second.get()._value.size() + value.size();
  _lru_storage.erase(key);
  _lru_history->erase(key);

  _lru_history->push_front(key, value);
  _lru_storage.insert(std::make_pair(std::ref(_lru_history->front()), std::ref(_lru_history->head())));
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  _actual_size = _actual_size - key.size() - item->second.get()._value.size();
  _lru_storage.erase(key);
  _lru_history->erase(key);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
  auto item = _lru_storage.find(key);
  if (item == _lru_storage.end())
    return false;

  value.assign(item->second.get()._value);
  _lru_history->update(item->second.get()._key, item->second.get()._value);

  return true;
}

} // namespace Backend
} // namespace Afina
