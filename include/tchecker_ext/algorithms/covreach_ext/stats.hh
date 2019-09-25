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
      stats_t():tchecker::covreach::stats_t(){};
      
      stats_t(std::vector<stats_t> stats_vec){
        unsigned long visited=0, covered=0, covered_non=0, direct=0;
        for (const auto & it : stats_vec){
          visited += it.visited_nodes();
          direct += it.directly_covered_leaf_nodes();
          covered += it.covered_leaf_nodes();
          covered_non += it.covered_nonleaf_nodes();
        }
        tchecker::covreach::stats_t(visited, direct, covered, covered_non);
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
  
    };
    
  } // end of namespace covereach
  
} // end of namespace tchecker

#endif // TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_STATS_HH
