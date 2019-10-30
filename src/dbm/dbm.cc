//
// Created by philipp on 02.10.19.
//

#include "tchecker_ext/dbm/dbm.hh"

#include <tchecker_ext/config.hh>

namespace tchecker_ext{
  namespace dbm_ext{

#define DBM(i,j)          dbm[(i)*dim1+(j)]
#define DBM1(i,j)         dbm1[(i)*dim1+(j)]
#define DBM2(i,j)         dbm2[(i)*dim2+(j)]
  
    enum tchecker::dbm::status_t partial_intersection(tchecker::dbm::db_t * dbm,
                                              tchecker::dbm::db_t const * dbm1,
                                              tchecker::dbm::db_t const * dbm2,
                                              tchecker::clock_id_t dim1,
                                              tchecker::clock_id_t dim2,
                                              tchecker::clock_id_t const * idx_clk){
      assert(dim1 >= 1);
      assert(dim2>=dim1);
      assert(dbm != nullptr);
      assert(dbm1 != nullptr);
      assert(dbm2 != nullptr);
      assert(idx_clk != nullptr);
      assert(tchecker::dbm::is_consistent(dbm1, dim1));
      assert(tchecker::dbm::is_consistent(dbm2, dim2));
      assert(tchecker::dbm::is_tight(dbm1, dim1));
      assert(tchecker::dbm::is_tight(dbm2, dim2));
#if (SCHLEPIL_DBG>=2)
      for (tchecker::clock_id_t i = 0; i < dim1; ++i){
        assert(idx_clk[i]<dim2);
      }
#endif
      tchecker::clock_id_t i2,j2;
  
      for (tchecker::clock_id_t i = 0; i < dim1; ++i) {
        i2 = idx_clk[i];
        for (tchecker::clock_id_t j = 0; j < dim1; ++j){
          j2 = idx_clk[j];
          DBM(i, j) = tchecker::dbm::min(DBM1(i, j), DBM2(i2, j2));
        } // j
      } // i
  
      return tchecker::dbm::tighten(dbm, dim1);
    } // partial_intersection
    
  }
}