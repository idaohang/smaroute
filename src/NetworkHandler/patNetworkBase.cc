/*
 * patNetworkBase.cc
 *
 *  Created on: Nov 1, 2011
 *      Author: jchen
 */

#include "patNetworkBase.h"
#include <shapefil.h>
#include "patDisplay.h"
#include "patCreateKmlStyles.h"
#include<iostream>
#include<sstream>
#include<fstream>
#include "patNetworkBase.h"
#include "patTransportMode.h"
#include "kml/dom.h"
using kmldom::DocumentPtr;
using kmldom::KmlFactory;
using kmldom::KmlPtr;
using kmldom::PlacemarkPtr;
using kmldom::FolderPtr;

using kmldom::StylePtr;
using kmldom::LineStylePtr;
patNetworkBase::patNetworkBase() :
		m_minimum_label(DBL_MAX) {

}

patNetworkBase::patNetworkBase(const patNetworkBase& other) :
		m_outgoing_incidents(other.m_outgoing_incidents), m_incoming_incidents(
				other.m_incoming_incidents), m_minimum_label(
				other.m_minimum_label), m_nodes(other.m_nodes), m_network_type(
				other.m_network_type), m_transport_mode(other.m_transport_mode) {
}
unsigned long patNetworkBase::getNodeSize() const {
	return m_outgoing_incidents.size();
}
patNetworkBase::~patNetworkBase() {
}

const unordered_map<const patNode*, set<const patRoadBase*> >* patNetworkBase::getOutgoingIncidents() const {
	return &m_outgoing_incidents;
}

const unordered_map<const patNode*, set<const patRoadBase*> >* patNetworkBase::getIncomingIncidents() const {
	return &m_incoming_incidents;
}
bool patNetworkBase::hasDownStream(const patNode * const a_node) const {

	unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator find_node =
			m_outgoing_incidents.find(a_node);
	if (!find_node->second.empty()) {
		return true;
	} else {
		return false;
	}
}
TransportMode patNetworkBase::getTransportMode() const {
	return m_transport_mode;
}
double patNetworkBase::computeMinimumLabel() {

	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator n_iter =
			m_outgoing_incidents.begin(); n_iter != m_outgoing_incidents.end();
			++n_iter) {
		for (set<const patRoadBase*>::const_iterator road_iter =
				n_iter->second.begin(); road_iter != n_iter->second.end();
				++road_iter) {
			m_minimum_label =
					(m_minimum_label < (*road_iter)->getLength()) ?
							m_minimum_label : (*road_iter)->getLength();
		}
	}
	return m_minimum_label;
}
void patNetworkBase::buildIncomingIncidents() {
	m_incoming_incidents.clear();
	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator n_iter =
			m_outgoing_incidents.begin(); n_iter != m_outgoing_incidents.end();
			++n_iter) {
		for (set<const patRoadBase*>::const_iterator road_iter =
				n_iter->second.begin(); road_iter != n_iter->second.end();
				++road_iter) {
			const patNode* down_node = (*road_iter)->getDownNode();
			m_incoming_incidents[down_node];
			m_incoming_incidents[down_node].insert(*road_iter);
		}
	}
}
double patNetworkBase::getMinimumLabel() const {

	return m_minimum_label;
}

void patNetworkBase::finalizeNetwork() {
	computeMinimumLabel();
	m_nodes.clear();
	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator n_iter =
			m_outgoing_incidents.begin(); n_iter != m_outgoing_incidents.end();
			++n_iter) {
		for (set<const patRoadBase*>::const_iterator road_iter =
				n_iter->second.begin(); road_iter != n_iter->second.end();
				++road_iter) {
			m_nodes.insert((*road_iter)->getUpNode());
			m_nodes.insert((*road_iter)->getDownNode());
		}
	}
	buildIncomingIncidents();
}

