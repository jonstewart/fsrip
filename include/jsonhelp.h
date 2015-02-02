#pragma once

#include <iostream>
#include <string>

template<class T>
const T& j(const T& x) {
  return x;
}

template<class T>
const T* j(const T* x) {
  return x;
}

std::string j(const std::string& x);

// template<>
// std::string j<char>(const char* x);

template<class T>
class JsonPair {
public:
  JsonPair(const std::string& key, const T& val, bool first): Key(key), Value(val), First(first) {}

  const std::string& Key;
  const T& Value;
  bool First;
};

template<class T>
JsonPair<T> j(const std::string& key, const T& val, bool first = false) {
  return JsonPair<T>(key, val, first);
}

template<class T>
std::ostream& operator<<(std::ostream& out, const JsonPair<T>& pair) {
  if (!pair.First) {
    out << ",";
  }
  out << j(pair.Key) << ":";
  out << j(pair.Value);
  return out;
}
