//
// Created by philipp on 10/31/19.
//

#ifndef TCHECKER_EXT_SWAPPED_VECTOR_HH
#define TCHECKER_EXT_SWAPPED_VECTOR_HH

template<class T>
class swapped_vector: protected std::vector<T>{
  // Access to members of private vector
  using std::vector<T>::begin;
  using std::vector<T>::end;
  using std::vector<T>::cbegin;
  using std::vector<T>::cend;
  using std::vector<T>::const_iterator;
  using std::vector<T>::iterator;
  using std::vector<T>::reserve;
  using std::vector<T>::clear;
  using std::vector<T>::operator[];
  
  template <class... ARGS>
  void emplace_back()
  
  -
  -    /*!
-     \brief Clears the container
-     \post All internal vectors are cleared and is_safe is false
-     */
  -    void clear();
};

#endif //TCHECKER_EXT_SWAPPED_VECTOR_HH

