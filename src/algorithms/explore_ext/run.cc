/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#include "tchecker_ext/algorithms/explore_ext/run.hh"
#include "tchecker/utils/waiting.hh"

namespace tchecker_ext {
  
  namespace explore_ext {
    
    void run(tchecker::parsing::system_declaration_t const & sysdecl,
             tchecker_ext::explore_ext::options_t const & options,
             tchecker::log_t & log)
    {
      switch (options.search_order()) {
        case tchecker::explore::options_t::BFS:
          tchecker_ext::explore_ext::details::run<tchecker::fifo_waiting_t>(sysdecl, options, log);
          break;
        case tchecker::explore::options_t::DFS:
          tchecker_ext::explore_ext::details::run<tchecker::lifo_waiting_t>(sysdecl, options, log);
          break;
        default:
          log.error("Unsupported search order for explore algorithm");
          break;
      }
    }
    
  } // end of namespace explore
  
} // end of namespace tchecker
