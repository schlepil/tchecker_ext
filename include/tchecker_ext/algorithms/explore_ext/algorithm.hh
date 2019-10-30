/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_ALGORITHM_HH
#define TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_ALGORITHM_HH

#include <tuple>
#include <vector>

#include "tchecker/basictypes.hh"
#include "tchecker/algorithms/explore/builder.hh"


#include "tchecker/algorithms/covreach/accepting.hh"

/*!
 \file algorithm.hh
 \brief Explore algorithm
 */

namespace tchecker_ext {
  
  namespace explore_ext {
    
    /*!
     \class algorithm_t
     \brief Explore algorithm
     \tparam TS : type of transition system, should derive from tchecker::ts::ts_t
     \tparam GRAPH : type of graph, should derive from tchecker::explore::graph_t
     \tparam WAITING : type of waiting container, should derive from tchecker::utils::waiting_t
     */
    template <class TS, class GRAPH, template <class NPTR> class WAITING>
    class algorithm_t {
    public:
      
      using state_t = typename TS::state_t;
      
      using node_ptr_t = typename GRAPH::node_ptr_t;
      using edge_ptr_t = typename GRAPH::edge_ptr_t;
      
      using node_ptr_t_vec = std::vector<node_ptr_t>;
      using accepting_t = tchecker::covreach::accepting_labels_t<node_ptr_t>;
  
      using key_t = size_t;
      static inline key_t node_to_key(node_ptr_t const & node)
      {
        return tchecker::ta::details::hash_value(*node);
      }
      
      static inline
      
      /*!
       \brief Build a graph from a transition system
       \param ts : a transition system
       \param graph : a graph
       \post graph is built from a traversal of ts. For each state/transition
       of ts that is visited, a node/edge is added to graph. The order in which
       the states of ts are visited depend on the policy implemented by WAITING.
       */
      void run(TS & ts, GRAPH & graph, node_ptr_t_vec & initial_nodes_vec,
               node_ptr_t_vec & accepting_nodes_vec, accepting_t & accepting_labels)
      {

          tchecker::explore::builder_t<TS, GRAPH> builder(ts, graph);
          WAITING<node_ptr_t> waiting;

          node_ptr_t initial_node, next_node;
          edge_ptr_t edge;
          tchecker::state_status_t status;
          bool is_new_node;

          auto initial_range = ts.initial();
          for (auto it = initial_range.begin(); ! it.at_end(); ++it) {
              std::tie(initial_node, status, is_new_node) = builder.initial_node(*it);

              if (is_new_node) {
                  graph.add_node(initial_node);
                  waiting.insert(initial_node);
                  initial_nodes_vec.push_back(initial_node);
              }
          }

          while (! waiting.empty()) {
              node_ptr_t node = waiting.first();
              waiting.remove_first();
              
              //Check if node is accepting. If so store
              // Here no node coverage is currently used, so a node will continue to exist for always
              if (accepting_labels(node)){
                accepting_nodes_vec.push_back(node);
              }

              auto vedges_range = ts.outgoing_edges(*node);
              for (auto it = vedges_range.begin(); ! it.at_end(); ++it) {
                  std::tie(next_node, edge, status, is_new_node) = builder.next_node(node, *it);

                  if (is_new_node) {
                      graph.add_node(next_node);
                      waiting.insert(next_node);
                  }

                  if (status == tchecker::STATE_OK)
                      graph.add_edge(node, next_node, edge);
              }
          }
      }
    };
    
  } // end of namespace explore_ext
  
} // end of namespace tchecker_ext

#endif // TCHECKER_EXT_ALGORITHMS_EXPLORE_EXT_ALGORITHM_HH
