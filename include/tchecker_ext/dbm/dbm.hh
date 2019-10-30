//
// Created by philipp on 02.10.19.
//

#ifndef TCHECKER_EXT_DBM_HH
#define TCHECKER_EXT_DBM_HH

#include "tchecker/dbm/dbm.hh"

namespace tchecker_ext{
  namespace dbm_ext{
  
    /*!
     \brief Partial Intersection
     \param dbm : a dbm of size1
     \param dbm1 : a dbm of size1
     \param dbm2 : a dbm of size2>=size1
     \param dim1 : dimension of dbm and dbm1
     \param idx_clk : array of clock index which are to be compared
     \pre dbm, dbm1 and dbm2 are not nullptr (checked by assertion)
     dbm, dbm1 and dbm2 are dim1*dim1 | size2*size2 arrays of difference bounds
     dbm1 and dbm2 are consistent (checked by assertion)
     dbm1 and dbm2 are tight (checked by assertion)
     dim >= 1 (checked by assertion).
     \post dbm is the partial intersection of dbm1 and dbm2
     dbm is consistent
     dbm is tight
     \return EMPTY if the intersection of dbm1 and dbm2 is empty, NON_EMPTY otherwise
     \note dbm can be one of dbm1 or dbm2
     */
    enum tchecker::dbm::status_t partial_intersection(tchecker::dbm::db_t * dbm,
                                              tchecker::dbm::db_t const * dbm1,
                                              tchecker::dbm::db_t const * dbm2,
                                              tchecker::clock_id_t dim1,
                                              tchecker::clock_id_t dim2,
                                              tchecker::clock_id_t const * idx_clk);
    
  }
}

#endif //TCHECKER_EXT_DBM_HH
