#pragma once

#include <iostream>
#include <string>

template<class T>
const T& j(const T& x) {
  return x;
}

template<>
const string& j<string>(const string& x) {
  static string S; // so evil
  S = "\"";
  S += x;
  S += "\"";
  return S;
}

template<class T>
class JsonPair {
public:
  JsonPair(const string& key, const T& val, bool first): Key(key), Value(val), First(first) {}

  const string& Key;
  const T& Value;
  bool First;
};

template<class T>
JsonPair<T> j(const string& key, const T& val, bool first = false) {
  return JsonPair<T>(key, val, first);
}

template<class T>
ostream& operator<<(ostream& out, const JsonPair<T>& pair) {
  if (!pair.First) {
    out << ",";
  }
  out << j(pair.Key) << ":";
  out << j(pair.Value);
  return out;
}
