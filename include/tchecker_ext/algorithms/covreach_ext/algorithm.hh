/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_ALGORITHM_HH
#define TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_ALGORITHM_HH

#include <functional>
#include <iterator>
#include <tuple>
#include <vector>
#include <atomic>

#include "tchecker/basictypes.hh"
#include "tchecker/algorithms/covreach/builder.hh"

#include "tchecker_ext/algorithms/covreach_ext/graph.hh"
#include "tchecker_ext/algorithms/covreach_ext/stats.hh"

/*!
 \file algorithm.hh
 \brief Reachability algorithm with covering
 */

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    namespace threaded_working{
      // Forward decl.
      template <class BUILDER, class GRAPH, class STATS>
      void expand_node(typename GRAPH::node_ptr_t &node, BUILDER &builder, GRAPH &graph, std::vector<typename GRAPH::node_ptr_t> & nodes, STATS & stats);
      
      template <class GRAPH, class BUILDER, class WAITING, class ACCEPTING, class STATS>
      void worker_fun(GRAPH & graph, BUILDER & builder, WAITING & waiting, ACCEPTING & accepting, STATS & stats, std::atomic_bool & is_reached) {
        using node_ptr_t = typename GRAPH::node_ptr_t;
  
        node_ptr_t current_node;
        std::vector<node_ptr_t> next_nodes;
  
        // Stop if some other thread reached the label
        while (!is_reached && waiting.pop_and_increment(current_node)) {
          stats.increment_visited_nodes();
          // Check if done
          if (accepting(current_node)) {
            // set the done "flag"
            is_reached = true;
            // all work is done
            break;
          }
          // Building is thread safe as the allocation is
          // The nodes in next_nodes can currently only be accessed by this thread
    
          next_nodes.clear();
          tchecker_ext::covreach_ext::threaded_working::expand_node(current_node, builder, graph, next_nodes, stats);
    
          // This part is critical; graph sections have to be protected with locks
          graph.check_and_insert(current_node, next_nodes, stats);
    
          // next_nodes still holds all nodes, however is_active is correctly set for each element
          // active nodes will be automatically filtered
          waiting.insert_and_decrement(next_nodes);
          // Done
        }
      }
  
      template <class BUILDER, class GRAPH, class STATS>
      void expand_node(typename GRAPH::node_ptr_t &node, BUILDER &builder, GRAPH &graph, std::vector<typename GRAPH::node_ptr_t> & nodes, STATS & stats) {
        using node_ptr_t = typename GRAPH::node_ptr_t;
        using transition_ptr_t = typename GRAPH::ts_allocator_t::transition_ptr_t;
        
        node_ptr_t next_node{nullptr};
        transition_ptr_t transition{nullptr};
    
        auto outgoing_range = builder.outgoing(node);
        for (auto it = outgoing_range.begin(); !it.at_end(); ++it) {
          std::tie(next_node, transition) = *it;
          assert(next_node != node_ptr_t{nullptr});
          nodes.push_back(next_node);
        }
        // Check for each node if covered by some node
        for (auto &node_a : nodes) {
          for (auto &node_b : nodes) {
            if (node_a == node_b) {
              // Do not compare to self
              continue;
            }
            if (graph.is_le(node_a, node_b)) {
              node_a->make_inactive();
              // Once it is inactive -> continue
              stats.increment_directly_covered_leaf_nodes();
              break;
            }
          }
        }
      } //expand_node
      
    } // threaded_working
    
    /*!
     \class algorithm_t
     \brief Reachability algorithm with node covering
     \tparam TS : type of transition system, should derive from tchecker::ts::ts_t
     \tparam GRAPH : type of graph, should derive from tchecker::covreach::graph_t
     \tparam WAITING : type of waiting container, should derive from tchecker::covreach::active_waiting_t
     */
    template <class TS, class GRAPH, template <class NPTR> class WAITING>
    class algorithm_t {
      using ts_t = TS;
      using transition_t = typename GRAPH::ts_allocator_t::transition_t;
      using transition_ptr_t = typename GRAPH::ts_allocator_t::transition_ptr_t;
      using graph_t = GRAPH;
      using ts_allocator_t = typename GRAPH::ts_allocator_t;
      using node_ptr_t = typename GRAPH::node_ptr_t;
      using edge_ptr_t = typename GRAPH::edge_ptr_t;
      using builder_t = typename tchecker::covreach::builder_t<ts_t, ts_allocator_t>;
    public:
      /*!
       \brief Reachability algorithm with node covering
       \param ts : a transition system
       \param graph : a graph
       \param accepting : an accepting function over nodes
       \pre accepting is monotonous w.r.t. the ordering over nodes in graph: if a node is accepting,
       then any bigger node is accepting as well (partially checked by assertion)
       \post this algorithm visits ts and builds graph. Graph stores the maximal nodes in ts and edges
       between them. There are two kind of edges: actual edges which correspond to a transition in ts,
       and abstract edges. There is an abstract edge n1->n2 when the actual successor of n1 in ts is
       smaller than n2 (for some n2 in the graph). The order in which the states of ts are visited
       depends on the policy implemented by WAITING.
       The algorithms stops when an accepting node has been found, or when the graph has been entirely
       visited.
       \return ACCEPTING if TS has an accepting run, NON_ACCEPTING otherwise
       \note this algorithm may not terminate if graph is not finite
       */
      std::tuple<enum tchecker::covreach::outcome_t, tchecker_ext::covreach_ext::stats_t>
      run(TS & ts, GRAPH & graph, tchecker::covreach::accepting_condition_t<node_ptr_t> accepting, const long num_threads)
      {
        using accepting_t = tchecker::covreach::accepting_condition_t<node_ptr_t>;
        //using worker_t = tchecker_ext::covreach_ext::threaded_working::worker_t<ts_t, graph_t, WAITING>;
        
        builder_t builder(ts, graph.ts_allocator());
        WAITING<node_ptr_t> waiting;
        std::vector<node_ptr_t> nodes;
        std::vector<tchecker_ext::covreach_ext::stats_t> stats_vec;
        std::vector<std::thread> thread_vec;
        
        std::atomic_bool is_reached=false;
  
        // initial nodes
        // Set them before the threads are started
        nodes.clear();
        expand_initial_nodes(builder, graph, nodes);
        waiting.insert_and_decrement(nodes, false);
        
        // Now the actual work can start
        
        // Get one stat for each thread
        // template <class GRAPH, class BUILDER, class WAITING, class ACCEPTING, class STATS>
        //void worker_fun(GRAPH & graph, BUILDER & builder, WAITING & waiting, ACCEPTING & accepting, STATS & stats, std::atomic_bool & is_reached)
        for (long i=0; i<num_threads-1; ++i){
          stats_vec.emplace_back();
          thread_vec.emplace_back( tchecker_ext::covreach_ext::threaded_working::worker_fun<graph_t,
                                          builder_t, WAITING<node_ptr_t>, accepting_t, tchecker_ext::covreach_ext::stats_t>,
                                   std::ref(graph), std::ref(builder), std::ref(waiting), std::ref(accepting), std::ref(stats_vec[i]), std::ref(is_reached) );
        }
        
        // The last "thread" runs in the main thread
        // As this is blocking, we know when we are done
        
        stats_vec.emplace_back();
        tchecker_ext::covreach_ext::threaded_working::worker_fun<graph_t,
            builder_t, WAITING<node_ptr_t>, accepting_t, tchecker_ext::covreach_ext::stats_t>(graph, builder, waiting, accepting, stats_vec.back(), is_reached);
        
        // Wait till all are joined
        for (auto && it : thread_vec){
          it.join();
        }
  
        tchecker_ext::covreach_ext::stats_t stat_tot(stats_vec);
        
        return std::make_tuple(is_reached ? tchecker::covreach::REACHABLE : tchecker::covreach::UNREACHABLE, stat_tot);
      }
      
      /*!
       \brief Expand initial nodes
       \param builder : a transition system builder
       \param graph : a graph
       \param nodes : a vector of nodes
       \post the initial nodes provided by builder have been added to graph and to nodes
       */
      void expand_initial_nodes(tchecker::covreach::builder_t<TS, ts_allocator_t> & builder, GRAPH & graph,
                                std::vector<node_ptr_t> & nodes)
      {
        node_ptr_t node{nullptr};
        transition_ptr_t transition{nullptr};
        
        auto initial_range = builder.initial();
        for (auto it = initial_range.begin(); ! it.at_end(); ++it) {
          std::tie(node, transition) = *it;
          assert(node != node_ptr_t{nullptr});
          
          graph.add_node(node, GRAPH::ROOT_NODE);
          
          nodes.push_back(node);
        }
      }
    };
    
  } // end of namespace covreach
  
} // end of namespace tchecker

