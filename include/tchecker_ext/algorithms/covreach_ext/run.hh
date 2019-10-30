/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_RUN_HH
#define TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_RUN_HH
/*
#include "tchecker/algorithms/covreach/accepting.hh"
#include "tchecker/algorithms/covreach/algorithm.hh"
#include "tchecker/algorithms/covreach/cover.hh"
#include "tchecker/algorithms/covreach/graph.hh"
#include "tchecker/algorithms/covreach/options.hh"
#include "tchecker/async_zg/async_zg_ta.hh"
#include "tchecker/algorithms/covreach/output.hh"
#include "tchecker/algorithms/covreach/stats.hh"
#include "tchecker/graph/allocators.hh"
#include "tchecker/graph/output.hh"
#include "tchecker/parsing/declaration.hh"
#include "tchecker/ts/allocators.hh"
#include "tchecker/utils/gc.hh"
#include "tchecker/utils/log.hh"
#include "tchecker/zg/zg_ta.hh"
*/

#include "tchecker/algorithms/covreach/run.hh"
#include "tchecker_ext/algorithms/covreach_ext/options.hh"
#include "tchecker_ext/algorithms/covreach_ext/algorithm.hh"
#include "tchecker_ext/algorithms/covreach_ext/graph.hh"

#include "tchecker_ext/algorithms/covreach_ext/allocator.hh"


