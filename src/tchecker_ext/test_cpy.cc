//
// Created by philipp on 30.09.19.
//

#include <iostream>

#include "tchecker/parsing/parsing.hh"
#include "tchecker/ts/allocators.hh"
#include "tchecker/utils/gc.hh"
#include "tchecker/utils/log.hh"
#include "tchecker/zg/zg_ta.hh"


int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " filename " << std::endl;
    return 1;
  }
  
  tchecker::log_t log(&std::cerr);
  
  tchecker::parsing::system_declaration_t const * sysdecl = nullptr;
  try {
    sysdecl = tchecker::parsing::parse_system_declaration(argv[1], log);
    if (sysdecl == nullptr)
      throw std::runtime_error("System declaration cannot be built");
    
    tchecker::zg::ta::model_t model(*sysdecl, log);
    
    using zone_graph_t = tchecker::zg::ta::elapsed_extraLUplus_local_t;
    using ts_t = zone_graph_t::ts_t;
    const ts_t ts(model);
    ts_t ts2(ts);
    
    
    
    using state_t = zone_graph_t::shared_state_t;
    using transition_t = zone_graph_t::transition_t;
    using state_allocator_t = zone_graph_t::state_pool_allocator_t<state_t>;
    using transition_allocator_t = zone_graph_t::transition_singleton_allocator_t<transition_t>;

    tchecker::gc_t gc;

    using allocator_t = tchecker::ts::allocator_t<state_allocator_t, transition_allocator_t>;
    allocator_t allocator(gc, std::make_tuple(model, 100000), std::make_tuple());

    gc.start();
    gc.stop();

    std::cout << *sysdecl << std::endl;
  }
  catch (std::exception const & e) {
    log.error(e.what());
  }
  delete sysdecl;
  
  return 0;
}