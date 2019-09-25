/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#include "tchecker_ext/algorithms/covreach_ext/run.hh"

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    void run(tchecker::parsing::system_declaration_t const & sysdecl,
             tchecker_ext::covreach_ext::options_t const & options,
             tchecker::log_t & log)
    {
      switch (options.search_order()) {
        case tchecker_ext::covreach_ext::options_t::BFS:
          tchecker_ext::covreach_ext::details::run<tchecker_ext::covreach_ext::threaded_fifo_waiting_t>(sysdecl, options, log);
          break;
        case tchecker_ext::covreach_ext::options_t::DFS:
          tchecker_ext::covreach_ext::details::run<tchecker_ext::covreach_ext::threaded_lifo_waiting_t>(sysdecl, options, log);
          break;
        default:
          log.error("Unsupported search order for multi-threaded covreach algorithm");
          break;
      }
    }
    
  } // end of namespace covreach_ext
  
} // end of namespace tchecker_ext
