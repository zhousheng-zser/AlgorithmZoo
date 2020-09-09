#pragma once
#ifndef _DAG_HPP_
#define _DAG_HPP_
#include <list>
#include <queue>
#include <unordered_set>

namespace glasssix
{
	namespace excalibur
	{
		// note internal use of pointer to N (supports the Nodes being concrete objects)
		template <typename N>
		using node_set = std::unordered_set<const N*>;  ///<allows find and just one of each node
		template <typename N>
		using node_vec = std::vector<const N*>;  ///<typically used to return results

		/// visitor interface
		/**Defines the visitor class interface for the DirectedAcyclicGraph
		 */
		 /// visitor Class/Interface templated on the node N
		template <typename N>  // N is the templated node
		class visitor
		{
		public:
			visitor();  ///<Constructor
			/// Key function for visitor pattern
			virtual void visit(const N* node) = 0;
			/// returns vector of all child nodes (including the start node and all children of children)
			virtual const node_vec<N>& traverse_children(const N& startnode, int depth) = 0;
			/// returns vector of all parent nodes (including the start node and all parents of parents)
			virtual const node_vec<N>& traverse_parents(const N& startnode, int depth) = 0;
			/// returns everything linked to the start node
			virtual const node_vec<N>& traverse_undirected(const N& startnode, int depth) = 0;

		protected:
		};

		/// node class for visitor pattern templated on T the item of interest
		template <typename T>  // T is the item of interest inside the node
		class node
		{
		public:
			//typedef node<T> TNode;
			node(const T& v);  ///< Constructor
			node();            ///< Needed for putting a node inside a unordered_set
			// ideally no copying because it means nodes are no longer unique
			node<T>& operator=(node<T>&) = delete;
			node<T>& operator=(const node<T>&) = delete;
			// unordered_set requires these to be available
			node(node<T>&) {};
			node(const node<T>&) {};
			// Move is good
			node<T>& operator=(node<T>&& other) = default;
			node(node<T>&& other) {};

			void accept(visitor<node<T>>& visitor) const;  ///< Key function for visitor pattern
			void add_child(node& node);  ///< Add in a link (this will set the reverse parent link in the other node)
			const T& value() const
			{
				return m_val;
			};  ///< return the node item
			const node_set<node<T>>& children() const
			{
				return m_children;
			}
			const node_set<node<T>>& parents() const
			{
				return m_parents;
			}

		protected:
			T m_val;                                                 ///< thing that the node is encapsulating (eg identifier )
			node_set<node<T>> m_children;                               ///< direct child nodes
			node_set<node<T>> m_parents;                                ///< direct parent nodes
			void addParent(node& node)
			{
				m_parents.insert(&node);
			}  // private as only available via add_child
		};

		template <typename N>  /// Breadth First Search implementation of bfsvisitor (iterative)
		class bfsvisitor : public visitor<N>
		{ ///N is the node
		public:
			bfsvisitor();
			void visit(const N* node) override;  ///< key to visitor pattern
			const node_vec<N>& traverse_children(const N& node, int depth = -1) override;
			const node_vec<N>& traverse_parents(const N& node, int depth = -1) override;
			const node_vec<N>& traverse_undirected(const N& node, int depth = -1) override;

		protected:
			node_set<N> m_visited;    ///< which nodes have been visited (reset each time a traversal is made)
			node_vec<N> m_result;  ///< the list of nodes that are linked and that will be returned
			enum class enum_visit_type { CHILDREN, PARENTS, UNDIRECTED };  ///< internal enumeration

			/// core traversal code uses by all of the public traversals
			virtual void traverse(const node_set<N>& nodes, bfsvisitor<N>::enum_visit_type visittype, int depth);  // the iterative method
			bool already_visited(const N* node) const;
		};

		/// Breadth First Search alternative implementation using recursion
		template <typename N>
		class bfsrecurse_visitor : public bfsvisitor<N>
		{
		public:
		private:
			/// core traversal code uses by all of the public traversals
			virtual void traverse(const node_set<N>& nodes, typename bfsvisitor<N>::enum_visit_type visittype, int depth) override;
		};

