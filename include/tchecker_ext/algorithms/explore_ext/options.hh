/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_OPTIONS_HH
#define TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_OPTIONS_HH

#include "tchecker/algorithms/explore/options.hh"
#include "tchecker/algorithms/covreach/options.hh"


/*!
 \file options.hh
 \brief Options for labeled explore algorithm
 */
// TODO change later to inherit most and only change the things necessary
namespace tchecker_ext {
  
  namespace explore_ext {
    
    /*!
     \class options_t
     \brief Options for labeled explore algorithm
     */
    class options_t: public tchecker::explore::options_t {
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
      : tchecker::explore::options_t() //Empty range
      {
        auto it = range.begin(), end = range.end();
        for ( ; it != end; ++it )
          set_option(it->first, it->second, log);
        check_mandatory_options(log);
      }
      
      /*!
       \brief Copy constructor (deleted)
       */
      options_t(tchecker_ext::explore_ext::options_t const & options) = delete;
      
      /*!
       \brief Move constructor
       */
      options_t(tchecker_ext::explore_ext::options_t && options);
      
      /*!
       \brief Destructor
       */
      ~options_t();
      
      /*!
       \brief Assignment operator (deleted)
       */
      tchecker_ext::explore_ext::options_t & operator= (tchecker_ext::explore_ext::options_t const &) = delete;
      
      /*!
       \brief Move-assignment operator
       */
      tchecker_ext::explore_ext::options_t & operator= (tchecker_ext::explore_ext::options_t &&);
      
      /*!
       \bief Type of range of accepting labels
       */
      using accepting_labels_range_t = tchecker::range_t<std::vector<std::string>::const_iterator>;

      /*!
       \brief Accessor
       \return accepting labels
       */
      tchecker::covreach::options_t::accepting_labels_range_t accepting_labels() const;
      
      /*!
       \brief Check that mandatory options have been set
       \param log : a logging facility
       \post All errors and warnings have been reported to log
       */
      void check_mandatory_options(tchecker::log_t & log) const;
      
      /*!
       \brief Short options string (getopt_long format)
       */
      static constexpr char const * const getopt_long_options = "f:h:l:m:o:s";
      
      /*!
       \brief Long options (getopt_long format)
       */
      static constexpr struct option const getopt_long_options_long[]
      = {
        {"format",       required_argument, 0, 'f'},
        {"help",         no_argument,       0, 'h'},
        {"labels",       required_argument, 0, 'l'},
        {"model",        required_argument, 0, 'm'},
        {"output",       required_argument, 0, 'o'},
        {"search-order", required_argument, 0, 's'},
        {"block-size",   required_argument, 0, 0},
        {0, 0, 0, 0}
      };
      
      /*!
       \brief Options description
       \param os : output stream
       \post A description of options for explore algorithm has been output to os
       \return os after output
       */
      static std::ostream & describe(std::ostream & os);
      
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
       \brief Set accepting labels
       \param value : option value
       \param log : logging facility
       \post accepting labels have been set to value.
       An error has been reported to log if value is not admissible.
       */
      void set_accepting_labels(std::string const & value, tchecker::log_t & log);
      
      std::vector<std::string> _accepting_labels;  /*!< Accepting labels */
    }; // options_t
    
  } // end of namespace explore_ext
  
} // end of namespace tchecker_ext

#endif // TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_OPTIONS_HH