void patNetworkBase::exportKML(const string file_path) const {
	string kml_file_path = file_path + ".kml";
	ofstream kml_file(kml_file_path.c_str());
	patCreateKmlStyles doc;
	DocumentPtr document = doc.createStylesForKml();

	KmlFactory* factory = KmlFactory::GetFactory();
	FolderPtr stop_folder = factory->CreateFolder();
	FolderPtr links_folder = factory->CreateFolder();
	FolderPtr signals_folder = factory->CreateFolder();

	stop_folder->set_name(string("Stop"));
	signals_folder->set_name(string("Signals"));
	links_folder->set_name(string("Links"));
	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator incid_iter =
			m_outgoing_incidents.begin();
			incid_iter != m_outgoing_incidents.end(); ++incid_iter) {

		if (incid_iter->first->hasTrafficSignal()) {

			PlacemarkPtr node = incid_iter->first->getKML();
			node->set_styleurl("#signal");
			signals_folder->add_feature(node);
		}
		if (isStop(incid_iter->first)) {
			PlacemarkPtr node = incid_iter->first->getKML();
			node->set_styleurl("#stop");
			stop_folder->add_feature(node);
		}
		for (set<const patRoadBase*>::const_iterator road_iter =
				incid_iter->second.begin();
				road_iter != incid_iter->second.end(); ++road_iter) {
			FolderPtr road = (*road_iter)->getKML(
					getTransportModeString(m_transport_mode));
			links_folder->add_feature(road);
		}
	}

	document->add_feature(stop_folder);
	document->add_feature(links_folder);
	document->add_feature(signals_folder);
	KmlPtr kml = factory->CreateKml();
	kml->set_feature(document);

	kml_file << kmldom::SerializePretty(kml);
	kml_file.close();
	cout << "\t" << kml_file_path << " written" << endl;
}
void patNetworkBase::exportCadytsOSM(const string file_path) const {

	unordered_set<const patArc*> arc_set;
	unordered_set<const patNode*> node_set;
	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator incid_iter =
			m_outgoing_incidents.begin();
			incid_iter != m_outgoing_incidents.end(); ++incid_iter) {

		for (set<const patRoadBase*>::const_iterator road_iter =
				incid_iter->second.begin();
				road_iter != incid_iter->second.end(); ++road_iter) {
			vector<const patArc*> arc_list = (*road_iter)->getArcList();
			for (vector<const patArc*>::iterator arc_iter = arc_list.begin();
					arc_iter != arc_list.end(); ++arc_iter) {
				if (*arc_iter == NULL) {
					continue;
				} else {
					arc_set.insert(*arc_iter);
					node_set.insert((*arc_iter)->getUpNode());
					node_set.insert((*arc_iter)->getDownNode());
				}
			}
		}
	}

	ofstream cadyts_vis_data(file_path.c_str());
	cadyts_vis_data << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	cadyts_vis_data << "\t<osm>" << endl;

	for (unordered_set<const patNode*>::const_iterator node_iter =
			node_set.begin(); node_iter != node_set.end(); ++node_iter) {
		if (*node_iter != NULL) {
			cadyts_vis_data << "\t<node id=\"" << (*node_iter)->getUserId()
					<< "\" lon=\"" << (*node_iter)->getLongitude()
					<< "\" lat=\"" << (*node_iter)->getLatitude() << "\" />"
					<< endl;
		}
	}

	for (unordered_set<const patArc*>::const_iterator arc_iter =
			arc_set.begin(); arc_iter != arc_set.end(); ++arc_iter) {
		if (*arc_iter != NULL) {
			cadyts_vis_data << "\t<way id=\""
					<< (*arc_iter)->getUpNode()->getUserId() << "_"
					<< (*arc_iter)->getDownNode()->getUserId() << "\" >"
					<< endl;
			cadyts_vis_data << "\t\t<nd ref=\""
					<< (*arc_iter)->getUpNode()->getUserId() << "\" />" << endl;
			cadyts_vis_data << "\t\t<nd ref=\""
					<< (*arc_iter)->getDownNode()->getUserId() << "\" />"
					<< endl;
			cadyts_vis_data << "\t</way>" << endl;

		}
	}
	cadyts_vis_data << "</osm>" << endl;
	cadyts_vis_data.close();
}
bool patNetworkBase::exportShpFiles(const string file_path) const {

	string shape_file_path = file_path + ".shp";
	string stop_file_path = file_path + "_stops.shp";
	string traffic_signal_path = file_path + "_signals.shp";
	string node_file_path = file_path + "_nodes.shp";

	SHPHandle shp_file_handler = SHPCreate(shape_file_path.c_str(), SHPT_ARC);
	SHPHandle stop_file_handler = SHPCreate(stop_file_path.c_str(), SHPT_POINT);
	SHPHandle node_file_handler = SHPCreate(node_file_path.c_str(), SHPT_POINT);
	SHPHandle signals_file_handler = SHPCreate(traffic_signal_path.c_str(),
			SHPT_POINT);

	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator incid_iter =
			m_outgoing_incidents.begin();
			incid_iter != m_outgoing_incidents.end(); ++incid_iter) {
		if (incid_iter->first->hasTrafficSignal()) {

			double node_padfX[1] = {
					incid_iter->first->getGeoCoord().longitudeInDegrees };
			double node_padfY[1] = {
					incid_iter->first->getGeoCoord().latitudeInDegrees };
			SHPObject* node_shp_object = SHPCreateSimpleObject(SHPT_POINT, 2,
					node_padfX, node_padfY, NULL);

			int stop_number = SHPWriteObject(signals_file_handler, -1,
					node_shp_object);
			SHPDestroyObject(node_shp_object);
		}
		{

			double node_padfX[1] = {
					incid_iter->first->getGeoCoord().longitudeInDegrees };
			double node_padfY[1] = {
					incid_iter->first->getGeoCoord().latitudeInDegrees };
			SHPObject* node_shp_object = SHPCreateObject(SHPT_POINT,
					incid_iter->first->getUserId(), 0, NULL, NULL, 1,
					node_padfX, node_padfY, NULL, NULL);

			int stop_number = SHPWriteObject(node_file_handler, -1,
					node_shp_object);
			SHPDestroyObject(node_shp_object);

		}
		if (isStop(incid_iter->first)) {
			double node_padfX[1] = {
					incid_iter->first->getGeoCoord().longitudeInDegrees };
			double node_padfY[1] = {
					incid_iter->first->getGeoCoord().latitudeInDegrees };
			SHPObject* node_shp_object = SHPCreateSimpleObject(SHPT_POINT, 2,
					node_padfX, node_padfY, NULL);

			int stop_number = SHPWriteObject(stop_file_handler, -1,
					node_shp_object);
			SHPDestroyObject(node_shp_object);
		}
		for (set<const patRoadBase*>::const_iterator road_iter =
				incid_iter->second.begin();
				road_iter != incid_iter->second.end(); ++road_iter) {
			vector<const patArc*> arc_list = (*road_iter)->getArcList();
			for (vector<const patArc*>::iterator arc_iter = arc_list.begin();
					arc_iter != arc_list.end(); ++arc_iter) {
				if (*arc_iter == NULL) {
					continue;
				}
				double up_node_x =
						(*arc_iter)->getUpNode()->getGeoCoord().longitudeInDegrees;
				double up_node_y =
						(*arc_iter)->getUpNode()->getGeoCoord().latitudeInDegrees;
				double down_node_x =
						(*arc_iter)->getDownNode()->getGeoCoord().longitudeInDegrees;
				double down_node_y =
						(*arc_iter)->getDownNode()->getGeoCoord().latitudeInDegrees;

				double padfX[2] = { up_node_x, down_node_x };
				double padfY[2] = { up_node_y, down_node_y };

				SHPObject* path_shp_object = SHPCreateSimpleObject(SHPT_ARC, 2,
						padfX, padfY, NULL);
				int object_number = SHPWriteObject(shp_file_handler, -1,
						path_shp_object);
				SHPDestroyObject(path_shp_object);
			}
		}
	}

	SHPClose(shp_file_handler);
	SHPClose(node_file_handler);
	SHPClose(signals_file_handler);
	SHPClose(stop_file_handler);
	cout << "\tshape files written" << endl;
	return true;
}

