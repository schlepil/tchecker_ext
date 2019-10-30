/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_RUN_HH
#define TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_RUN_HH

#include "tchecker/algorithms/explore/run.hh"

#include "tchecker_ext/algorithms/explore_ext/options.hh"
#include "tchecker_ext/algorithms/explore_ext/algorithm.hh"

/*!
 \file run.hh
 \brief Running explore algorithm
 */

namespace tchecker_ext {
  
  namespace explore_ext {
    
    namespace details {
    
      //Currently no need to change algorithm model

    /*!
     \brief Run explore algorithm
     \tparam EXPLORED_MODEL : type of explored model
     \tparam GRAPH_OUTPUTTER : type of graph outputter
     \tparam WAITING : type of waiting container
     \param sysdecl : a system declaration
     \param options : explore algorithm options
     \param log : logging facility
     \post explore algorithm has been run on a model of sysdecl as defined by EXPLORED_MODEL
     and following options and the exploreation policy implented by WAITING. The graph has
     been output using GRAPH_OUPUTTER
     Every error and warning has been reported to log.
     */
    template<class EXPLORED_MODEL,template <class N, class E, class NO, class EO> class GRAPH_OUTPUTTER,template <class NPTR> class WAITING>
    void run(tchecker::parsing::system_declaration_t const & sysdecl,
             tchecker_ext::explore_ext::options_t const & options,
             tchecker::log_t & log)
    {
      using model_t = typename EXPLORED_MODEL::model_t;
      using ts_t = typename EXPLORED_MODEL::ts_t;
      using node_t = typename EXPLORED_MODEL::node_t;
      using edge_t = typename EXPLORED_MODEL::edge_t;
      using hash_t = tchecker::intrusive_shared_ptr_delegate_hash_t;
      using equal_to_t = tchecker::intrusive_shared_ptr_delegate_equal_to_t;
      using graph_allocator_t = typename EXPLORED_MODEL::graph_allocator_t;
      using node_outputter_t = typename EXPLORED_MODEL::node_outputter_t;
      using edge_outputter_t = typename EXPLORED_MODEL::edge_outputter_t;
      using graph_outputter_t = GRAPH_OUTPUTTER<node_t, edge_t, node_outputter_t, edge_outputter_t>;
      using graph_t = tchecker::explore::graph_t<graph_allocator_t, hash_t, equal_to_t, graph_outputter_t>;
      
      using node_ptr_t = typename graph_t::node_ptr_t;

      //using sol_graph_t = tchecker::post_proc::graph_t<typename graph_t::node_ptr_t, typename graph_t::edge_ptr_t, hash_t, equal_to_t>;

      model_t model(sysdecl, log);
      ts_t ts(model);

      tchecker::gc_t gc;

      graph_t graph(model.system().name(),
                    std::tuple<tchecker::gc_t &, std::tuple<model_t &, std::size_t>, std::tuple<>>
                            (gc, std::tuple<model_t &, std::size_t>(model, options.block_size()), std::make_tuple()),
                    options.output_stream(),
                    EXPLORED_MODEL::node_outputter_args(model),
                    EXPLORED_MODEL::edge_outputter_args(model));

      // Construct the accepting label
      tchecker::label_index_t label_index(model.system().labels());
      for (std::string const & label : options.accepting_labels()) {
        if (label_index.find_value(label) == label_index.end_value_map())
          label_index.add(label);
      }
  
      tchecker::covreach::accepting_labels_t<node_ptr_t> accepting_labels(label_index, options.accepting_labels());
  
      //sol_graph_t sol_graph; //For post processing
      std::vector<node_ptr_t> initial_nodes_vec; // Avoid having to reconstruct the initial nodes
      std::vector<node_ptr_t> accepting_nodes_vec; // Avoid having to reconstruct the initial nodes

      gc.start();

      tchecker_ext::explore_ext::algorithm_t<ts_t, graph_t, WAITING> algorithm;

      try {
          algorithm.run(ts, graph, initial_nodes_vec, accepting_nodes_vec, accepting_labels);
      }
      catch (...) {
          gc.stop();
          graph.clear();
          graph.free_all();
          throw;
      }

      //Do the post-processing
      // TODO
//      for (auto && it : init_nodes_ptr){
//        std::cout <<  it.refcount() << std::endl;
//        sol_graph.add_node(it, tchecker::post_proc::INIT);
//        std::cout <<  it.refcount() << std::endl;
//      }


      gc.stop();
      initial_nodes_vec.clear();
      accepting_nodes_vec.clear();
      graph.free_all();
    }
      
