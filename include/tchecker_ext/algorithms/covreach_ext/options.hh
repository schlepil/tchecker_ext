/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_OPTIONS_HH
#define TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_OPTIONS_HH

#include "tchecker/algorithms/covreach/options.hh"

/*!
 \file options.hh
 \brief Options for covering reachability algorithm
 */

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    /*!
     \class options_t
     \brief Options for covering reachability algorithm
     */
    class options_t : public tchecker::covreach::options_t{
    public:
      
      /*!
       \brief Constructor
       \tparam MAP_ITERATOR : iterator on a map std::string -> std::string,
       should dereference to a pair of std::string (key, value)
       \param range : range (begin, end) of map iterators
       \param log : logging facility
       \post this has been built from range, and all errors/warnings have been
       reported to log
       */
      template <class MAP_ITERATOR>
      options_t(tchecker::range_t<MAP_ITERATOR> const & range, tchecker::log_t & log)
      : tchecker::covreach::options_t(),
        _num_threads(1),
        _n_notify(0)
      {
        auto it = range.begin(), end = range.end();
        for ( ; it != end; ++it )
          set_option(it->first, it->second, log);
        check_mandatory_options(log);
      }
      
      /*!
       \brief Copy constructor (deleted)
       */
      options_t(tchecker_ext::covreach_ext::options_t const & options) = delete;
      
      /*!
       \brief Move constructor
       */
      options_t(tchecker_ext::covreach_ext::options_t && options);
      
      /*!
       \brief Destructor
       */
      ~options_t();
      
      /*!
       \brief Assignment operator (deleted)
       */
      tchecker_ext::covreach_ext::options_t & operator= (tchecker_ext::covreach_ext::options_t const &) = delete;
      
      /*!
       \brief Move-assignment operator
       */
      tchecker_ext::covreach_ext::options_t & operator= (tchecker_ext::covreach_ext::options_t &&);
  
      /*!
       \brief Options description
       \param os : output stream
       \post A description of options for explore algorithm has been output to os
       \return os after output
       */
      static std::ostream & describe(std::ostream & os);
      
      /*!
       \brief Accessor
       \return number of threads
       */
      unsigned int num_threads() const;
  
      /*!
       \brief Accessor
       \return number of explorations before notification
       */
      unsigned int n_notify() const;
      
      /*!
       \brief Check that mandatory options have been set
       \param log : a logging facility
       \post All errors and warnings have been reported to log
       */
      void check_mandatory_options(tchecker::log_t & log) const;
      
      /*!
       \brief Short options string (getopt_long format)
       */
      static constexpr char const * const getopt_long_options = "c:f:hl:m:n:o:s:St";
      
      /*!
       \brief Long options (getopt_long format)
       */
      static constexpr struct option const getopt_long_options_long[]
      = {
        {"cover",        required_argument, 0, 'c'},
        {"format",       required_argument, 0, 'f'},
        {"help",         no_argument,       0, 'h'},
        {"labels",       required_argument, 0, 'l'},
        {"model",        required_argument, 0, 'm'},
        {"notify",       required_argument, 0, 'n'},
        {"output",       required_argument, 0, 'o'},
        {"search-order", required_argument, 0, 's'},
        {"stats",        no_argument,       0, 'S'},
        {"threads",      required_argument, 0, 't'},
        {"block-size",   required_argument, 0, 0},
        {"table-size",   required_argument, 0, 0},
        {0, 0, 0, 0}
      };
      
      
    protected:
      /*!
       \brief Set option
       \param key : option key
       \param value : option value
       \param log : logging facility
       \post option key has been set to value if key is a known option with expected value.
       An error has been reported to log if value is not admissible.
       A warning has been reported to log if key is not an option name.
       */
      void set_option(std::string const & key, std::string const & value, tchecker::log_t & log);
      
      /*!
       \brief Set number of threads
       \param value : option value
       \param log : logging facility
       \post number of threads is updated
       */
      void set_num_threads(std::string const & value, tchecker::log_t & log);
  
      /*!
       \brief Set n_notify
       \param value : option value
       \param log : logging facility
       \post n_notify is updated
       */
      void set_n_notify(std::string const & value, tchecker::log_t & log);
      
      unsigned int _num_threads; /*!< Number of worker threads */
      unsigned int _n_notify; /*! Number of states to explore before notifying */
    };
    
  } // end of namespace covreach_ext
  
} // end of namespace tchecker_ext

#endif // TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_OPTIONS_HH
