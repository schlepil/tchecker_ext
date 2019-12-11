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
        \class threaded_waiting_t
        \brief Waiting container that is thread safe
        \param W : type of underlying waiting container, should contain nodes that inherit
        from tchecker::covreach::node_t.
        \note To be thread safe the container is not allowed to interfere with the reference counter of the underlying objects
              therefore use lists or deque for container, not vector
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
          \note The container is only truly empty if there exist no other threads that
                might still insert new items.
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
          \param node_vec : vector of elements to insert
          \param do_decrement : whether decrementing pending or not;
          \post elements that do not point to null are inserted into waiting, container is now empty
          \note do_decrement should only be false when inserting initial elements
          \note this call is blocking
          */
        void insert_and_decrement(std::vector<node_ptr_t> & node_vec, bool do_decrement=true){

          _lock.lock();//blocking until locked

          for (node_ptr_t & node : node_vec){
            if (node.ptr() != nullptr){
              W::swap_insert(node); // This does not change the reference counter -> No need to lock container
              assert(node.ptr() == nullptr);
            }
          }
          if(do_decrement){
            assert(_n_pending>0);
            --_n_pending;
          }
          // All modifications on waiting done
          
          // All nodes in node_vec should be nullptr by now (Either because they were inserted into the waiting queue
          // or because they were already removed due to inactivity beforehand) -> ok to delete
          node_vec.clear();
          _lock.unlock();
          return;
        }
    
        /*!
         \brief Store the first element in the given reference, if by chance another tread has taken the last element
                return false;
         \param node : reference to a node pointer
         \pre node points to null (checked by assertion)
         \post next element is stored in node, returns true; returns false when empty, node remains null
         \note This call is blocking
         */
        bool pop_and_increment(node_ptr_t & node){
          assert(node.ptr()==nullptr);
          while(true){ // Redo loop until container is empty
            _lock.lock();//blocking until locked

            if ((_n_pending==0) && (W::empty())){
              //The list is really empty -> done
              _lock.unlock();
              return false;
            }
            if (!W::empty()){
              W::swap_first_and_remove(node);
              // "increment"
              ++_n_pending;
              _lock.unlock();
              return true;
            }

            _lock.unlock(); //Sleep how long?
            // If one arrives here that means that other workers might enqueue node -> redo the loop
            std::this_thread::sleep_for(std::chrono::microseconds(5));//todo
          } // while
        }
        
        bool check_for_no_null(){
          _lock.lock();
          return W::check_for_no_null();
          _lock.unlock();
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
