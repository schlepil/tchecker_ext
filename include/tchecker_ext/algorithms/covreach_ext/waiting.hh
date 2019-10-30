//
// Created by philipp on 24.09.19.
//

#ifndef TCHECKER_EXT_WAITING_HH
#define TCHECKER_EXT_WAITING_HH

#include <vector>
#include <chrono>
#include <thread>

#include "tchecker/algorithms/covreach/waiting.hh"
#include "tchecker_ext/utils/spinlock.hh"

#include <tchecker_ext/config.hh>

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
      class threaded_waiting_t: private W{
      public:
        /*!
          \brief Type of pointers to node
          */
        using node_ptr_t = typename W::element_t;
    
        /*!
\brief Constructor
*/
        threaded_waiting_t()
            : W()
        {}
    
        /*!
         \brief Copy constructor
         */
        threaded_waiting_t(tchecker_ext::covreach_ext::details::threaded_waiting_t<W> const &) = delete;
    
        /*!
         \brief Move constructor
         */
        threaded_waiting_t(tchecker_ext::covreach_ext::details::threaded_waiting_t<W> &&) = delete;
    
        /*!
         \brief Destructor
         */
        ~threaded_waiting_t() = default;
    
        /*!
         \brief Assignment operator
         */
        tchecker_ext::covreach_ext::details::threaded_waiting_t<W> &
        operator= (tchecker_ext::covreach_ext::details::threaded_waiting_t<W> const &) = delete;
    
        /*!
         \brief Move-assignment operator
         */
        tchecker_ext::covreach_ext::details::threaded_waiting_t<W> &
        operator= (tchecker_ext::covreach_ext::details::threaded_waiting_t<W> &&) = delete;
    
        /*!
          \brief Accessor
          \return true if the container is empty, false otherwise
          */
        bool empty()
        {
          _lock.lock();
          bool is_empty = (_n_pending==0) && (W::empty());
          _lock.unlock();
          return is_empty;
        }
    
        /*!
          \brief Insert a list of elements and decrement pending
          \param t : element
          \post t has been inserted in this container if t satisfies the filter
          */
        void insert_and_decrement(std::vector<node_ptr_t> & node_vec, bool do_decrement=true, int const worker_num=0){
          unsigned long n_inserted = 0;
          unsigned long n_inserted_active = 0;
          _lock.lock();//blocking until locked
#if (SCHLEPIL_DBG>=2)
          std::cout << "Acquired waiting lock to insert with pending|empty " << _n_pending << " ; " << W::empty() << std::endl;
#endif
          for (node_ptr_t & node : node_vec){
            if (node.ptr() != nullptr){
              ++n_inserted;
              n_inserted_active += node->is_active(); // It can happen that another threads makes them inactive in the mean-time
              W::swap_insert(node); //This does not change the reference counter -> No need to lock container
              assert(node.ptr() == nullptr);
            }
          }
          if(do_decrement){
            assert(_n_pending>0);
            --_n_pending;
          }
#if (SCHLEPIL_DBG>=2)
          std::cout << "inserted " << n_inserted << " from which were active " << n_inserted_active << " by " << "worker_num" << worker_num << std::endl;
          for (node_ptr_t & node : node_vec) {
            assert(node.ptr() == nullptr);
          }
#endif
          // All node in node_vec should be nullptr by now (Either because they were inserted into the waiting queue
          // or because they were already removed due to inactivity beforehand
          // It is safe to clear the vector now
          node_vec.clear();
          _lock.unlock();
          return;
        }
    
        /*!
         \brief Store the first element in the ptr, if by chance another tread has taken the last element
                return false; This call is blocking
         \param node; contains the next element if successful
         */
        bool pop_and_increment(node_ptr_t & node){
          assert(node.ptr()==nullptr);
          while(true){
            _lock.lock();//blocking until locked
#if (SCHLEPIL_DBG>=2)
            std::cout << "Acquired waiting lock to pop with pending|empty " << _n_pending << " ; " << W::empty() << std::endl;
#endif
            if ((_n_pending==0) && (W::empty())){
              //The list is really empty -> done
              _lock.unlock();
              return false;
            }
            if (!W::empty()){
              // There is something in the list -> get it
              // "pop"
              //node = tchecker::covreach::details::active_waiting_t<W>::first();
              //tchecker::covreach::details::active_waiting_t<W>::remove_first();
              // This will not effect the reference counter, so there is no problem with other threads creating
              // or deleting reference to the underlying shared
              // TODO: Currently we cannot use a true active waiting as this would interfere with the reference counter in a non-controlled way
              W::swap_first_and_remove(node); //This will then remove a shared nullptr -> no reference counter is changed
          
              // "increment"
              ++_n_pending;
              _lock.unlock();
              return true;
            }
#if (SCHLEPIL_DBG>=2)
            std::cout << "unlocking" << std::endl;
#endif
            _lock.unlock(); //Sleep here? How long?
            // Give others a chance to put sth
            std::this_thread::sleep_for(std::chrono::microseconds(5));//todo
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
     \note this container does not filter active nodes - this is currently not supported by threading
     */
    template <class NODE_PTR>
    using threaded_fifo_waiting_t = tchecker_ext::covreach_ext::details::threaded_waiting_t<tchecker::fifo_waiting_t<NODE_PTR>>;
    
    /*!
     \brief Last-In-First-Out waiting container
     \note this container does not filter active nodes - this is currently not supported by threading
     */
    template <class NODE_PTR>
    using threaded_lifo_waiting_t = tchecker_ext::covreach_ext::details::threaded_waiting_t<tchecker::lifo_waiting_t<NODE_PTR>>;
    
  } // covreach_ext
} // tchecker_ext

#endif //TCHECKER_EXT_WAITING_HH
