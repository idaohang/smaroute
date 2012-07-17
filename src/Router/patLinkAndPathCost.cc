/*
 * patPathCost.cc
 *
 *  Created on: Jun 1, 2012
 *      Author: jchen
 */

#include "patLinkAndPathCost.h"
#include "patRoadBase.h"
#include "patMultiModalPath.h"
#include "patException.h"

patLinkAndPathCost::~patLinkAndPathCost() {
	if (m_ps_computer != NULL) {
		delete m_ps_computer;
		m_ps_computer = NULL;
	}

}
patLinkAndPathCost::patLinkAndPathCost(double link_scale, double length_coef,
		double ps_coef, double sb_coef) :
		m_ps_computer(NULL) {
	if (length_coef != 0.0) {
		m_link_coefficients[ENUM_LENGTH] = length_coef;
	}
	if (sb_coef != 0.0) {
		m_link_coefficients[ENUM_SPEED_BUMP] = sb_coef;
	}
	for (map<ARC_ATTRIBUTES_TYPES, double>::iterator a_iter =
			m_link_coefficients.begin(); a_iter != m_link_coefficients.end();
			++a_iter) {
		if (a_iter->second > 0.0) {
			cout << "param: " << a_iter->first << "," << a_iter->second << endl;
		}
	}
	m_link_cost_scale = link_scale;
	m_pathsize_coefficient = ps_coef;

//	DEBUG_MESSAGE(
//			"length coef: "<<m_link_coefficients[ENUM_LENGTH]<< ", sb coef: "<<m_link_coefficients[ENUM_SPEED_BUMP]<<", link cost scale: "<<m_link_cost_scale<<", path size coef"<<m_pathsize_coefficient);

}

patLinkAndPathCost* patLinkAndPathCost::clone() const {
	return new patLinkAndPathCost(*this);
}
patLinkAndPathCost::patLinkAndPathCost(
		map<ARC_ATTRIBUTES_TYPES, double>& link_coef, double &link_scale,
		double &ps_scale) :
		m_ps_computer(NULL) {
	m_link_coefficients.clear();
	for (map<ARC_ATTRIBUTES_TYPES, double>::iterator a_iter = link_coef.begin();
			a_iter != link_coef.end(); ++a_iter) {
		if (a_iter->second != 0.0) {
			cout << "param: " << a_iter->first << "," << a_iter->second << endl;
			m_link_coefficients[a_iter->first] = a_iter->second;
		}
	}
//	m_link_coefficients = link_coef;
	m_link_cost_scale = link_scale;
	m_pathsize_coefficient = ps_scale;
}
double patLinkAndPathCost::getCost(const patRoadBase* road) const {
	double cost = 0.0;
	for (map<ARC_ATTRIBUTES_TYPES, double>::const_iterator a_iter =
			m_link_coefficients.begin(); a_iter != m_link_coefficients.end();
			++a_iter) {
//		DEBUG_MESSAGE(a_iter->first<<","<<a_iter->second);
//		DEBUG_MESSAGE(road->getAttribute(a_iter->first));
		cost += a_iter->second * ((double) road->getAttribute(a_iter->first));

	}
	return m_link_cost_scale * cost;
}

void patLinkAndPathCost::deleteLinkCoefficient(ARC_ATTRIBUTES_TYPES coef_type) {
	m_link_coefficients.erase(coef_type);
}
void patLinkAndPathCost::setLinkCoefficient(ARC_ATTRIBUTES_TYPES coef_type,
		double coef_value) {
	m_link_coefficients[coef_type] = coef_value;
}
double patLinkAndPathCost::getCost(const patMultiModalPath& path) const {

	double pathCost = 0.0;
	vector<const patArc*> arcs = path.getArcList();
	for (vector<const patArc*>::const_iterator arc_iter = arcs.begin();
			arc_iter != arcs.end(); ++arc_iter) {
		pathCost += getCost(*arc_iter);
	}
	if (m_ps_computer != NULL && m_pathsize_coefficient > 0.0) {
		double ps_value = m_ps_computer->getPS(path);
		pathCost += m_link_cost_scale * m_pathsize_coefficient * log(ps_value);

//		map<patMultiModalPath, double>::const_iterator find_path =
//				m_pathsize->find(path);
//		if (find_path == m_pathsize->end()) {
//			throw RuntimeException(
//					"path size coefficient is specified, but not find");
//		} else {
//			pathCost += m_link_cost_scale * m_pathsize_coefficient
//					* log(find_path->second);
//		}
	}
	return pathCost;
}

double patLinkAndPathCost::getCostWithPathSize(const patMultiModalPath& path,
		double path_size) const {
	double path_cost = getCost(path)
			+ m_link_cost_scale * m_pathsize_coefficient * log(path_size);
	return path_cost;
}
void patLinkAndPathCost::setLinkCostScale(const double linkCostScale) {
	m_link_cost_scale = linkCostScale;
}

double patLinkAndPathCost::getLinkCostScale() const {
	return m_link_cost_scale;
}

const map<ARC_ATTRIBUTES_TYPES, double>& patLinkAndPathCost::getLinkCoefficients() const {
	return m_link_coefficients;
}
const unordered_map<const char*, double>& patLinkAndPathCost::getPathCoefficients() const {
	return m_path_coefficients;
}
double patLinkAndPathCost::getPathSizeCoefficient() const {
	return m_pathsize_coefficient;
}

void patLinkAndPathCost::setPathSizeComputer(patPathSizeComputer* pathsize) {
	m_ps_computer = pathsize->clone();
}
/*
 * Get attributes of a path for the utility function.
 * @param path: the path.
 * @return: hashed map with key and value.
 */
map<string, double> patLinkAndPathCost::getAttributes(
		const patMultiModalPath& path) const {
	map<string, double> attributes;
	for (map<ARC_ATTRIBUTES_TYPES, double>::const_iterator a_iter =
			m_link_coefficients.begin(); a_iter != m_link_coefficients.end();
			++a_iter) {
		//DEBUG_MESSAGE(a_iter->second<<"*"<<arc->getAttribute(a_iter->first));
		double cost = 0.0;
		vector<const patArc*> arcs = path.getArcList();
		for (vector<const patArc*>::const_iterator arc_iter = arcs.begin();
				arc_iter != arcs.end(); ++arc_iter) {
			cost += ((double) (*arc_iter)->getAttribute(a_iter->first));
		}
		attributes[patArc::getAttributeTypeString(a_iter->first)] = cost; //FIXME scale parameter
	}
	if (m_ps_computer != NULL) {
		double ps_value = m_ps_computer->getPS(path);
		attributes["path_size"] = m_ps_computer->getPS(path);
	}
	return attributes;

}