#endif // TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_ALGORITHM_HH



//template <class TS, class GRAPH, template <class NPTR> class WAITING>
//class worker_t{
//  using ts_t = TS;
//  using transition_t = typename GRAPH::ts_allocator_t::transition_t;
//  using transition_ptr_t = typename GRAPH::ts_allocator_t::transition_ptr_t;
//  using graph_t = GRAPH;
//  using ts_allocator_t = typename GRAPH::ts_allocator_t;
//  using node_ptr_t = typename GRAPH::node_ptr_t;
//  using edge_ptr_t = typename GRAPH::edge_ptr_t;
//  using waiting_t = WAITING<node_ptr_t>;
//
//public:
//  /*!
//   \brief Constructor
//   */
//  worker_t() = default;
//
//  /*!
//    \brief Function for worker thread
//    */
//  template <class BUILDER, class ACCEPTING, class STATS>
//  static void worker(graph_t & graph, BUILDER & builder, waiting_t & waiting, ACCEPTING & accepting, STATS & stats, std::atomic_bool & is_reached) {
//
//    node_ptr_t current_node;
//    std::vector<node_ptr_t> next_nodes;
//
//    // Stop if some other thread reached the label
//    while (!is_reached && waiting.pop_and_increment(current_node)) {
//      stats.increment_visited_nodes();
//      // Check if done
//      if (accepting(current_node)) {
//        // set the done "flag"
//        is_reached = true;
//        // all work is done
//        break;
//      }
//      // Building is thread safe as the allocation is
//      // The nodes in next_nodes can currently only be accessed by this thread
//
//      next_nodes.clear();
//      expand_node<BUILDER, STATS>(current_node, builder, graph, next_nodes, stats);
//
//      // This part is critical; graph sections have to be protected with locks
//      graph.check_and_insert(next_nodes, stats);
//
//      // next_nodes still holds all nodes, however is_active is correctly set for each element
//      // active nodes will be automatically filtered
//      waiting.insert_and_decrement(next_nodes);
//
//      // Done
//    }
//  }
//
//  /*!
//   \brief Expand node
//   \param node : a node
//   \param builder : a transition system builder
//   \param graph : a graph
//   \param nodes : a vector of nodes
//   \post the successor nodes of n provided by builder have been added to graph and to nodes
//   */
//  template <class BUILDER, class STATS>
//  static void expand_node(node_ptr_t &node, BUILDER &builder, GRAPH &graph,
//                          std::vector<node_ptr_t> & nodes, STATS & stats) {
//    node_ptr_t next_node{nullptr};
//    transition_ptr_t transition{nullptr};
//
//    auto outgoing_range = builder.outgoing(node);
//    for (auto it = outgoing_range.begin(); !it.at_end(); ++it) {
//      std::tie(next_node, transition) = *it;
//      assert(next_node != node_ptr_t{nullptr});
//      nodes.push_back(next_node);
//    }
//    // Check for each node if covered by some node
//    for (auto &node_a : nodes) {
//      for (auto &node_b : nodes) {
//        if (node_a == node_b) {
//          // Do not compare to self
//          continue;
//        }
//        if (graph.is_le(node_a, node_b)) {
//          node_a->make_inactive();
//          // Once it is inactive -> continue
//          stats.increment_directly_covered_leaf_nodes();
//          break;
//        }
//      }
//    }
//  }
//}; // worker_t