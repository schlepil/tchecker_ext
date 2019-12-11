/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#include "tchecker_ext/algorithms/covreach_ext/stats.hh"

namespace tchecker_ext {
  
  namespace covreach_ext {
    /*!
     * \brief Extend visited node by an advancement notification
     */
    void stats_t::increment_visited_nodes() {
      if (_do_notifiy){
        if (_n_since_last==0){
          _n_since_last = -_n_notify;
          std::cout << _notify_string << std::to_string(_visited_nodes) << "delta t = " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()-t_last).count() << " ms." << std::endl;
          t_last = std::chrono::high_resolution_clock::now();
        }
        _n_since_last++;
      }
      tchecker::covreach::stats_t::increment_visited_nodes();
    }
    

  } // end of namespace covreach_ext
  
} // end of namespace tchecker_ext
