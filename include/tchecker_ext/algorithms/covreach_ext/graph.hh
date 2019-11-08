//
// Created by philipp on 24.09.19.
//

#ifndef TCHECKER_EXT_GRAPH_HH
#define TCHECKER_EXT_GRAPH_HH

#include <chrono>
#include <thread>

#include "tchecker/algorithms/covreach/graph.hh"

#include "tchecker_ext/algorithms/covreach_ext/waiting.hh"
#include "tchecker_ext/utils/spinlock.hh"

#include <tchecker_ext/config.hh>



namespace tchecker_ext{
  namespace covreach_ext{
    
    using edge_type_t = tchecker::covreach::edge_type_t;
    
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
      using edge_ptr_t = typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::edge_ptr_t;
      
      using dir_graph_t = typename tchecker::graph::directed::graph_t<node_ptr_t, edge_ptr_t>;
      using cov_graph_t = typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>;
  
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
        \brief Add edge with minimal reference counter interference
        \param src : source node counter will be modified
        \param tgt : target node counter will be modified
        \param edge_type : type of edge
        \param check_existence: whether to always insert or not
        \post an edge src -> tgt with type edge_type has been allocated and added to the graph
        \note reference counter of newly created edge will be changed
        \note if check_existence is true, an edge will only be created of no other edge already exists
        \note if an abstract edge exists between src and target it will be "promoted"
        */
      void add_edge_swap(node_ptr_t const & src, node_ptr_t const & tgt,
                         enum tchecker::covreach::edge_type_t edge_type, bool check_existence=true)
      {
        // TODO make this more beautiful
        // The problem is to get the edge without taking/releasing references
        // and allow to change the edge type
        if (check_existence){
          // Search if any edges exist from src to tgt
          edge_ptr_t * current_edge_ptr = &dir_graph_t::get_outgoing_head(src);//src->template head<struct tchecker::graph::directed::details::outgoing>();
          const edge_ptr_t end_ptr = edge_ptr_t{nullptr};
          while (*current_edge_ptr != end_ptr){
//            if (dir_graph_t::get_incoming_node(*current_edge_ptr) == tgt){
            if (dir_graph_t::edge_tgt(*current_edge_ptr) == tgt){
                // Found an existing edge
                // Keep the "stronger" type. Therefore the edge will only be abstract if the new type and the current type are abstract
                if (!((edge_type == edge_type_t::ABSTRACT_EDGE) && ((*current_edge_ptr)->edge_type() == edge_type_t::ABSTRACT_EDGE))){
                  (*current_edge_ptr)->set_type(edge_type_t::ACTUAL_EDGE);
                }
                // Done
                return;
            }
          //current_edge = current_edge->template next<struct tchecker::graph::directed::details::outgoing>();
          current_edge_ptr = &dir_graph_t::get_next_outgoing_edge(*current_edge_ptr);
          }
        }
        // No such edge could be found
        // -> Allocate and place
        edge_ptr_t edge = cov_graph_t::_edge_allocator.construct(edge_type);
        dir_graph_t::add_edge_swap(src, tgt, edge);
      }
      
      /*!
       \brief Alternative implementation of move_incoming edges:
       Only modifies the reference counter of n1 and n2. It is thread safe if the containers of
       n1 and n2 are locked because then no other thread can modify the incoming edges list
       * @param n1
       * @param n2
       * @param edge_type
       */
      void move_incoming_edges(node_ptr_t const & n1,
                               node_ptr_t const & n2,
                               bool do_change_type=false,
                               enum tchecker::covreach::edge_type_t edge_type=edge_type_t::ACTUAL_EDGE) {
        // Base version of move incoming edges removed then added the edges
        // To avoid this, the edges here will be appended to the existing list of edges
        edge_ptr_t *current_edge_ptr = nullptr;
        const edge_ptr_t end_ptr = edge_ptr_t{nullptr};
        