		/// Constructor
		template <typename T>
		node<T>::node(const T& v) : m_val(v) {}

		/// Constructor
		template <typename T>
		node<T>::node() : m_val(T()) {}

		template <typename T>
		void node<T>::add_child(node& node)
		{
			// add_child automatically adds in the parent link - this should be a safer route and avoid missing links
			m_children.insert(&node);
			node.addParent(*this);
		}

		/**
		 accept the visitor
		 @param visitor<TNode>& visitor
		 @return void
		 */
		template <typename T>
		void node<T>::accept(visitor<node<T>>& visitor) const
		{
			visitor.visit(this);
		};

		/// Constructor
		template <typename N>
		visitor<N>::visitor() {}

		/// Constructor
		template <typename N>
		bfsvisitor<N>::bfsvisitor() : visitor<N>(), m_visited() {}

		/**
		 visit a node - add the node to the results and mark as "visited"
		 @param N* node - the node that is to be visited
		 @return void
		 */
		template <typename N>
		void bfsvisitor<N>::visit(const N* node)
		{
			m_result.push_back(node);  // add to result
			m_visited.insert(node);    // mark it as visited
		}

		template <typename N>
		bool bfsvisitor<N>::already_visited(const N* node) const
		{
			if (m_visited.find(node) == m_visited.end())
				return false;
			return true;
		}

		/**
		 traverse the nodes using Breadth First Search implemented using a Queue
		 @param node_set& nodes - the start node(s)
		 @param typename bfsvisitor<N>::enum_visit_type visittype - CHILDREN/PARENTS/UNDIRECTED
		 @param int depth - how many levels to visit (-1 = everything, 0 = start node(s), 2= start node plus 2 levels)
		 @return void
		 */
		template <typename N>
		void bfsvisitor<N>::traverse(const node_set<N>& nodes, typename bfsvisitor<N>::enum_visit_type visittype, int depth)
		{
			typedef typename bfsvisitor<N>::enum_visit_type pt;

			// Create a queue for the Breadth First Search
			std::queue<const N*> node_queue;
			std::queue<int> node_depth; //keeps track of the node depths so we can limit how deep we go if we wish

			// Mark the current node as visited and enqueue it
			for (auto const& node : nodes)
			{
				if (m_visited.find(node) == m_visited.end())
				{  // if node is not listed as already being visited
					node->accept(*this);                          // mark as visited and add to results
					node_queue.push(node);                         // put into the queue
					node_depth.push(0);
				}
			}
			while (!node_queue.empty())
			{
				// Get head node from Queue and iterate its children and/or parents
				// each of the parents and children that are not already visited
				// get put onto the end of the queue
				// One this is done the head node can be removed (popped)
				// from the queue and processing proceeds to the
				// next item in the queue
				int curdepth = node_depth.front();

				if ((depth < 0 || curdepth < depth) &&// NB depth=-1 means we are visiting everything
					((visittype == pt::CHILDREN) | (visittype == pt::UNDIRECTED)))
				{  // use the children
					for (auto node : node_queue.front()->children())
					{
						if (m_visited.find(node) == m_visited.end())
						{  // check node is not already being visited
							node->accept(*this);
							node_queue.push(node);
							node_depth.push(curdepth + 1);
						}
					}
				}
				if ((depth < 0 || curdepth < depth) && //NB depth=-1 means we are visiting everything
					((visittype == pt::PARENTS) | (visittype == pt::UNDIRECTED)))
				{
					// use the parents
					for (auto node : node_queue.front()->parents())
					{
						if (m_visited.find(node) == m_visited.end())
						{
							// check node is not already being visited
							node->accept(*this);
							node_queue.push(node);
							node_depth.push(curdepth + 1);
						}
					}
				}
				node_queue.pop();
				node_depth.pop();
			}
		}

