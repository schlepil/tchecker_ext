/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#include <boost/tokenizer.hpp>

#include "tchecker_ext/algorithms/explore_ext/options.hh"

namespace tchecker_ext {
  
  namespace explore_ext {
    
    options_t::options_t(tchecker_ext::explore_ext::options_t && options)
    : tchecker::explore::options_t(std::move(options)),
    _accepting_labels(std::move(options._accepting_labels))
    {
      options._os = nullptr;
    }
    
    
    options_t::~options_t()
    {
      _os->flush();
      if (_os != &std::cout)
        delete _os;
    }
    
    tchecker_ext::explore_ext::options_t & options_t::operator= (tchecker_ext::explore_ext::options_t && options)
    {
      tchecker::explore::options_t::operator=(std::move(options));
      if (this != &options) {
        _accepting_labels = std::move(options._accepting_labels);
      }
      return *this;
    }
  
    tchecker::covreach::options_t::accepting_labels_range_t options_t::accepting_labels() const
    {
      return tchecker::make_range(_accepting_labels);
    }

    void options_t::set_option(std::string const & key, std::string const & value, tchecker::log_t & log)
    {
      if (key == "l")
        set_accepting_labels(value, log);
      else
        tchecker::explore::options_t::set_option(key, value, log);
    }

    void options_t::set_accepting_labels(std::string const & value, tchecker::log_t & log)
    {
      boost::char_separator<char> sep(":");
      boost::tokenizer<boost::char_separator<char> > tokenizer(value, sep);
      auto it = tokenizer.begin(), end = tokenizer.end();
      for ( ; it != end; ++it)
        _accepting_labels.push_back(*it);
    }
    
    
    void options_t::check_mandatory_options(tchecker::log_t & log) const {
      tchecker::explore::options_t::check_mandatory_options(log);
      if ( _accepting_labels.size() == 0){
        log.error("No labels provided");
      }
    }
    
    std::ostream & options_t::describe(std::ostream & os)
    {
      os << "-f (dot|raw)     output format (graphviz DOT format or raw format)" << std::endl;
      os << "-h               this help screen" << std::endl;
      os << "-m model         where model is one of the following:" << std::endl;
      os << "                 fsm                          finite-state machine" << std::endl;
      os << "                 ta                           timed automaton" << std::endl;
      os << "                 zg:semantics:extrapolation   zone graph with:" << std::endl;
      os << "                   semantics:      elapsed        time-elapsed semantics" << std::endl;
      os << "                                   non-elapsed    non time-elapsed semantics" << std::endl;
      os << "                   extrapolation:  NOextra   no zone extrapolation" << std::endl;
      os << "                                   extraMg   ExtraM with global clock bounds" << std::endl;
      os << "                                   extraMl   ExtraM with local clock bounds" << std::endl;
      os << "                                   extraM+g  ExtraM+ with global clock bounds" << std::endl;
      os << "                                   extraM+l  ExtraM+ with local clock bounds" << std::endl;
      os << "                                   extraLUg  ExtraLU with global clock bounds" << std::endl;
      os << "                                   extraLUl  ExtraLU with local clock bounds" << std::endl;
      os << "                                   extraLU+g ExtraLU+ with global clock bounds" << std::endl;
      os << "                                   extraLU+l ExtraLU+ with local clock bounds" << std::endl;
      os << "                 async_zg:semantics            asynchronous zone graph with:" << std::endl;
      os << "                   semantics:      elapsed         time-elapsed semantics"    << std::endl;
      os << "                                   non-elapsed     non time-elapsed semantics" << std::endl;
      os << "-o filename      output graph to filename" << std::endl;
      os << "-s (bfs|dfs)     search order (breadth-first search or depth-first search)" << std::endl;
      os << "--block-size n   size of an allocation block (number of allocated objects)" << std::endl;
      os << std::endl;
      os << "Default parameters: -f raw -s dfs --block-size 10000, output to standard output" << std::endl;
      os << "                    -m must be specified" << std::endl;
      return os;
    }
    
  } // end of namespace explore_ext
  
} // end of namespace tchecker
