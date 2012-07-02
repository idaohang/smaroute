/*
 * patRouter.cc
 *
 *  Created on: Apr 20, 2012
 *      Author: jchen
 */

#include "patRouter.h"
#include "patException.h"
#include "patLinkCostExcludingNodes.h"
#include "patTimeFunctions.h"
patRouter::patRouter(const patNetworkBase* network,
		const patLinkAndPathCost* link_cost) :
		m_network(network), m_link_cost(link_cost) {

}

patRouter::~patRouter() {
}

// -------------------- INTERNALS --------------------

//TODO
//FIXE treCost algorithm
//CHECK shortestpathtree for backward
//double sp_time = 0.0;
//int cal_count = 0;
void patRouter::treeCost(patShortestPathTreeGeneral& shortest_path_tree,
		const patNode* root, set<const patNode*> targets,
		const Direction direction) const {
//	double time_start = getMillisecond();
	/*
	 * (1) define set of feasible targets
	 */
	set<const patNode*> targets_left;
	if (targets.empty()) {
		targets_left.insert(NULL);
	} else {
//        DEBUG_MESSAGE("WITH TARGETS");
		targets_left.insert(targets.begin(), targets.end());
	}
	/*
	 * (2) initialize data structures and search loop
	 */
	patNode* current_node;
	double minimum_label = m_network->getMinimumLabel();
	shortest_path_tree.setLabel(root, 0.0);
	shortest_path_tree.insertRoot(root);

	deque<const patNode*> list_of_nodes;
	list_of_nodes.push_back(root);

	const unordered_map<const patNode*, set<const patRoadBase*> >* incidents;

	if (direction == FWD) {
//		DEBUG_MESSAGE("FORWARD");
		incidents = m_network->getOutgoingIncidents();
	} else if (direction == BWD) {
		//DEBUG_MESSAGE("BACKWARD");
		incidents = m_network->getIncomingIncidents();
//		DEBUG_MESSAGE(incidents->size());
	}

	else {
		throw RuntimeException("wrong directin parameter");
	}
//	DEBUG_MESSAGE(incidents->size());
	/*
	 * (3) search until all reachable targets are found
	 */
//    DEBUG_MESSAGE(targets.size()<<","<<targets_left.size());
//    if(!targets.empty()){
//    DEBUG_MESSAGE(root->getUserId()<<","<<(*(targets.begin()))->getUserId());
//	}
    while (!targets_left.empty() && !list_of_nodes.empty()) {
		const patNode* node_to_process = *list_of_nodes.begin();
		list_of_nodes.pop_front();
		targets_left.erase(node_to_process);

		/*
		 * (3-A) expand forwards
		 */
//		DEBUG_MESSAGE(node_to_process->getUserId());
		unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator find_out_going =
				incidents->find(node_to_process);
		if (find_out_going == incidents->end()) {

			continue;
		}
		for (set<const patRoadBase*>::const_iterator outgoing_road_iter =
				find_out_going->second.begin();
				outgoing_road_iter != find_out_going->second.end();
				++outgoing_road_iter) {
			const patNode* down_node;

			if (direction == FWD) {
				down_node = (*outgoing_road_iter)->getDownNode();
			} else if (direction == BWD) {
				down_node = (*outgoing_road_iter)->getUpNode();
			}

			const double road_cost = (m_link_cost->getCost(*outgoing_road_iter));
			double down_node_label = shortest_path_tree.getLabel(down_node);
//			DEBUG_MESSAGE(down_node->getUserId()<<","<<road_cost);
			if (down_node_label
					> shortest_path_tree.getLabel(node_to_process) + road_cost) {
				shortest_path_tree.setLabel(
						down_node,
						shortest_path_tree.getLabel(node_to_process)
								+ road_cost);
				if (shortest_path_tree.getLabel(down_node) < 0.0000001) {
					WARNING(
							"NEGATIVE CYCLE DETECTED"<<shortest_path_tree.getLabel(down_node)<<","<<road_cost);
					throw RuntimeException("NEGATIVE CYCLE DETECTED");
				}

				shortest_path_tree.setPredecessor(down_node,
						*outgoing_road_iter);


// Add the node following Bertsekas (1993)
				if (list_of_nodes.empty()) {
					list_of_nodes.push_back(down_node);
				} else {
					double top_label = shortest_path_tree.getLabel(
							list_of_nodes.front());
					if (down_node_label <= top_label) {
						list_of_nodes.push_front(down_node);
					} else {
						list_of_nodes.push_back(down_node);
					}
				}
			}
		}
//        DEBUG_MESSAGE(list_of_nodes.size());
	}
//	double time_end = getMillisecond();
//	sp_time += ((double) time_end - time_start);
//	if (cal_count % 500 == 0) {
//		DEBUG_MESSAGE(cal_count << "," << sp_time);
//	}
//	++cal_count;

}

