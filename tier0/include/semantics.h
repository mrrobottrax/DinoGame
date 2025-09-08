#pragma once

template <typename T> struct remove_reference {
  using type = T;
};

template <typename T> struct remove_reference<T &> {
  using type = T;
};

template <typename T> struct remove_reference<T &&> {
  using type = T;
};

template <typename T>
T &&forward(typename remove_reference<T>::type &t) noexcept {
  return static_cast<T &&>(t);
}
