//
// Created by philipp on 12/11/19.
//

#ifndef TCHECKER_EXT_UTILS_HH
#define TCHECKER_EXT_UTILS_HH

#include <string>

namespace tchecker_ext{
  namespace utils{
    
    template<class T_TEMP, class T_RES>
    bool check_and_convert(T_TEMP val, char *p_end, T_RES &t){
      if(p_end==nullptr){
        return false;
      }
  
      if( val<std::numeric_limits<T_RES>::min() ||
          std::numeric_limits<T_RES>::max()<val ){
        return false;
      }
      t = (T_RES) val; //Safe to convert
      return true;
    }
    
    /*!
     * \brief Convert a string safely into the given numeric type
     * @tparam T
     * @param value
     * @param t
     * @return
     * \note Would be ideal to switch to c++20 with concepts
     */
    template <class T>
    bool to_numeric(std::string const & value, T & t){
  
      static_assert(std::is_arithmetic<T>::value,
          "Function can only convert to floats or ints");
  
      char *p_end = nullptr;
      
      if (std::is_integral<T>::value){
        if (std::is_signed<T>::value){
          long val = strtol(value.c_str(), &p_end,10);
          return check_and_convert(val, p_end, t);
        }else{
          unsigned long val = strtoul(value.c_str(), &p_end,10);
          return check_and_convert(val, p_end, t);
        }
      }else{
        double val = strtod(value.c_str(), &p_end);
        return check_and_convert(val, p_end, t);
      }
    }
  
  } // utils
} // tchecker_ext

#endif //TCHECKER_EXT_UTILS_HH
