#include "patObservation.h"
#include "patTraveler.h"
#include "patDisplay.h"
patObservation::patObservation(){
}


patObservation::patObservation( patTraveler* theTraveler,
		 patULong& theTripId):
		traveler(theTraveler),
		id(theTripId)

{

}

/*
* "equal" operator for observations
*@param[in] observation1 observations2 observations to be compared
*@return patTRUE, if the observations have the same id and the same traveler; 
*/
patBoolean operator==(const patObservation& observation1, 
				const patObservation& observation2){
	
	if (observation1.id == observation2.id && 
		observation1.traveler == observation2.traveler){
		return patTRUE;

	}
	else{
		return patFALSE;
	}
	
}
patBoolean operator<(const patObservation& observation1, 
				const patObservation& observation2){
	
	if (observation1.traveler < observation2.traveler){
		return patTRUE;

	}
	if (observation1.traveler > observation2.traveler){
		return patFALSE;

	}
	
	
	return (observation1.id < observation2.id);

}
void patObservation::setStartTime(const patULong& theStartTime){
	startTime = theStartTime;
}

void patObservation::setEndTime(const patULong& theEndTime){
	endTime = theEndTime;
}

void patObservation::setMapBounds(patReal minLat, 
			      patReal maxLat, 
			      patReal minLon, 
			      patReal maxLon) {
				  
  minLatitude = minLat ;
  maxLatitude = maxLat ;
  minLongitude = minLon ;
  maxLongitude = maxLon ;
}

/*
*path utilities
*/
void patObservation::setPathDDRs(const map<patPathJ*,patPathDDR>& thePathDDRs){
	pathDDRs = thePathDDRs;
}


map<patPathJ*,patPathDDR>* patObservation::getPathDDRs(){
	return &pathDDRs;
}

/*
*od utilities
*/
void patObservation::setOdDDRs(const map<patOdJ*,patReal>& theOdDDRs){
	odDDRs = theOdDDRs;
}

map<patOdJ*,patReal>* patObservation::getOdDDRs(){
	return &odDDRs;
}

 patTraveler* patObservation::getTraveler() {
	return traveler;
}

patULong patObservation::getId() {
	return id;
}

/*
*calculate sum of likelihood values of all generated paths
*return a vector with kinds of likelihood from 4 algorithms
*/
vector<patReal> patObservation::calDDRAll(){
	vector<patReal> rtnValues;
	ddrAll = 0.0;
	ddrSimpleAll = 0.0;
	ddrRawAll = 0.0;
	ddrMMAll = 0.0;
	for(map<patPathJ*,vector< patReal> >::iterator pathIter=  pathDDRsReal.begin();
	pathIter!=  pathDDRsReal.end();
	++pathIter){
	
		ddrAll += pathIter->second[0];
		ddrSimpleAll += pathIter->second[1];
		ddrRawAll += pathIter->second[2];
		ddrMMAll += pathIter->second[3];
	}
	rtnValues.push_back(ddrAll);
	rtnValues.push_back(ddrSimpleAll);
	rtnValues.push_back(ddrRawAll);
	rtnValues.push_back(ddrMMAll);
	return rtnValues;
}

patReal patObservation::getDDRAll(){
	return ddrAll;
}


patReal patObservation::getDDRRawAll(){
	return ddrRawAll;
}

/*
* get path candidates ordered by OD
*return a map, the keys are pointers to ODs, the values are map of related paths and likelihood values
*/
map<patOdJ*,map<patPathJ*,patPathDDR> >  patObservation::getPathDDRsOrderByOd_Object(){
	map<patOdJ*,map<patPathJ*,patPathDDR> > pathDDRsOrderByOd;
	
	 for(map<patPathJ*,patPathDDR>::iterator pathIter = pathDDRs.begin();
					pathIter != pathDDRs.end();
					++pathIter){

			pathDDRsOrderByOd[pathIter->first->getOd()] ;

		pathDDRsOrderByOd[pathIter->first->getOd()].insert(*pathIter);
	}
	return pathDDRsOrderByOd;
}

