/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#include <fstream>
#include <iterator>

#include <boost/tokenizer.hpp>

#include "tchecker_ext/utils/utils.hh"
#include "tchecker_ext/algorithms/covreach_ext/options.hh"

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    options_t::options_t(tchecker_ext::covreach_ext::options_t && options)
    : tchecker::covreach::options_t(static_cast<tchecker::covreach::options_t&&>(options)),
    _num_threads(options._num_threads),
    _n_notify(options._n_notify)
    {
      options._os = nullptr;
    }
    
    tchecker_ext::covreach_ext::options_t & options_t::operator= (tchecker_ext::covreach_ext::options_t && options)
    {
      tchecker::covreach::options_t::operator=(std::move(static_cast<tchecker::covreach::options_t&&>(options)));
      if (this != &options) {
        _num_threads = options._num_threads;
        _n_notify = options._n_notify;
      }
      return *this;
    }
    
    options_t::~options_t() = default;
  
    unsigned int options_t::num_threads() const
    {
      return _num_threads;
    }
  
    unsigned int options_t::n_notify() const
    {
      return _n_notify;
    }
    
    
    void options_t::set_option(std::string const & key, std::string const & value, tchecker::log_t & log)
    {
      if (key == "n"){
        set_n_notify(value, log);
      } else if (key == "t"){
        set_num_threads(value, log);
      }else{
        tchecker::covreach::options_t::set_option(key, value, log);
      }
    }
    
    void options_t::set_num_threads(std::string const &value, tchecker::log_t &log)
    {
      if (!tchecker_ext::utils::to_numeric(value, _num_threads)){
        log.error("Invalid value: " + value + " for command line option --threads, expecting an unsigned integer");
        throw std::runtime_error("Invalid value: " + value +
            " for command line option --threads, expecting an unsigned integer");
      }
      if(_num_threads==0){
        throw std::runtime_error("Number of threads needs to be strictly positive");
      }
    }
  
    void options_t::set_n_notify(std::string const &value, tchecker::log_t &log)
    {
      if (!tchecker_ext::utils::to_numeric(value, _n_notify)){
        log.error("Invalid value: " + value + " for command line option --notify, expecting an unsigned integer");
        throw std::runtime_error("Invalid value: " + value +
                                 " for command line option --notify, expecting an unsigned integer");
      }
    }
    
    void options_t::check_mandatory_options(tchecker::log_t & log) const
    {
      if (_algorithm_model == UNKNOWN)
        log.error("model must be set, use -m command line option");
    }
    
    
    std::ostream & options_t::describe(std::ostream & os)
    {
      tchecker::covreach::options_t::describe(os);
      os << "-t threads corresponds to the number of worker threads:" << std::endl;
      return os;
    }
    
  } // end of namespace covreach_ext
  
} // end of namespace tchecker_ext
