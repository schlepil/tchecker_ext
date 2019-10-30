//
// Created by philipp on 02.10.19.
//

#ifndef TCHECKER_EXT_ARRAY_HH
#define TCHECKER_EXT_ARRAY_HH

#include "tchecker/utils/array.hh"

namespace tchecker_ext{
  
  /*!
   \brief Partial equality check a1 is a sub-array of a2, s.t. they are equal iff for the index array J: a1[i] == a2[J[i]]
   \param a1 : array
   \param a2 : array
   \return
   */
  template <class T, std::size_t T_ALLOCSIZE, class BASE>
  bool part_eq (tchecker::make_array_t<T, T_ALLOCSIZE, BASE> const & a1,
                tchecker::make_array_t<T, T_ALLOCSIZE, BASE> const & a2,
                const std::vector<BASE> & idx_j)
  {
    assert (static_cast<BASE const &>(a1).capacity() <= static_cast<BASE const &>(a2).capacity());
    assert (static_cast<BASE const &>(a1).capacity() == idx_j.size());
    
    for (std::size_t i = 0; i < static_cast<BASE const &>(a1).capacity(); ++i) {
      assert((0 <= idx_j[i]) && (idx_j[i] < static_cast<BASE const &>(a2).capacity()));
      if (a1[i] != a2[idx_j[i]]){
        return false;
    }
  }
  return true;
  } //part_eq
  
  /*!
   \brief Hash of a partial array
   \param a : array
   \param idx_j : index array which parts to consider
   \return hash value for array a[J]
   */
  template <class T, std::size_t T_ALLOCSIZE, class BASE>
  std::size_t part_hash_value(tchecker::make_array_t<T, T_ALLOCSIZE, BASE> const & a,
                              const std::vector<BASE> & idx_j)
  {
    std::size_t h = hash_value(static_cast<BASE>(a));
    for (const BASE & j : idx_j){
      // TODO check if consistent
      boost::hash_combine(h, a[j]); //Hash_range simply does multiple hash-combines?
    }
    return h;
  }
  
  
}//tchecker_ext

#endif //TCHECKER_EXT_ARRAY_HH
