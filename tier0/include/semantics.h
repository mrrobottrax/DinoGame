#pragma once

template <typename T> struct RemoveReference {
  using type = T;
};

template <typename T> struct RemoveReference<T &> {
  using type = T;
};

template <typename T> struct RemoveReference<T &&> {
  using type = T;
};

template <typename T> typename RemoveReference<T>::type &&move(T &t) {
  return static_cast<typename RemoveReference<T>::type &&>(t);
}