        if ( (n1==n2) || (dir_graph_t::get_incoming_head(n1) == end_ptr)){
          // If the nodes are the same
          // Or if there are no edges to transfer
          // We are already done
          return;
        }
        
        // 1) Set the corresponding node to n2 and change the type
        current_edge_ptr = &dir_graph_t::get_incoming_head(n1);
        while (*current_edge_ptr != end_ptr) {
          if (do_change_type) {
            (*current_edge_ptr)->set_type(edge_type);
          }
          dir_graph_t::get_incoming_node(*current_edge_ptr) = n2; //Decreases n1 counter increases n2 counter
          current_edge_ptr = &dir_graph_t::get_next_incoming_edge(*current_edge_ptr);
        }
        
        // 2) Append it to the incoming of n2
        // 2.1) Search end of incoming, n2, infact search the "next" of the last element
        current_edge_ptr = &dir_graph_t::get_incoming_head(n2);
        while ((*current_edge_ptr) != end_ptr){
          current_edge_ptr = &dir_graph_t::get_next_incoming_edge(*current_edge_ptr);
        }
        // 2.2) Swap current (which is now next of last element) with head of incoming n1
        (*current_edge_ptr).swap( dir_graph_t::get_incoming_head(n1) );
        // Done
      }
  
      /*!
       \brief Move outgoing edges instead of removing them to avoid "dangling" node
       Only modifies the reference counter of n1 and n2. It is thread safe if the containers of
       n1 and n2 are locked because then no other thread can modify the incoming edges list
       * @param n1
       * @param n2
       * @param edge_type
       */
      void move_outgoing_edges(node_ptr_t const & n1,
                               node_ptr_t const & n2,
                               bool do_change_type=false,
                               enum tchecker::covreach::edge_type_t edge_type=edge_type_t::ACTUAL_EDGE) {
        edge_ptr_t *current_edge_ptr = nullptr;
        const edge_ptr_t end_ptr = edge_ptr_t{nullptr};
  
        if ( (n1==n2) || dir_graph_t::get_outgoing_head(n1) == end_ptr){
          // If the nodes are the same
          // Or if there are no edges to transfer
          // We are already done
          return;
        }
    
        // 1) Set the corresponding node to n2 and change the type
        current_edge_ptr = &dir_graph_t::get_outgoing_head(n1);
        while (*current_edge_ptr != end_ptr) {
          if (do_change_type) {
            (*current_edge_ptr)->set_type(edge_type);
          }
          dir_graph_t::get_outgoing_node(*current_edge_ptr) = n2; //Decreases n1 counter increases n2 counter
          current_edge_ptr = &dir_graph_t::get_next_outgoing_edge(*current_edge_ptr);
        }
    
        // 2) Append it to the incoming of n2
        // 2.1) Search end of incoming, n2, infact search the "next" of the last element
        current_edge_ptr = &dir_graph_t::get_outgoing_head(n2);
        while ((*current_edge_ptr) != end_ptr){
          current_edge_ptr = &dir_graph_t::get_next_outgoing_edge(*current_edge_ptr);
        }
        // 2.2) Swap current (which is now next of last element) with head of incoming n1
        (*current_edge_ptr).swap( dir_graph_t::get_outgoing_head(n1) );
        // Done
      }
      