// -------------------- ROUTING IMPLEMENTATIONS --------------------
void patRouter::fwdCost(patShortestPathTreeGeneral& tree, const patNode* origin,
		set<const patNode*> destinations) const {
	return treeCost(tree, origin, destinations, FWD);
}
void patRouter::bwdCost(patShortestPathTreeGeneral& tree,
		set<const patNode*> origins, const patNode* destination) const {
//	DEBUG_MESSAGE(*destination);
	return treeCost(tree, destination, origins, BWD);
}

void patRouter::fwdCost(patShortestPathTreeGeneral& tree, const patNode* origin,
		const patNode* destination) const {
	set<const patNode*> destinations;
	//destinations.insert(destination);
	return fwdCost(tree, origin, destinations);
}

void patRouter::fwdCost(patShortestPathTreeGeneral& tree,
		const patNode* origin) const {
	return fwdCost(tree, origin, m_network->getNodes());
}
void patRouter::bwdCost(patShortestPathTreeGeneral& tree,
		const patNode* destination) const {
	return bwdCost(tree, m_network->getNodes(), destination);
}

void patRouter::bwdCost(patShortestPathTreeGeneral& tree, const patNode* origin,
		const patNode* destination) const {
	set<const patNode*> origins;
	origins.insert(origin);
	//DEBUG_MESSAGE(*destination);
	return bwdCost(tree, origins, destination);
}

unordered_map<const patNode*, patMultiModalPath> patRouter::bestRoutes(
		set<const patNode*> origins, const patNode* destination,
		const patShortestPathTreeGeneral* bwdCostTree) const {
	unordered_map<const patNode*, patMultiModalPath> result;
	for (set<const patNode*>::const_iterator origin_iter = origins.begin();
			origin_iter != origins.end(); ++origin_iter) {
        list<const patRoadBase*> list_of_roads;
        bwdCostTree->getShortestPathTo(list_of_roads,*origin_iter);
		patMultiModalPath new_path(list_of_roads);
		result[*origin_iter] = new_path;

	}
	return result;
}

unordered_map<const patNode*, patMultiModalPath> patRouter::bestRoutes(
		const patNode* origin, set<const patNode*> destinations,
		const patShortestPathTreeGeneral* fwdCostTree) const {
	unordered_map<const patNode*, patMultiModalPath> result;
	for (set<const patNode*>::const_iterator dest_iter = destinations.begin();
			dest_iter != destinations.end(); ++dest_iter) {
        
        list<const patRoadBase*> list_of_roads;

        fwdCostTree->getShortestPathTo(list_of_roads,*dest_iter);
		patMultiModalPath new_path(list_of_roads);
		result[*dest_iter] = new_path;

	}
	return result;
}

patMultiModalPath patRouter::bestRouteBwd(const patNode*origin,
		const patNode* destination,
		const patShortestPathTreeGeneral* bwdCost) const {
	set<const patNode*> origins;
	origins.insert(origin);
	return bestRoutes(origins, destination, bwdCost)[origin];
}

