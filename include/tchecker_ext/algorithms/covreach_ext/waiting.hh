//
// Created by philipp on 24.09.19.
//

#ifndef TCHECKER_EXT_WAITING_HH
#define TCHECKER_EXT_WAITING_HH

#include <vector>

#include "tchecker/algorithms/covreach/waiting.hh"
#include "tchecker_ext/utils/spinlock.hh"

namespace tchecker_ext{
  namespace covreach_ext{
    namespace details{
  
      /*!
       \class threaded_active_waiting_t
       \brief Waiting container that filters active nodes and is thread safe
       \param W : type of underlying waiting container, should contain nodes that inherit
       from tchecker::covreach::details::active_node_t
       \note this container acts as W restricted to its active nodes
       */
       template <class W>
       class threaded_active_waiting_t: private tchecker::covreach::details::active_waiting_t<W>{
       public:
         /*!
           \brief Type of pointers to node
           */
         using node_ptr_t = typename W::element_t;
  
         /*!
 \brief Constructor
 */
         threaded_active_waiting_t()
             : tchecker::covreach::details::active_waiting_t<W>()
         {}
  
         /*!
          \brief Copy constructor
          */
         threaded_active_waiting_t(tchecker_ext::covreach_ext::details::threaded_active_waiting_t<W> const &) = delete;
  
         /*!
          \brief Move constructor
          */
         threaded_active_waiting_t(tchecker_ext::covreach_ext::details::threaded_active_waiting_t<W> &&) = delete;
  
         /*!
          \brief Destructor
          */
         ~threaded_active_waiting_t() = default;
  
         /*!
          \brief Assignment operator
          */
         tchecker_ext::covreach_ext::details::threaded_active_waiting_t<W> &
         operator= (tchecker_ext::covreach_ext::details::threaded_active_waiting_t<W> const &) = delete;
  
         /*!
          \brief Move-assignment operator
          */
         tchecker_ext::covreach_ext::details::threaded_active_waiting_t<W> &
         operator= (tchecker_ext::covreach_ext::details::threaded_active_waiting_t<W> &&) = delete;
  
         /*!
           \brief Remove node
           \param n : a node
           \post n is inactive
           \note n is actually not removed from this waiting container but marked inactive. The node n is removed and
           ingored when it becomes the first element in this container
           */
         void remove(node_ptr_t const & n)
         {
           _lock.lock();
           tchecker::covreach::details::active_waiting_t<W>::remove(n);
           _lock.unlock();
         }
  
         /*!
           \brief Accessor
           \return true if the container is empty, false otherwise
           */
         bool empty()
         {
           _lock.lock();
           bool is_empty = (_n_pending==0) && (tchecker::covreach::details::active_waiting_t<W>::empty());
           _lock.unlock();
           return is_empty;
         }
  
         /*!
           \brief Insert a list of elements and decrement pending
           \param t : element
           \post t has been inserted in this container if t satisfies the filter
           */
         void insert_and_decrement(const std::vector<node_ptr_t> & node_vec, bool do_decrement=true){
           _lock.lock();
           for (const node_ptr_t & node : node_vec){
             tchecker::covreach::details::active_waiting_t<W>::insert(node);
           }
           if(do_decrement){
             --_n_pending;
           }
           _lock.unlock();
         }
         
         
         /*!
          \brief Store the first active element in the ptr, if by chance another tread has taken the last element
                 return false; This call is blocking
          \param node; contains the next element if successful
          */
          bool pop_and_increment(node_ptr_t & node){
            while(true){
              _lock.lock();
              if ((_n_pending==0) && (tchecker::covreach::details::active_waiting_t<W>::empty())){
                //The list is really empty -> done
                return false;
              }
              if (!tchecker::covreach::details::active_waiting_t<W>::empty()){
                // There is something in the list -> get it
                // "pop"
                node = tchecker::covreach::details::active_waiting_t<W>::first();
                tchecker::covreach::details::active_waiting_t<W>::remove_first();
                // "increment"
                ++_n_pending;
                return true;
              }
              // If one arrives here that means that other workers might enqueue node -> redo the loop
            }
          }

       private:
         tchecker_ext::spinlock_t _lock; /*! Lock making the waiting list thread safe */
         long _n_pending=0; /*! Number of threads that are still pending, that is, which are likely to enqueue elements */
         
       };
    } // details
  
    /*!
     \brief First-In-First-Out waiting container
     \note this container filters active nodes
     */
    template <class NODE_PTR>
    using threaded_fifo_waiting_t = tchecker_ext::covreach_ext::details::threaded_active_waiting_t<tchecker::fifo_waiting_t<NODE_PTR>>;
    
    /*!
     \brief Last-In-First-Out waiting container
     \note this container filters active nodes
     */
    template <class NODE_PTR>
    using threaded_lifo_waiting_t = tchecker_ext::covreach_ext::details::threaded_active_waiting_t<tchecker::lifo_waiting_t<NODE_PTR>>;
    
  } // covreach_ext
} // tchecker_ext

#endif //TCHECKER_EXT_WAITING_HH