      //TODO unify the two "moves"
      
      
      /*!
       \brief Function that will decide whether a node is covered by the graph or is covering some in the graph
       \note The covered / covering nodes are all in the same container -> We can modify the edges of existing nodes
             The edges can be modified as they are newly created
       @tparam STATS
       @param next_nodes_vec
       @param stats
       */
      template <class STATS>
      void check_and_insert(node_ptr_t &parent_node, std::vector<node_ptr_t> &next_nodes_vec, STATS &stats){
        // The idea is to lock the parent container and
        // then loop over the different containers and take one that is currently free
        // Attention outer loop is necessary to ensure liveness
        size_t no_access_counter=0;
        size_t no_access=0;
        size_t num_to_treat = 0;
        node_ptr_t covering_node{nullptr};
        node_ptr_t next_node{nullptr};
        std::vector<node_ptr_t> covered_nodes_vec;
        auto covered_nodes_vec_inserter = std::back_inserter(covered_nodes_vec);
        std::vector<tchecker::graph::cover::node_position_t> associated_container_num(next_nodes_vec.size());
        std::vector<bool> is_treated(next_nodes_vec.size(), true);
  
        tchecker::graph::cover::node_position_t parent_container_num =
            tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(parent_node);
        
        // Before starting check if the parent is still active
        // This is necessary as we can no longer use waiting_ok
        _container_locks[parent_container_num].lock();
        if (!parent_node->is_active()){
          // Till here the next-nodes are only known to this thread
          // We can just delete the whole vector
          next_nodes_vec.clear();
          // Delete the reference to the parent
          // This is important as the reference counter of a node can only be safely changed
          // if the corresponding container is locked
          parent_node = node_ptr_t{nullptr};
          // Unlock and return
          _container_locks[parent_container_num].unlock();
          return;
        }
        //Unlock if parent still active
        _container_locks[parent_container_num].unlock();
        
        // next_nodes are still thread local
        // Loop once to get all container id's and count how many are active
        for (size_t i=0; i < next_nodes_vec.size(); ++i){
          assert(next_nodes_vec[i]->refcount()==1);
          if (next_nodes_vec[i]->is_active()){
            num_to_treat++;
            is_treated[i] = false; // This node has to be handled
            associated_container_num[i] = tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(next_nodes_vec[i]); // This corresponds to the container id
          }else{
            // If they are inactive delete the reference
            next_nodes_vec[i] = node_ptr_t{nullptr};
          }
        }
  
        while (num_to_treat>0){ // Until all next_nodes_vec are treated; Other loop to avoid dead-locks
          // Lock the parent; This can produce a deadlock
          // if all children and parents have the same states across all threads
          _container_locks[parent_container_num].lock();
          
          assert(covered_nodes_vec.empty());
          no_access_counter = 0;
          while (num_to_treat>0){ // Until all next_nodes_vec are treated
            no_access = 1; // No child container has been accessed
            for (size_t i=0; i < next_nodes_vec.size(); ++i) { // Loop over next_nodes_vec to check which one we can treat next
              if (is_treated[i]){
                continue; // Already done
              }
              assert(next_nodes_vec[i].ptr()!=nullptr);
              // Check if the associated container is free AND acquire it
              // Except if it is the same container as parent, then ok...
              if (_container_locks[associated_container_num[i]].lock_once()
                  || (associated_container_num[i]==parent_container_num) ){
                
                no_access = 0; // Got access to at least one
                
                //Swap them so that next_nodes_vec[i] becomes nullptr
                next_node.swap(next_nodes_vec[i]);

                // Mark as treated
                --num_to_treat;
                is_treated[i] = true;
                
                // Now we can treat the next_node as all the nodes that we have to compare it to are stored in this (now locked) container
                if (tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::is_covered_external(next_node, covering_node)){ //covered?
                  assert( (tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(next_node) ==
                              tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(covering_node)) );
                  // This is ok as parent and covering are locked
                  // Here one can or cannot search for existing edges
                  add_edge_swap(parent_node, covering_node, tchecker::covreach::ABSTRACT_EDGE, true);
                  // Safely delete the next_node/covering_node reference
                  next_node = node_ptr_t{nullptr};
                  covering_node = node_ptr_t{nullptr};
                  stats.increment_covered_leaf_nodes();
                }else{ // covering?
                  // Now we are sure that the node is not included in some other node
                  // and we will add it to the graph along with the edge
                  assert(next_node->is_active()); //Todo
                  //From now on others threads can possible see it if the corresponding container is unlocked
                  tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_node(next_node);
                  // ok parent and next_node is locked
                  // Here it is sure that no other edge exists -> do not check
                  add_edge_swap(parent_node, next_node, tchecker::covreach::ACTUAL_EDGE, false);
                  
                  // Check if this new node covers others
                  assert(covered_nodes_vec.empty());
                  cov_graph_t::covered_nodes(next_node, covered_nodes_vec_inserter); //next_node and covered nodes are in the same container so we can change the reference counter

                  for (size_t j=0; j<covered_nodes_vec.size(); ++j ){
                    covered_nodes_vec[j]->make_inactive();
                    cover_node(covered_nodes_vec[j], next_node);
                    stats.increment_covered_nonleaf_nodes();
                  }// covered
                  covered_nodes_vec.clear(); //Clear before releasing the container
                  // Swap it back into the vector as this node remains active
                  next_node.swap(next_nodes_vec[i]);
                }//covering
                
                //Make sure all ptr as reset before releasing the container
                assert(covered_nodes_vec.empty());
                assert(next_node.ptr() == nullptr);
                assert(covering_node.ptr() == nullptr);
                
                // Finished treating this node
                // Unlock, except if parent
                if(associated_container_num[i]!=parent_container_num){
                  _container_locks[associated_container_num[i]].unlock(); // Release container
                }
              } // if locked
            } // for next_node : next_nodes_vec
            no_access_counter += no_access;
            
            if (no_access_counter>100){//todo parametrize or find the right time/nbr of cycles
              _container_locks[parent_container_num].unlock();//Release container
              // Give other threads a chance
              std::this_thread::sleep_for(std::chrono::microseconds(5));
              break;
            }
          } // while untreated
        } // while untreated deadlocks
  
        // Also safely release the reference to the parent
        parent_node = node_ptr_t{nullptr};
        //Release parent container
        _container_locks[parent_container_num].unlock();

        return;
      }//check_and_insert
      