    /*!
     \brief Run explore algorithm
     \tparam GRAPH_OUTPUTTER : type of graph outputter
     \tparam WAITING : type of waiting container
     \param sysdecl : a system declaration
     \param log : logging facility
     \param options : explore algorithm options
     \post explore algorithm has been run on a model of sysdecl following options and
     the exploration policy implemented by WAITING. The graph has been output using
     GRAPH_OUPUTTER
     Every error and warning has been reported to log.
     */
    template <template <class N, class E, class NO, class EO> class GRAPH_OUTPUTTER, template <class NPTR> class WAITING>
    void run(tchecker::parsing::system_declaration_t const & sysdecl,
             tchecker_ext::explore_ext::options_t const & options,
             tchecker::log_t & log)
    {
      switch (options.explored_model()) {
        case tchecker::explore::options_t::FSM:
          tchecker_ext::explore_ext::details::run<tchecker::explore::details::fsm::explored_model_t, GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::TA:
          tchecker_ext::explore_ext::details::run<tchecker::explore::details::ta::explored_model_t, GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_NOEXTRA:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_no_extrapolation_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRAM_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraM_global_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRAM_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraM_local_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRAM_PLUS_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraMplus_global_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRAM_PLUS_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraMplus_local_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRALU_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraLU_global_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRALU_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraLU_local_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRALU_PLUS_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraLUplus_global_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_ELAPSED_EXTRALU_PLUS_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::elapsed_extraLUplus_local_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_NOEXTRA:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_no_extrapolation_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRAM_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraM_global_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRAM_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraM_local_t>, GRAPH_OUTPUTTER,
          WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRAM_PLUS_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraMplus_global_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRAM_PLUS_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraMplus_local_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRALU_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraLU_global_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRALU_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraLU_local_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRALU_PLUS_G:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraLUplus_global_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ZG_NON_ELAPSED_EXTRALU_PLUS_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::zg::ta::explored_model_t<tchecker::zg::ta::non_elapsed_extraLU_local_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ASYNC_ZG_ELAPSED_EXTRALU_PLUS_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::async_zg::ta::explored_model_t<tchecker::async_zg::ta::elapsed_extraLUplus_local_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        case tchecker::explore::options_t::ASYNC_ZG_NON_ELAPSED_EXTRALU_PLUS_L:
          tchecker_ext::explore_ext::details::run
          <tchecker::explore::details::async_zg::ta::explored_model_t<tchecker::async_zg::ta::non_elapsed_extraLUplus_local_t>,
          GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
          break;
        default:
          log.error("unsupported explored model");
      }
    }
      
      
      
      
      
      /*!
       \brief Run explore algorithm
       \tparam WAITING : type of waiting container
       \param sysdecl : a system declaration
       \param options : explore algorithm options
       \param log : logging facility
       \post explore algorithm has been run on a model of sysdecl following options and
       the exploration policy implemented by WAITING
       Every error and warning has been reported to log.
       */
      template <template <class NPTR> class WAITING>
      void run(tchecker::parsing::system_declaration_t const & sysdecl,
               tchecker_ext::explore_ext::options_t const & options,
               tchecker::log_t & log)
      {
        switch (options.output_format()) {
          case tchecker::explore::options_t::DOT:
            tchecker_ext::explore_ext::details::run<tchecker::graph::dot_outputter_t, WAITING>(sysdecl, options, log);
            break;
          case tchecker::explore::options_t::RAW:
            tchecker_ext::explore_ext::details::run<tchecker::graph::raw_outputter_t, WAITING>(sysdecl, options, log);
            break;
          default:
            log.error("unsupported output format");
        }
      }
      
    } // end of namespace details
    
    
    
    
    /*!
     \brief Run explore algorithm
     \param sysdecl : a system declaration
     \param options : explore algorithm options
     \param log : logging facility
     \post explore algorithm has been run on a model of sysdecl following options.
     Every error and warning has been reported to log.
     */
    void run(tchecker::parsing::system_declaration_t const & sysdecl,
             tchecker_ext::explore_ext::options_t const & options,
             tchecker::log_t & log);
    
  } // end of namespace explore_ext
  
} // end of namespace tchecker_ext

#endif // TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_RUN_HH
