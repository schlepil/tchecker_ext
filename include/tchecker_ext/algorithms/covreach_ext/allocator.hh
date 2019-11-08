//
// Created by philipp on 11.10.19.
//
//

#ifndef TCHECKER_EXT_ALLOCATOR_HH
#define TCHECKER_EXT_ALLOCATOR_HH

#include "tchecker/ts/allocators.hh"

namespace tchecker_ext{
  namespace threaded_ts{
  
    template <class STATE_ALLOCATOR, class TRANSITION_ALLOCATOR>
    class allocator_t: public tchecker::ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR>{
    public:
      
      // Declare all the constructors of base class
      /*!
       \brief Constructor
       \param gc : garbage collector
       \param sa_args : parameters to a constructor of STATE_ALLOCATOR
       \param ta_args : parameters to a constructor of TRANSITION_ALLOCATOR
       \post this owns a state allocator built from sa_args, and a transition allocator built from ta_args.
       Both allocators have been enrolled to gc
       */
      template <class ... SA_ARGS, class ... TA_ARGS>
      allocator_t(tchecker::gc_t & gc, std::tuple<SA_ARGS...> && sa_args, std::tuple<TA_ARGS...> && ta_args)
          : tchecker::ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR>(gc, sa_args, ta_args)
      {}
  
      /*!
       \brief Constructor
       \param t : tuple of arguments to a constructor of tchecker::ts::allocator_t
       \post see other constructors
       */
      template <class ... SA_ARGS, class ... TA_ARGS>
      allocator_t(std::tuple<tchecker::gc_t &, std::tuple<SA_ARGS...>, std::tuple<TA_ARGS...>> && t)
          : tchecker::ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR>(std::get<0>(t),
                        std::forward<std::tuple<SA_ARGS...>>(std::get<1>(t)),
                        std::forward<std::tuple<TA_ARGS...>>(std::get<2>(t)))
      {}
  
      /*!
       \brief Copy constructor (deleted)
       */
      allocator_t(tchecker_ext::threaded_ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR> const &) = delete;
  
      /*!
       \brief Move constructor (deleted)
       */
      allocator_t(tchecker_ext::threaded_ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR> &&) = delete;
  
      /*!
       \brief Destructor
       \note see destruct_all()
       */
      ~allocator_t() = default;
  
      /*!
       \brief Assignment operator (deleted)
       */
      tchecker_ext::threaded_ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR> &
      operator= (tchecker_ext::threaded_ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR> const &) = delete;
  
      /*!
       \brief Move-assignment operator (deleted)
       */
      tchecker_ext::threaded_ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR> &
      operator= (tchecker_ext::threaded_ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR> &&) = delete;
      
      
      /*!
       * \brief Accessor to the state/node allocator
       * @return
       */
      STATE_ALLOCATOR &  get_state_allocator(){
        return tchecker::ts::allocator_t<STATE_ALLOCATOR, TRANSITION_ALLOCATOR>::_state_allocator;
      }
    };
  
  
    /*!
     \class threaded_allocator_helper_t
     \brief Auxilliary class to allow for multithreaded exploration
     \note This is basically a work-around to the problem of the singleton allocated transition
           threaded_builder_allocators share a common allocator for the states, but each holds its own
           singleton_pool for the transition
     \tparam STATE_ALLOCATOR : type of state allocator
     \tparam TRANSITION_ALLOCATOR : type of transition allocator
     */
    template <class TS_ALLOCATOR>
    class threaded_builder_allocator_t {
    public:
      /*!
       \brief Type of transition allocator
       */
      using transition_allocator_t = typename TS_ALLOCATOR::transition_allocator_t;
      /*!
       \brief Type of state allocator
       */
      using state_allocator_t = typename TS_ALLOCATOR::state_allocator_t;
      /*!
       \brief Type of states
       */
      using state_t = typename state_allocator_t::t;
      /*!
       \brief Type of pointers to state
       */
      using state_ptr_t = typename state_allocator_t::ptr_t;
      /*!
       \brief Type of transitions
       */
      using transition_t = typename transition_allocator_t::t;
  
      /*!
       \brief Type of pointers to transition
       */
      using transition_ptr_t = typename transition_allocator_t::ptr_t;
  
      /*!
       \brief Constructor
       \param gc : garbage collector
       \param sa_args : parameters to a constructor of STATE_ALLOCATOR
       \param ta_args : parameters to a constructor of TRANSITION_ALLOCATOR
       \post this owns a state allocator built from sa_args, and a transition allocator built from ta_args.
       Both allocators have been enrolled to gc
       */
      template <class ... TA_ARGS>
      threaded_builder_allocator_t(tchecker::gc_t & gc, TS_ALLOCATOR &ts_allocator, std::tuple<TA_ARGS...> && ta_args)
          : _ts_allocator(ts_allocator),
            _transition_allocator(std::make_from_tuple<transition_allocator_t >(ta_args))
      {
        _transition_allocator.enroll(gc);
      }
  