		/**
		 traverse the nodes using Breadth First Search implemented using a recursion
		 @param node_set<N>& nodes - the start node(s)
		 @param typename bfsvisitor<N>::enum_visit_type visittype - CHILDREN/PARENTS/UNDIRECTED
		 @param int depth - how many levels to visit (-1 = everything, 0 = start node(s), 2= start node plus 2 levels)
		 @return void
		 */
		template <typename N>
		void bfsrecurse_visitor<N>::traverse(const node_set<N>& nodes, typename bfsvisitor<N>::enum_visit_type visittype, int depth)
		{
			// For a recursive  breadth first traversal we gather all nodes at the same depth
			typedef typename bfsvisitor<N>::enum_visit_type pt;
			node_set<N> visitnextnodes;  // this collects all the nodes at the next "depth"
			if (nodes.empty())
			{
				return;  // end of the recursion
			}
			for (auto node : nodes)
			{
				// Only process a node if not already visited
				if (bfsvisitor<N>::m_visited.find(node) == bfsvisitor<N>::m_visited.end())
				{
					// this will add the node to the "result" and mark the node as visited
					node->accept(*this);
					// Now add in all the children/parent/undirected links for the next depth
					// and store these into visitnextnodes
					// NB depth=-1 means we are visiting everything
					if (depth != 0 && (visittype == pt::CHILDREN | visittype == pt::UNDIRECTED))
						for (const auto child : node->children())
						{
							if (!this->already_visited(child)) visitnextnodes.insert(child);
						}
					if (depth != 0 && (visittype == pt::PARENTS | visittype == pt::UNDIRECTED))
						for (const auto parent : node->parents())
						{
							if (!this->already_visited(parent)) visitnextnodes.insert(parent);
						}
				}
			}
			depth--;
			traverse(visitnextnodes, visittype, depth);
		}

		/**
		 traverse the children using Breadth First Search
		 @param N& startnode
		 @param int depth - how many levels to visit (-1 = everything, 0 = start node(s), 2= start node plus 2 levels)
		 @return const std::vector<N*>&  results vector of Nodes
		 */
		template <typename N>
		const std::vector<const N*>& bfsvisitor<N>::traverse_children(const N& startnode, int depth)
		{
			m_result = {};                // reset the list of results:
			node_set<N> root{ &startnode };  // create an initial node_set containing the root node
			traverse(root, bfsvisitor<N>::enum_visit_type::CHILDREN, depth);
			m_visited = {};  // reset the list of visited nodes
			return m_result;
		}

		/**
		 traverse the parents using Breadth First Search
		 @param N& startnode
		 @param int depth - how many levels to visit (-1 = everything, 0 = start node(s), 2= start node plus 2 levels)
		 @return const std::vector<N*>&  results vector of Nodes
		 */
		template <typename N>
		const std::vector<const N*>& bfsvisitor<N>::traverse_parents(const N& startnode, int depth)
		{
			m_result = {};                // reset the list of results
			node_set<N> root{ &startnode };  // create an initial node_set containing the root node
			traverse(root, bfsvisitor<N>::enum_visit_type::PARENTS, depth);
			m_visited = {};  // reset the list of visited nodes
			return m_result;
		}

		/**
		 traverse all nodes linked to the start node using Breadth First Search
		 @param N& startnode
		 @param int depth - how many levels to visit (-1 = everything, 0 = start node(s), 2= start node plus 2 levels)
		 @return const std::vector<N*>&  results vector of Nodes
		 */
		template <typename N>
		const std::vector<const N*>& bfsvisitor<N>::traverse_undirected(const N& startnode, int depth)
		{
			m_result = {};                // reset the list of results
			node_set<N> root{ &startnode };  // create an initial node_set containing the root node
			traverse(root, bfsvisitor<N>::enum_visit_type::UNDIRECTED, depth);
			m_visited = {};  // reset the list of visited nodes
			return m_result;
		}
	}
}
#endif // !_DAG_HPP_
