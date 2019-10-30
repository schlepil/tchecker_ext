//
// Created by philipp on 16/09/2019.
//

#ifndef TCHECKER_POST_PROC_EXPLORE_HH
#define TCHECKER_POST_PROC_EXPLORE_HH

#include <vector>

#include "tchecker/graph/directed_graph.hh"
#include "tchecker/graph/find_graph.hh"
#include "tchecker/algorithms/covreach/accepting.hh"

#include "tchecker/ta/details/state.hh"

namespace tchecker_ext{

    namespace post_proc {
  
      enum node_types_t {
        REGULAR = 0,
        INIT,
        ACCEPTING,
      };
  
      /*!
       \brief Directed graph representing all reachable states from the accepted states
       */
  
      //template<class NODE, class EDGE, class NODE_PTR, class EDGE_PTR>
//      class solution_graph_t
//          : public tchecker::graph::directed::graph_t<NODE_PTR, EDGE_PTR> {
      template<class ALLOCATOR>
      class solution_graph_t:public ALLOCATOR{
      
      public:
        using node_t = typename ALLOCATOR::node_t;
//        using edge_t = EDGE;
        using node_ptr_t = typename ALLOCATOR::node_ptr_t;
//        using edge_ptr_t = EDGE_PTR;
    
        template <class ... A_ARGS>
        solution_graph_t(size_t table_size, std::tuple<A_ARGS...> && a_args):
            ALLOCATOR(std::forward<std::tuple<A_ARGS...>>(a_args)),
            nodes_map_(table_size){
          nodes_init_.clear();
          nodes_accepting_.clear();
        }
    
        /*!
         * \brief Adds a node that does not yet exist in the graph
         * @param node_ptr
         * @return true if success false otherwise
         */
        bool add_node(node_ptr_t const &node_ptr, node_types_t type) {
          
          if (type == REGULAR){
            assert(false); //Regular nodes are not to be inserted by hand
            return false;
          }
  
          //Construct a copy
          node_ptr_t copied_node_ptr = ALLOCATOR::allocate_from_node(node_ptr, std::make_tuple(0));
  
          size_t table_pos = get_position_table(copied_node_ptr);
          nodes_map_[table_pos].push_back(copied_node_ptr);
          
          if (type == INIT)
            nodes_init_.push_back(copied_node_ptr);
          else if (type == ACCEPTING)
            nodes_accepting_.push_back(copied_node_ptr);
          else
            throw std::runtime_error("wrong enum");
          return true;
        }
        
        inline size_t get_position_table(const node_ptr_t & node_ptr) const {
          return get_position_table(*node_ptr);
        }
        inline size_t get_position_table(const node_t & node)const{
          return tchecker::ta::details::hash_value(node) % nodes_map_.size();
        }
        
        /*!
         \brief Delete all nodes and edges; clear the allocator
         */
         void free_all(){
           nodes_map_.clear();
           nodes_init_.clear();
           nodes_accepting_.clear();
           ALLOCATOR::free_all();
           return;
         }

      protected:
        std::vector<std::vector<node_ptr_t>> nodes_map_;
        std::vector<node_ptr_t> nodes_init_;
        std::vector<node_ptr_t> nodes_accepting_;
      };
  
  
//      template<class ZONE_GRAPH, class SOL_GRAPH, class ACCEPT>
//      void construct_solution_graph()
//
//      template <class NODE_PTR, class EDGE_PTR>
//      class partial_solution_graph : public partial_solution_graph<NODE_PTR, EDGE_PTR{
//
//
//      private:
//          const std::vector<tchecker::clock_id_t> _idx_clk;
//          const std::vector<tchecker::loc_id_t> _idx_loc;
//          const std::vector<tchecker::intvar_id_t> _idx_intvar;
//      };
  
    }
}


#endif //TCHECKER_POST_PROC_EXPLORE_HH