map<patOdJ*,map<patPathJ*,vector<patReal> > >  patObservation::getPathDDRsOrderByOd_Real(){
	map<patOdJ*,map<patPathJ*,vector<patReal> > > pathDDRsOrderByOd;
	
	 for(map<patPathJ*,vector<patReal> >::iterator pathIter = pathDDRsReal.begin();
					pathIter != pathDDRsReal.end();
					++pathIter){

			pathDDRsOrderByOd[pathIter->first->getOd()] ;

		pathDDRsOrderByOd[pathIter->first->getOd()].insert(*pathIter);
	}
	return pathDDRsOrderByOd;
}
map<patPathJ*,vector<patReal> >* patObservation::getPathDDRsReal(){
	return &pathDDRsReal;
}
map<patPathJ*,vector<patReal> >* patObservation::getPathDDRsRealNormalized(){
	return &pathDDRsRealNormalized;
}

	map<patPathJ*,patReal>* patObservation::getPathDDRsRealRaw(){
	return &pathDDRsRealRaw;
	}
/*	
void patObservation::setOdPathSetByDDRs(){
	map<patOdJ*,map<patPathJ*,vector<patReal> > > pathDDRsOrderByOd =getPathDDRsOrderByOd_Real();
	DEBUG_MESSAGE("there are "<<pathDDRsOrderByOd.size()<<" ods.");
	for(map<patOdJ*,map<patPathJ*,vector<patReal> > >::iterator odIter=pathDDRsOrderByOd.begin();
		odIter!=pathDDRsOrderByOd.end();
		++odIter){
		//DEBUG_MESSAGE("there are "<<odIter->second.size()<<" paths for od"<<*(odIter->first));
		odIter->first->setPathSetByDDRs(&(odIter->second));
	}
}
*/
void patObservation::normalizeDDRs(){
vector<patReal> ddrAll = calDDRAll();
	vector<patReal> badDDR;
	badDDR.push_back(9999.9);
	badDDR.push_back(9999.9);
	badDDR.push_back(9999.9);
	badDDR.push_back(9999.9);
	for (map<patPathJ*,vector<patReal> >::iterator pathIter= pathDDRsReal.begin();
		 pathIter!=pathDDRsReal.end();
		 ++pathIter){
		pathDDRsRealNormalized[const_cast<patPathJ*>(pathIter->first)]=badDDR;
	
	}
	
	if (ddrAll[0]!=0.0){
	
	for (map<patPathJ*,vector<patReal> >::iterator pathIter= pathDDRsReal.begin();
		pathIter!=pathDDRsReal.end();
		++pathIter){
		pathDDRsRealNormalized[pathIter->first][0]=pathIter->second[0]/(ddrAll[0]);
//		DEBUG_MESSAGE(pathDDRsRealNormalized[pathIter->first][0]);
	}	 
	
	}
	

	if (ddrAll[1]!=0.0){
	
	for (map<patPathJ*,vector<patReal> >::iterator pathIter= pathDDRsReal.begin();
		pathIter!=pathDDRsReal.end();
		++pathIter){
		pathDDRsRealNormalized[pathIter->first][1]=pathIter->second[1]/(ddrAll[1]);
	}	
	
	}
	if (ddrAll[2]!=0.0){
	
	for (map<patPathJ*,vector<patReal> >::iterator pathIter= pathDDRsReal.begin();
		pathIter!=pathDDRsReal.end();
		++pathIter){
		pathDDRsRealNormalized[pathIter->first][2]=pathIter->second[2]/(ddrAll[2]);
	}	
	
	}
	
	if (ddrAll[3]!=0.0){
	
	for (map<patPathJ*,vector<patReal> >::iterator pathIter= pathDDRsReal.begin();
		pathIter!=pathDDRsReal.end();
		++pathIter){
		pathDDRsRealNormalized[pathIter->first][3]=pathIter->second[3]/(ddrAll[3]);
	}	
	
	}

	}
patPathSampling* patObservation::getPathSampling(patPathJ* aPath){
		map<patPathJ*, patPathSampling>::iterator iter = ChoiceSet.find(aPath);
	if(iter==ChoiceSet.end()){
		WARNING("path is not in the choice set");
		return NULL;		
	}
		return &(iter->second);

}
map<patPathJ*, patPathSampling>* patObservation::getChoiceSet(){
	return &ChoiceSet;
}
void patObservation::sampleChoiceSet(patNetwork* theNetwork,vector< list < pair<patArc*, patULong> > >* adjList,patError*& err){
	DEBUG_MESSAGE("sample choice set for an observation");
	for (map<patPathJ*,vector<patReal> >::iterator pathIter = pathDDRsReal.begin();
				pathIter!=pathDDRsReal.end();
				pathIter++)
	{
		patPathSampling thePathSampling=patPathSampling(theNetwork,adjList,pathIter->first);
		thePathSampling.run(patULong(1),err);
		ChoiceSet.insert(pair<patPathJ*,patPathSampling>(pathIter->first,thePathSampling));

	}
	
	
}