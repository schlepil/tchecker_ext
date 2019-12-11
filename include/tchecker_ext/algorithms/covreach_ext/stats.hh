/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_STATS_HH
#define TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_STATS_HH

#include "tchecker/algorithms/covreach/stats.hh"

#include <vector>
#include <chrono>


/*!
 \file stats.hh
 \brief Statistics for multithreaded covering reachability algorithm
 */

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    /*!
     \class stats_t
     \brief Statistics for multithreaded covering reachability algorithm
     */
    class stats_t: public tchecker::covreach::stats_t {
    public:
  
      /*!
       \brief Constructor
       */
      stats_t(int n_notification=0, std::string notify_string="")
      : tchecker::covreach::stats_t(), _n_notify(n_notification),
        _n_since_last(-_n_notify), _do_notifiy(n_notification > 0),
        _notify_string(notify_string), t_last(std::chrono::high_resolution_clock::now())
      {};
      
      template <class CONTAINER >
      stats_t(CONTAINER stats_vec):tchecker::covreach::stats_t(){
        for (const auto & it : stats_vec){
          _visited_nodes += it.visited_nodes();
          _directly_covered_leaf_nodes += it.directly_covered_leaf_nodes();
          _covered_leaf_nodes += it.covered_leaf_nodes();
          _covered_nonleaf_nodes += it.covered_nonleaf_nodes();
        }
      }
  
      /*!
       \brief Copy constructor
       */
      stats_t(tchecker_ext::covreach_ext::stats_t const & other):tchecker::covreach::stats_t(other){}
  
      /*!
       \brief Move constructor
       */
      stats_t(tchecker_ext::covreach_ext::stats_t && other):tchecker::covreach::stats_t(std::move(other)){}
  
      /*!
       \brief Destructor
       */
      ~stats_t(){}
  
      /*!
       \brief Assignment operator
       */
      tchecker_ext::covreach_ext::stats_t & operator= (tchecker_ext::covreach_ext::stats_t const & other){
        tchecker::covreach::stats_t::operator=(other);
        return *this;
      }
  
      /*!
       \brief Move-assignment operator
       */
      tchecker::covreach::stats_t & operator= (tchecker::covreach::stats_t && other){
        tchecker::covreach::stats_t::operator=(std::move(other));
        return *this;
      }
      
      /*!
       * \brief Extend the incrementation of visitied node by an advancement outputter
       */
      void increment_visited_nodes();

    protected:
      int _n_notify; /*! Number of nodes between two notifications */
      int _n_since_last=0; /*!Avoid modulo*/
      bool _do_notifiy; /*! Boolean to switch notification on or off */
      std::string _notify_string; /*! String to display when notified */
      std::chrono::high_resolution_clock::time_point t_last; /*! Timing */
  
    };
    
  } // end of namespace covereach
  
} // end of namespace tchecker

#endif // TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_STATS_HH