patMultiModalPath patRouter::bestRouteFwd(const patNode* origin,
		const patNode* destination,
		const patShortestPathTreeGeneral* fwdCost) const {
	set<const patNode*> destinations;
	destinations.insert(destination);
	return bestRoutes(origin, destinations, fwdCost)[destination];
}

unordered_map<const patNode*, patMultiModalPath> patRouter::bestRoutes(
		const patNode* origin, set<const patNode*> destinations) const {
	patShortestPathTreeGeneral fwd_cost(FWD);
	fwdCost(fwd_cost, origin, destinations);
	return bestRoutes(origin, destinations, &fwd_cost);
}

unordered_map<const patNode*, patMultiModalPath> patRouter::bestRoutes(
		set<const patNode*> origins, const patNode* destination) const {
	patShortestPathTreeGeneral bwd_cost(BWD);
	bwdCost(bwd_cost, origins, destination);
	return bestRoutes(origins, destination, &bwd_cost);
}

patMultiModalPath patRouter::bestRoute(const patNode* origin,
		const patNode* destination) const {
	set<const patNode*> destinations;
	destinations.insert(destination);
	return bestRoutes(origin, destinations)[destination];
}

// public double bestRouteCost(final Node origin, final Node destination) {
//
//
// }

// -------------------- SUPPLEMENTARY IMPLEMENTATIONS --------------------

// -------------------- TODO FROM UpdateRouter --------------------

void patRouter::costWithoutExcludedNodes(
                                         patShortestPathTreeGeneral& tree,
                                         const patNode* root,
                                         const patNode* destination,
                                         const set<const patNode*>& excludedNodes,
                                         const patShortestPathTreeGeneral* treeCost,
                                         Direction direction) const {
	return costWithoutExcludedNodes2(tree, root, destination,excludedNodes, treeCost,
			direction);
}

void patRouter::costWithoutExcludedNodes2(patShortestPathTreeGeneral& tree,
		const patNode* root ,const patNode* destination,const set<const patNode*>& excludedNodes,
		const patShortestPathTreeGeneral* treeCost, Direction direction) const {
	/*
	 * (1) simple case: the root node is excluded
	 */

	if (excludedNodes.find(root) != excludedNodes.end()) {
		//DEBUG_MESSAGE("root is excluded");
		throw RuntimeException("trying to compute SP cost tree with forbidden root");
	}
	if (excludedNodes.find(destination) != excludedNodes.end()) {
		//DEBUG_MESSAGE("root is excluded");
		throw RuntimeException("trying to compute SP cost tree with forbidden target");
	}
	/*
	 * (2) compute completely new tree cost
	 */
	patLinkCostExcludingNodes myLinkCost(m_link_cost, excludedNodes);
	//DEBUG_MESSAGE(excludedNodes.size());
	patRouter myRouter(m_network, &myLinkCost);
	set<const patNode*> targets;
	targets.insert(destination);
//    DEBUG_MESSAGE(destination->getUserId());
	myRouter.treeCost(tree, root, targets, direction);
	//DEBUG_MESSAGE("OK");
	/*
	 * (3) simply return the result
	 */
}

void patRouter::fwdCostWithoutExcludedNodes(patShortestPathTreeGeneral& tree,
		const patNode* origin,const patNode* destination,
		set<const patNode*> excludedNodes,
		const patShortestPathTreeGeneral* fwdCost) const {
	return costWithoutExcludedNodes(tree, origin,destination, excludedNodes,
			fwdCost, FWD);
}

void patRouter::bwdCostWithoutExcludedNodes(patShortestPathTreeGeneral& tree,
		const patNode* destination,const patNode* origin, 
		set<const patNode*> excludedNodes,
		const patShortestPathTreeGeneral* bwdCost) const {
	return costWithoutExcludedNodes(tree, destination, origin, excludedNodes,
			bwdCost, BWD);
}

const patNetworkBase* patRouter::getNetwork() const {
	return m_network;
}