      /*!
       \brief Cover a node
       \param covered_node : covered node
       \param covering_node : covering node
       \post graph has been updated to let covering_node replace covered_node
       */
      void cover_node(node_ptr_t & covered_node, node_ptr_t & covering_node)
      {
        // This is ok, as covered_node and covering_node are locked
        move_incoming_edges(covered_node, covering_node, true, tchecker::covreach::ABSTRACT_EDGE);
        // The successors of covering_node can cover any successor of covered_node
        // currently, if the successor of covering node is exactly as large as some child of covered_node,
        // there will only be an abstract_edge between them though it should be actual
        move_outgoing_edges(covered_node, covering_node);
        tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::remove_node(covered_node);
        return;
      }

    protected:
      // TODO the locks should probably go to cover/graph for more coherence
      std::vector<tchecker_ext::spinlock_t> _container_locks; /*! One lock for each node_ptr_t container */
    };
    
  }//covreach_ext
}//tchecker_ext

#endif //TCHECKER_EXT_GRAPH_HH














//
//if (tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::is_le(covering_node, next_node)){
//// The two nodes are actually equal (is_covered ensure next_node <= covering_node)
//// In this cas we want to move all incoming transitions to the new node
//// as the parent_node of next_node might be a cover of the original parent_node of the
//// covering node
//tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_node(next_node); //From now on others threads can possible see it if the corresponding container is unlocked
//tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_edge(parent_node, next_node, tchecker::covreach::ACTUAL_EDGE);
//// This will move all incoming edges from covering_node to next_node
//// without changing the mode (actual/abstract)
//cover_node(covering_node, next_node, true);
//covering_node->make_inactive();
//}else{
//// The next_node is actually covered by the covering_node, therefore the parent_node
//// of next_node cannot be a cover of the parent_node of covering node
//// Treat regularly
//// Add an abstract edges from the parent to the covering_node
//tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_edge(parent_node, covering_node, tchecker::covreach::ABSTRACT_EDGE);
//}
//// Safely delete the next_node/covering_node reference