      /*!
       \brief Copy constructor (deleted)
       */
      threaded_builder_allocator_t(tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR> const &) = delete;
  
      /*!
       \brief Move constructor (deleted)
       */
      threaded_builder_allocator_t(tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR> &&) = delete;
  
      /*!
       \brief Destructor
       \note see destruct_all()
       */
      ~threaded_builder_allocator_t() = default;
  
      /*!
       \brief Assignment operator (deleted)
       */
      tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR> &
      operator= (tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR> const &) = delete;
  
      /*!
       \brief Move-assignment operator (deleted)
       */
      tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR> &
      operator= (tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR> &&) = delete;
  
      /*!
       \brief transitions destruction
       \post all allocated transitions have been deleted. All pointers
       returned by method allocate_transition() have been invalidated
       */
      void destruct_all()
      {
        _transition_allocator.destruct_all();
      }
  
      /*!
       \brief Fast memory deallocation
       \post all allocated transitions have been freed. No destructor
       has been called. All pointers returned by methods allocate_state(),
       allocate_from_state() and allocate_transition() have been invalidated
       */
      void free_all()
      {
        _transition_allocator.free_all();
      }
      
      //Everything concerning the states is "forward" to the ts_allocator
      /*!
       \brief State construction
       \param args : parameters to a constructor of state_t
       \return pointer to a newly allocated state constructed from args
       */
      template <class ... ARGS>
      inline state_ptr_t construct_state(ARGS && ... args)
      {
        return _ts_allocator.template construct_state(std::forward<ARGS>(args)...);
      }
  
      /*!
       \brief State construction
       \param args : tuple of parameters to a constructor of state_t
       \return pointer to a newly allocated state constructed from args
       */
      template <class ... ARGS>
      inline state_ptr_t construct_state(std::tuple<ARGS...> && args)
      {
        return _ts_allocator.template construct_state(std::forward<std::tuple<ARGS...>>(args));
      }
  
      /*!
       \brief State construction
       \param state : a state
       \param args : extra parameters to a constructor of state_t
       \return pointer to a newly allocated state constructed from state and args
       */
      template <class ... ARGS>
      inline state_ptr_t construct_from_state(state_ptr_t const & state, ARGS && ... args)
      {
        return _ts_allocator.template construct_from_state(state, std::forward<ARGS>(args)...);
      }
  
      /*!
       \brief State construction
       \param state : a state
       \param args : tuple of extra parameters to a constructor of state_t
       \return pointer to a newly allocated state constructed from state and args
       */
      template <class ... ARGS>
      inline state_ptr_t construct_from_state(state_ptr_t const & state, std::tuple<ARGS...> && args)
      {
        return _ts_allocator.template construct_from_state(state, std::forward<std::tuple<ARGS...>>(args));
      }
  
      /*!
       \brief Destruct state
       \param p : pointer to state
       \pre p has been allocated by this allocator
       p is not nullptr
       \post the state pointed by p has been destructed and set to nullptr if its reference
       counter is 1 (i.e. p is the only pointer to this state)
       */
      bool destruct_state(state_ptr_t & p)
      {
        return _ts_allocator.destruct_state(p);
      }
      
      /*!
       \brief Transition construction
       \param args : tuple of parameters to a constructor of transition_t
       \return pointer to a newly allocated transition constructed from args
       */
      template <class ... ARGS>
      inline transition_ptr_t construct_transition(std::tuple<ARGS...> && args)
      {
        return
            std::apply(&tchecker_ext::threaded_ts::threaded_builder_allocator_t<TS_ALLOCATOR>::_construct_transition<ARGS...>,
                       std::tuple_cat(std::make_tuple(this), args));
      }
  
      /*!
       \brief Destruct transition
       \param p : pointer to transition
       \pre p has been allocated by this allocator
       p is not nullptr
       \post the transition pointed by p has been destructed and set to nullptr if its reference
       counter is 1 (i.e. p is the only pointer to this transition)
       */
      bool destruct_transition(transition_ptr_t & p)
      {
        return _transition_allocator.destruct(p);
      }
  
      /*!
       \brief Enroll on garbage collector
       \param gc : garbage collector
       \pre this is not enrolled to a garbage collector yet
       \post this is enrolled to gc
       */
      void enroll(tchecker::gc_t & gc)
      {
        _transition_allocator.enroll(gc);
      }



    protected:
  
      /*!
       \brief Transition construction
       \param args : parameters to a constructor of transition_t
       \return pointer to a newly allocated transiton constructed from args
       */
      template <class ... ARGS>
      inline transition_ptr_t _construct_transition(ARGS && ... args)
      {
        return _transition_allocator.construct(args...);
      }
      
      TS_ALLOCATOR &_ts_allocator; /*!reference to original allocator*/
      transition_allocator_t  _transition_allocator; /*! Holds it own transition allocator for threaded building*/
  
    };
    
  }
  
}

#endif //TCHECKER_EXT_ALLOCATOR_HH