/*!
 \file run.hh
 \brief Running explore algorithm
 */

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    namespace details {
      
      namespace zg {
        
        namespace ta {
          
          /*!
           \class algorithm_model_t
           \brief Model for covering reachability over zone graphs of timed automata
           */
          template <class ZONE_SEMANTICS>
          class algorithm_model_t: public tchecker::covreach::details::zg::ta::algorithm_model_t<ZONE_SEMANTICS>{
          public:
            
            using ts_allocator_t = tchecker_ext::threaded_ts::allocator_t<
                typename tchecker::covreach::details::zg::ta::algorithm_model_t<ZONE_SEMANTICS>::node_allocator_t,
                typename tchecker::covreach::details::zg::ta::algorithm_model_t<ZONE_SEMANTICS>::transition_allocator_t>;
            
            using graph_t  = tchecker_ext::covreach_ext::graph_t<typename tchecker::covreach::details::zg::ta::algorithm_model_t<ZONE_SEMANTICS>::key_t,
                                                                 typename tchecker::covreach::details::zg::ta::algorithm_model_t<ZONE_SEMANTICS>::ts_t,
                                                                 ts_allocator_t>;
            
            using builder_allocator_t =
                tchecker_ext::threaded_ts::threaded_builder_allocator_t<ts_allocator_t >;
          };

        } // end of namespace ta
        
      } // end of namespace zg
      
      
      
      
      namespace async_zg {
        
        namespace ta {
          
          /*!
           \class algorithm_model_t
           \brief Model for covering reachability over asynchronous zone graphs of timed automata
           */
          template <class ZONE_SEMANTICS>
          class algorithm_model_t: public tchecker::covreach::details::async_zg::ta::algorithm_model_t<ZONE_SEMANTICS>{
          public:
  
            using ts_allocator_t = tchecker_ext::threaded_ts::allocator_t<
                typename tchecker::covreach::details::async_zg::ta::algorithm_model_t<ZONE_SEMANTICS>::node_allocator_t,
                typename tchecker::covreach::details::async_zg::ta::algorithm_model_t<ZONE_SEMANTICS>::transition_allocator_t>;
            
            using graph_t  = tchecker_ext::covreach_ext::graph_t<typename tchecker::covreach::details::async_zg::ta::algorithm_model_t<ZONE_SEMANTICS>::key_t,
                                                                 typename tchecker::covreach::details::async_zg::ta::algorithm_model_t<ZONE_SEMANTICS>::ts_t,
                                                                 ts_allocator_t>;
  
            using helper_allocator_t =
              tchecker_ext::threaded_ts::threaded_builder_allocator_t<ts_allocator_t>;
          };
          
        } // end of namespace ta
        
      } // end of namespace async_zg
      
      
      
      
      /*!
       \brief Run covering reachability algorithm
       \tparam ALGORITHM_MODEL : type of algorithm model
       \tparam GRAPH_OUTPUTTER : type of graph outputter
       \tparam WAITING : type of waiting container
       \param sysdecl : a system declaration
       \param options : covering reachability algorithm options
       \param log : logging facility
       \post covering reachability algorithm has been run on a model of sysdecl as defined by
       ALGORITHM_MODEL and following options and the exploreation policy implented by WAITING.
       The graph has been output using GRAPH_OUPUTTER
       Every error and warning has been reported to log.
       */
      template
      <template <class NODE_PTR, class STATE_PREDICATE> class COVER_NODE,
      class ALGORITHM_MODEL,
      template <class N, class E, class NO, class EO> class GRAPH_OUTPUTTER,
      template <class NPTR> class WAITING
      >
      void run(tchecker::parsing::system_declaration_t const & sysdecl,
               tchecker_ext::covreach_ext::options_t const & options,
               tchecker::log_t & log)
      {
        using model_t = typename ALGORITHM_MODEL::model_t;
        using ts_t = typename ALGORITHM_MODEL::ts_t;
        using graph_t = typename ALGORITHM_MODEL::graph_t;
        using node_ptr_t = typename ALGORITHM_MODEL::node_ptr_t;
        using state_predicate_t = typename ALGORITHM_MODEL::state_predicate_t;
        using cover_node_t = COVER_NODE<node_ptr_t, state_predicate_t>;
        
        using builder_allocator_t = typename ALGORITHM_MODEL::builder_allocator_t;
        
        std::cout << "Building first model";
        model_t model(sysdecl, log);
        ts_t ts(model);
        std::cout << "Done" << std::endl;
        
        std::cout << model.local_lu_map() << std::endl << "Done 2 " << std::endl;
        
        //std::deque<model_t> model_vec;
        std::deque<ts_t> ts_vec; // Create multiple instances to have multiple vm's
  
        for (int i=0; i<options.num_threads(); ++i){
          //model_vec.emplace_back(sysdecl, log);
          //ts_vec.emplace_back(model_vec.back());
          ts_vec.push_back(ts);
          std::cout << "transition sys nr " << i << " : " << &ts_vec.back() << std::endl;
          //std::cout << ts_vec.back()._zg._zone_semantics._local_lu_map << std::endl;
          //ts_vec.push_back(ts);
        }
        
        cover_node_t cover_node(ALGORITHM_MODEL::state_predicate_args(model), ALGORITHM_MODEL::zone_predicate_args(model));
        
        tchecker::label_index_t label_index(model.system().labels());
        for (std::string const & label : options.accepting_labels()) {
          if (label_index.find_value(label) == label_index.end_value_map())
            label_index.add(label);
        }
  
        tchecker::covreach::accepting_labels_t<node_ptr_t> accepting_labels(label_index, options.accepting_labels()); //Create one accepting for each thread
        
        tchecker::gc_t gc;
        
        graph_t graph(gc,
                      std::tuple<tchecker::gc_t &, std::tuple<model_t &, std::size_t>, std::tuple<>>
                      (gc, std::tuple<model_t &, std::size_t>(model, options.block_size()), std::make_tuple()),
                      options.block_size(),
                      options.nodes_table_size(),
                      ALGORITHM_MODEL::node_to_key,
                      cover_node);
        
        // Construct the helper allocator
        // Each builder allocator has its own transition (singleton) allocator, but all share the
        // node allocator with the graph
        std::deque<builder_allocator_t> builder_alloc_vec;
        for (int i=0; i<options.num_threads(); ++i){
          builder_alloc_vec.emplace_back(gc, graph.ts_allocator(), std::make_tuple());
        }
        
        gc.start();
        
        enum tchecker::covreach::outcome_t outcome;
        tchecker_ext::covreach_ext::stats_t stats;
        tchecker_ext::covreach_ext::algorithm_t<ts_t, builder_allocator_t , graph_t, WAITING> algorithm;
        
        try {
          std::tie(outcome, stats) = algorithm.run(ts_vec, builder_alloc_vec, graph, accepting_labels, options.num_threads());
        }
        catch (...) {
          gc.stop();
          graph.clear();
          graph.free_all();
//          for (int i=0; i<options.num_threads(); ++i){
//            delete ts_vec[i];
//          }
          throw;
        }
        
        std::cout << "REACHABLE " << (outcome == tchecker::covreach::REACHABLE ? "true" : "false") << std::endl;
        
        if (1 || options.stats()) {
          std::cout << "STORED_NODES " << graph.nodes_count() << std::endl;
          std::cout << stats << std::endl;
        }
        
        if (options.output_format() == tchecker_ext::covreach_ext::options_t::DOT) {
          tchecker::covreach::dot_outputter_t<typename ALGORITHM_MODEL::node_outputter_t>
          dot_outputter(ALGORITHM_MODEL::node_outputter_args(model));
          dot_outputter.template output<graph_t, tchecker::instrusive_shared_ptr_hash_t>(options.output_stream(),
                                                                                         graph,
                                                                                         model.system().name());
        }
        
        gc.stop();
        graph.clear();
        graph.free_all();
//        for (int i=0; i<options.num_threads(); ++i){
//          delete ts_vec[i];
//        }
      }
      
      
      /*!
       \brief Run covering reachability algorithm for asynchronous zone graphs
       \tparam ALGORITHM_MODEL : type of algorithm model
       \tparam GRAPH_OUTPUTTER : type of graph outputter
       \tparam WAITING : type of waiting container
       \param sysdecl : a system declaration
       \param options : covering reachability algorithm options
       \param log : logging facility
       \post covering reachability algorithm has been run on a model of sysdecl as defined by ALGORITHM_MODEL
       and following options and the exploreation policy implented by WAITING. The graps has
       been output using GRAPH_OUPUTTER
       Every error and warning has been reported to log.
       */
      template
      <class ALGORITHM_MODEL,
      template <class N, class E, class NO, class EO> class GRAPH_OUTPUTTER,
      template <class NPTR> class WAITING
      >
      void run_async_zg(tchecker::parsing::system_declaration_t const & sysdecl,
                        tchecker_ext::covreach_ext::options_t const & options,
                        tchecker::log_t & log)
      {
        if (options.node_covering() == tchecker::covreach::options_t::INCLUSION)
          tchecker_ext::covreach_ext::details::run<tchecker::covreach::cover_sync_inclusion_t, ALGORITHM_MODEL, GRAPH_OUTPUTTER, WAITING>
          (sysdecl, options, log);
        else
          log.error("Unsupported node covering");
      }
      
      
      /*!
       \brief Run covering reachability algorithm for zone graphs
       \tparam ALGORITHM_MODEL : type of algorithm model
       \tparam GRAPH_OUTPUTTER : type of graph outputter
       \tparam WAITING : type of waiting container
       \param sysdecl : a system declaration
       \param options : covering reachability algorithm options
       \param log : logging facility
       \post covering reachability algorithm has been run on a model of sysdecl as defined by ALGORITHM_MODEL
       and following options and the exploreation policy implented by WAITING. The graps has
       been output using GRAPH_OUPUTTER
       Every error and warning has been reported to log.
       */
      template
      <class ALGORITHM_MODEL,
      template <class N, class E, class NO, class EO> class GRAPH_OUTPUTTER,
      template <class NPTR> class WAITING
      >
      void run_zg(tchecker::parsing::system_declaration_t const & sysdecl,
                  tchecker_ext::covreach_ext::options_t const & options,
                  tchecker::log_t & log)
      {
        switch (options.node_covering()) {
          case tchecker::covreach::options_t::INCLUSION:
            tchecker_ext::covreach_ext::details::run<tchecker::covreach::cover_inclusion_t, ALGORITHM_MODEL, GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ALU_G:
            tchecker_ext::covreach_ext::details::run<tchecker::covreach::cover_alu_global_t, ALGORITHM_MODEL, GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ALU_L:
            tchecker_ext::covreach_ext::details::run<tchecker::covreach::cover_alu_local_t, ALGORITHM_MODEL, GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::AM_G:
            tchecker_ext::covreach_ext::details::run<tchecker::covreach::cover_am_global_t, ALGORITHM_MODEL, GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::AM_L:
            tchecker_ext::covreach_ext::details::run<tchecker::covreach::cover_am_local_t, ALGORITHM_MODEL, GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          default:
            log.error("unsupported node covering");
        }
      }
      
      
      /*!
       \brief Run covering reachability algorithm
       \tparam GRAPH_OUTPUTTER : type of graph outputter
       \tparam WAITING : type of waiting container
       \param sysdecl : a system declaration
       \param log : logging facility
       \param options : covering reachability algorithm options
       \post covering reachability algorithm has been run on a model of sysdecl following options and
       the exploration policy implemented by WAITING. The graph has been output using
       GRAPH_OUPUTTER
       Every error and warning has been reported to log.
       */
      template <template <class N, class E, class NO, class EO> class GRAPH_OUTPUTTER, template <class NPTR> class WAITING>
      void run(tchecker::parsing::system_declaration_t const & sysdecl,
               tchecker_ext::covreach_ext::options_t const & options,
               tchecker::log_t & log)
      {
        switch (options.algorithm_model()) {
//          case tchecker::covreach::options_t::ASYNC_ZG_ELAPSED_EXTRALU_PLUS_L:
//            tchecker_ext::covreach_ext::details::run_async_zg
//            <tchecker_ext::covreach_ext::details::async_zg::ta::algorithm_model_t<tchecker::async_zg::ta::elapsed_extraLUplus_local_t>,
//            GRAPH_OUTPUTTER, WAITING>
//            (sysdecl, options, log);
//            break;
//          case tchecker::covreach::options_t::ASYNC_ZG_NON_ELAPSED_EXTRALU_PLUS_L:
//            tchecker_ext::covreach_ext::details::run_async_zg
//            <tchecker_ext::covreach_ext::details::async_zg::ta::algorithm_model_t<tchecker::async_zg::ta::non_elapsed_extraLUplus_local_t>,
//            GRAPH_OUTPUTTER, WAITING>
//            (sysdecl, options, log);
//            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_NOEXTRA:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_no_extrapolation_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRAM_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraM_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRAM_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraM_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRAM_PLUS_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraMplus_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRAM_PLUS_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraMplus_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRALU_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraLU_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRALU_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraLU_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRALU_PLUS_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraLUplus_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_ELAPSED_EXTRALU_PLUS_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::elapsed_extraLUplus_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_NOEXTRA:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_no_extrapolation_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRAM_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraM_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRAM_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraM_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRAM_PLUS_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraMplus_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRAM_PLUS_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraMplus_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRALU_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraLU_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRALU_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraLU_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRALU_PLUS_G:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraLUplus_global_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::ZG_NON_ELAPSED_EXTRALU_PLUS_L:
            tchecker_ext::covreach_ext::details::run_zg
            <tchecker_ext::covreach_ext::details::zg::ta::algorithm_model_t<tchecker::zg::ta::non_elapsed_extraLU_local_t>,
            GRAPH_OUTPUTTER, WAITING>
            (sysdecl, options, log);
            break;
          default:
            log.error("unsupported model");
        }
      }
      
      
      
      
      
      /*!
       \brief Run covering reachability algorithm
       \tparam WAITING : type of waiting container
       \param sysdecl : a system declaration
       \param options : covering reachability algorithm options
       \param log : logging facility
       \post covering reachability algorithm has been run on a model of sysdecl following options and
       the exploration policy implemented by WAITING
       Every error and warning has been reported to log.
       */
      template <template <class NPTR> class WAITING>
      void run(tchecker::parsing::system_declaration_t const & sysdecl,
               tchecker_ext::covreach_ext::options_t const & options,
               tchecker::log_t & log)
      {
        switch (options.output_format()) {
          case tchecker::covreach::options_t::DOT:
            tchecker_ext::covreach_ext::details::run<tchecker::graph::dot_outputter_t, WAITING>(sysdecl, options, log);
            break;
          case tchecker::covreach::options_t::RAW:
            tchecker_ext::covreach_ext::details::run<tchecker::graph::raw_outputter_t, WAITING>(sysdecl, options, log);
            break;
          default:
            log.error("unsupported output format");
        }
      }
      
    } // end of namespace details
    
    
    
    
    /*!
     \brief Run covering reachability algorithm
     \param sysdecl : a system declaration
     \param options : covering reachability algorithm options
     \param log : logging facility
     \post covering reachability algorithm has been run on a model of sysdecl following options.
     Every error and warning has been reported to log.
     */
    void run(tchecker::parsing::system_declaration_t const & sysdecl,
             tchecker_ext::covreach_ext::options_t const & options,
             tchecker::log_t & log);
    
  } // end of namespace covreach
  
} // end of namespace tchecker

#endif // TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_RUN_HH
