/*!
 * \file hash_key.h
 *
 * \date 21.02.2011
 * \author Tilo Eissler
 */

#ifndef HASH_KEY_H_
#define HASH_KEY_H_

#include <string.h>

template<class _Key> struct hash_key {
};

inline std::size_t hash_string(const char *str) {
 std::size_t hash = 0; // TODO other SALT ??
 for (; *str; ++str) {
  hash = 101 * hash + *str;
 }
 return hash;
}

template<>
struct hash_key<const char *> {
 std::size_t operator()(const char *str) const {
  return hash_string(str);
 }
};

template<>
struct hash_key<char *> {
 std::size_t operator()(const char *str) const {
  return hash_string(str);
 }
};

struct equal_str {
 bool operator()(const char* s1, const char* s2) const {
  if (!strcasecmp(s1, s2)) {
   return true;
  }
  return false;
 }
};

#endif /* HASH_KEY_H_ */
