//
// Created by philipp on 24.09.19.
//

#ifndef TCHECKER_EXT_GRAPH_HH
#define TCHECKER_EXT_GRAPH_HH

#include "tchecker/algorithms/covreach/graph.hh"

#include "tchecker_ext/algorithms/covreach_ext/waiting.hh"
#include "tchecker_ext/utils/spinlock.hh"

namespace tchecker_ext{
  namespace covreach_ext{
    
    /*!
     \brief A partially thread safe version of tchecker::covreach::graph_t
     \note Attention this class is not inherently thread-safe; Only function where it is explicetly noted are thread-safe
     \tparam KEY
     \tparam TS
     \tparam TS_ALLOCATOR
     */
    template <class KEY, class TS, class TS_ALLOCATOR>
    class graph_t: public tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>{
    public:
  
      /*!
       \brief Type of pointers to node
       */
      using node_ptr_t = typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::node_ptr_t;
  
      template <class ... ARGS>
      graph_t(tchecker::gc_t & gc,
              std::tuple<ARGS...> && ts_alloc_args,
              std::size_t block_size,
              std::size_t table_size,
              typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::node_to_key_t node_to_key,
              typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::node_binary_predicate_t le_node)
              : tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>(gc, std::forward<std::tuple<ARGS...>>(ts_alloc_args), block_size, table_size, node_to_key, le_node)
          {
            _container_locks = std::vector<tchecker_ext::spinlock_t>(table_size);
          }
      
      /*!
       \brief Function that will decide whether a node is covered by the graph or is covering some in the graph
       \note The covered / covering nodes are all in the same container -> We can modify the edges of existing nodes
             The edges can be modified as they are newly created
       @tparam STATS
       @param next_nodes_vec
       @param stats
       */
      // TODO check if it wouldn't be more efficient to only keep incoming edges correct and add the outgoing edges afterwards
      template <class STATS>
      void check_and_insert(node_ptr_t  parent_node, std::vector<node_ptr_t> &next_nodes_vec, STATS stats){
        
        // The idea is to loop over the different containers and take one that is currently free
        size_t num_to_treat = 0;
        node_ptr_t covering_node, next_node;
        std::vector<node_ptr_t> covered_nodes_vec;
        auto covered_nodes_vec_inserter = std::back_inserter(covered_nodes_vec);
        std::vector<tchecker::graph::cover::node_position_t> associated_container_num(next_nodes_vec.size());
        std::vector<bool> is_treated(next_nodes_vec.size(), true);
        
        // Loop once to get all container id's and count how many are active
        for (size_t i=0; i < next_nodes_vec.size(); ++i){
          if (next_nodes_vec[i]->is_active()){
            num_to_treat++;
            is_treated[i] = false; // This node has to be handled
            associated_container_num[i] = tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(next_nodes_vec[i]); // This corresponds to the container id
          }
        }
        
        while (num_to_treat>0){ // Until all next_nodes_vec are treated
          for (size_t i=0; i < next_nodes_vec.size(); ++i) { // Loop over next_nodes_vec to check which one we can treat next
            if (is_treated[i]){
              continue; //Already done
            }
            // Check if the associated container is free AND acquire it
            if (_container_locks[i].lock_once()){
              next_node = next_nodes_vec[i];
              // Now we can treat the node as all next_nodes_vec that we have to compare it to are stored in this container
              if (tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::is_covered_external(next_node, covering_node)){
                next_node->make_inactive();
                tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_edge(parent_node, covering_node, tchecker::covreach::ABSTRACT_EDGE);
                stats.increment_covered_leaf_nodes();
              }
  
              // Now we are sure that the node is not included in some other node
              // and we will add it to the graph along with the edge
              tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_node(next_node);
              tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_edge(parent_node, next_node, tchecker::covreach::ACTUAL_EDGE);
              
              // Check if this new nodes covers others
              covered_nodes_vec.clear();
              tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::covered_nodes(next_node, covered_nodes_vec_inserter);
              for (node_ptr_t & covered_node : covered_nodes_vec) {
                covered_node->make_inactive(); // This ensure that it is not enlisted in waiting afterwards
                cover_node(covered_node, next_node);
                stats.increment_covered_nonleaf_nodes();
              }
              
              // Finished treating this node
              _container_locks[i].unlock(); // Release container
              --num_to_treat;
              is_treated[i] = true;
            } // if locked
          } // for next_node : next_nodes_vec
        } // while untreated
      
      }//check_and_insert
      /*!
       \brief Cover a node
       \param covered_node : covered node
       \param covering_node : covering node
       \post graph has been updated to let covering_node replace covered_node
       */
      void cover_node(node_ptr_t & covered_node, node_ptr_t & covering_node)
      {
        tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::move_incoming_edges(covered_node, covering_node, tchecker::covreach::ABSTRACT_EDGE);
        tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::remove_edges(covered_node);
        tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::remove_node(covered_node);
      }

    private:
      // TODO the locks should probably go to cover/graph for more coherence
      std::vector<tchecker_ext::spinlock_t> _container_locks; /*! One lock for each node_ptr_t container */
      
    };
    
  }//covreach_ext
}//tchecker_ext

#endif //TCHECKER_EXT_GRAPH_HH