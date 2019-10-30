/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_SPINLOCK_HH
#define TCHECKER_EXT_SPINLOCK_HH

#include "tchecker/utils/spinlock.hh"

/*!
 \file spinlock.hh
 \brief Spin lock
 */

namespace tchecker_ext {
  
  /*!
   \class spinlock_t
   \brief Spin lock
   */
  class spinlock_t: public tchecker::spinlock_t {
  public:
    /*!
     \brief Constructor
     \post this lock is unlocked
     */
    spinlock_t():tchecker::spinlock_t()
    {}
    
    /*!
     \brief Copy constructor
     */
    spinlock_t(tchecker_ext::spinlock_t const &) = delete;
    
    /*!
     \brief Move constructor
     */
    spinlock_t(tchecker_ext::spinlock_t &&) = delete;
    
    /*!
     \brief Destructor
     */
    ~spinlock_t() = default;
    
    /*!
     \brief Assignment operator
     */
    tchecker_ext::spinlock_t & operator= (tchecker_ext::spinlock_t const &) = delete;
    
    /*!
     \brief Move assignment oeprator
     */
    tchecker_ext::spinlock_t & operator= (tchecker_ext::spinlock_t &&) = delete;
    
    /*!
     \brief Try once to acquire the lock and return the result
     \post lock acquired -> true; locked by different thread -> false
     */
    inline bool lock_once()
    {
      // the flag returns its old value; It returns false if it is currently not taken
      // Therefore if it returns false, we set it to true and this thread acquired the lock
      return !_flag.test_and_set(std::memory_order_acquire);
    }
    
  };
  
  
} // end of namespace tchecker_ext

#endif // TCHECKER_EXT_SPINLOCK_HH