set<const patArc*> patNetworkBase::getAllArcs() const {
	set<const patArc*> all_arcs;
	for (unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator node_iter =
			m_outgoing_incidents.begin();
			node_iter != m_outgoing_incidents.end(); ++node_iter) {
		for (set<const patRoadBase*>::const_iterator road_iter =
				node_iter->second.begin(); road_iter != node_iter->second.end();
				++road_iter) {
			vector<const patArc*> arc_list = (*road_iter)->getArcList();
			all_arcs.insert(arc_list.begin(), arc_list.end());
		}
	}
	return all_arcs;
}

void patNetworkBase::addArc(const patArc* arc) {
	const patNode* up_node = arc->getUpNode();
	m_outgoing_incidents[up_node];
	m_outgoing_incidents[up_node].insert(arc);
}

bool patNetworkBase::isPT() const {
	return isPublicTransport(m_transport_mode);
}
set<const patNode*> patNetworkBase::getNodes() const {
	return m_nodes;
}

void patNetworkBase::removeNode(const patNode* node) {
	m_outgoing_incidents.erase(node);

}
const set<const patRoadBase*> patNetworkBase::getOutgoingRoads(
		const patNode* node) const {
	unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator find_outgoing =
			m_outgoing_incidents.find(node);
	if (find_outgoing == m_outgoing_incidents.end()) {
		return set<const patRoadBase*>();
	} else {
		return find_outgoing->second;
	}

}

/**
 * 0 normal status
 * 1 intermediate node
 * 2 deadend
 */
NODE_STATUS patNetworkBase::checkNodeStatus(const patNode* node) const {

	unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator find_outgoing =
			m_outgoing_incidents.find(node);
	unordered_map<const patNode*, set<const patRoadBase*> >::const_iterator find_incoming =
			m_incoming_incidents.find(node);

	if (find_outgoing == m_outgoing_incidents.end()
			|| find_outgoing->second.empty()
			|| find_incoming == m_incoming_incidents.end()
			|| find_incoming->second.empty()) {
		return DEADEND;
	}

	set<const patNode*> outgoing_nodes;
	set<const patNode*> incoming_nodes;

	for (set<const patRoadBase*>::const_iterator arc_iter =
			find_outgoing->second.begin();
			arc_iter != find_outgoing->second.end(); ++arc_iter) {
		outgoing_nodes.insert((*arc_iter)->getDownNode());
	}
	short incidents = 0;
	short duplicate_incoming=0;
	for (set<const patRoadBase*>::const_iterator arc_iter =
			find_incoming->second.begin();
			arc_iter != find_incoming->second.end(); ++arc_iter) {
		const patNode* new_node =(*arc_iter)->getUpNode();
		++incidents;
		if (outgoing_nodes.find(new_node)!=outgoing_nodes.end()){
			++duplicate_incoming;
		}
	}

	short valid_incidents = incidents+ outgoing_nodes.size()-duplicate_incoming;

	if(valid_incidents<=1){
		return DEADEND;
	}
	else if(valid_incidents==2){
		return INTERMEDIATE;
	}
	else{
		return NORMAL;
	}
}
