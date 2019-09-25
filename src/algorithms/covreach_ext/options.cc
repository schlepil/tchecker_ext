/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#include <fstream>
#include <iterator>

#include <boost/tokenizer.hpp>

#include "tchecker_ext/algorithms/covreach_ext/options.hh"

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    options_t::options_t(tchecker_ext::covreach_ext::options_t && options)
    : tchecker::covreach::options_t(static_cast<tchecker::covreach::options_t&&>(options)),
    _num_threads(options._num_threads)
    {
      options._os = nullptr;
    }
    
    tchecker_ext::covreach_ext::options_t & options_t::operator= (tchecker_ext::covreach_ext::options_t && options)
    {
      tchecker::covreach::options_t::operator=(std::move(static_cast<tchecker::covreach::options_t&&>(options)));
      if (this != &options) {
        _num_threads = options._num_threads;
      }
      return *this;
    }
    
    options_t::~options_t() = default;
    
    int options_t::num_threads() const
    {
      return _num_threads;
    }
    
    
    void options_t::set_option(std::string const & key, std::string const & value, tchecker::log_t & log)
    {
      if (key == "t"){
        set_num_threads(value, log);
      }else{
        tchecker::covreach::options_t::set_option(key, value, log);
      }
    }
    
    void options_t::set_num_threads(std::string const &value, tchecker::log_t &log)
    {
      for (auto c : value)
        if (! isdigit(c)) {
          log.error("Invalid value: " + value + " for command line option --table-size, expecting an unsigned integer");
          return;
        }
      
      _num_threads = std::stoi(value);
    }
    
    void options_t::check_mandatory_options(tchecker::log_t & log) const
    {
      if (_algorithm_model == UNKNOWN)
        log.error("model must be set, use -m command line option");
    }
    
    
    std::ostream & options_t::describe(std::ostream & os)
    {
      tchecker::covreach::options_t::describe(os);
      os << "-t threads       corresponds to the number of worker threads:" << std::endl;
      return os;
    }
    
  } // end of namespace covreach_ext
  
} // end of namespace tchecker_ext
