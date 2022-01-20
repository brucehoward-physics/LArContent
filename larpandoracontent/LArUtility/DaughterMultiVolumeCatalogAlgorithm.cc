/**
 *  @file   larpandoracontent/LArUtility/DaughterMultiVolumeCatalogAlgorithm.cc
 *
 *  @brief  Implementation of the algorithm showing multi-volume clusters (trying to identify mis-matches)
 
 *  $Log: $
 */

// BH: Big thanks to Andy C for helping me think of how to approach the problem and pointing me toward 
//       DaughterVolumeCatalogAlgorithm that this copies from, and then I copy and edit from other
//       Pandora stuff as well for sure...

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArUtility/DaughterMultiVolumeCatalogAlgorithm.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include <set>

using namespace pandora;

namespace lar_content
{

DaughterMultiVolumeCatalogAlgorithm::DaughterMultiVolumeCatalogAlgorithm()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterMultiVolumeCatalogAlgorithm::Run()
{
    // Will a CaloHitVector work here or does it need to be CaloHitList??
    std::map<std::string, CaloHitList> clstvolumesToHitsMap;
    unsigned int clusterNumber = 0;

    PANDORA_MONITORING_API(SetEveDisplayParameters(this->GetPandora(), true, DETECTOR_VIEW_XZ, -1.f, 1.f, 1.f));

    const ClusterList *pClustersU(nullptr);
    const ClusterList *pClustersV(nullptr);
    const ClusterList *pClustersW(nullptr);

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputClusterListNameW, pClustersW));
    // Loop similar to "fixed" version of BoundedCluster MopUp
    ClusterVector pClusterVecW(pClustersW->begin(),pClustersW->end());
    for ( const Cluster *const pCluster : pClusterVecW )
    {
        CaloHitList clusterHitList;
	pCluster->GetOrderedCaloHitList().FillCaloHitList(clusterHitList);

	CaloHitList pCaloHitsOut;
	std::set< std::tuple< unsigned int, unsigned int, HitType > > setClstVols; // tpcVol, daughterVol, view

	std::map< std::string, CaloHitList > clstHitMap; // count the hits in each plane in this go...

	// As in DaughterVolumeCatalog
	for (const CaloHit *const pCaloHit : clusterHitList)
	{
	    const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
	    if (!pLArCaloHit)
	        continue;
	    pCaloHitsOut.push_back(pLArCaloHit);
	    const HitType view{pLArCaloHit->GetHitType()};
	    const unsigned int tpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
	    const unsigned int daughterVolume{pLArCaloHit->GetDaughterVolumeId()};
	    setClstVols.emplace( std::make_tuple(tpcVolume, daughterVolume, view) );

	    if ( clstHitMap.find( std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view) ) == clstHitMap.end() )
	        clstHitMap[ std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view) ] = CaloHitList();
	    clstHitMap[std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view)].emplace_back(pLArCaloHit);
	}

	if ( setClstVols.size() < 2 ) continue; // not a multi-volume cluster...
	std::string key = std::to_string(clusterNumber);
	for ( auto const& volTuple : setClstVols )
	{
	  key = key +
	        "__" + std::to_string( std::get<0>(volTuple) ) +
	        "_" + std::to_string( std::get<1>(volTuple) ) +
	        "_" + std::to_string( std::get<2>(volTuple) );
	}

	if (clstvolumesToHitsMap.find(key) != clstvolumesToHitsMap.end())
	{
	    std::cout << "ALREADY LOOKED AT THIS KEY!" << std::endl;
	    std::abort();
	}
	std::cout << key << " -- size = " << pCaloHitsOut.size() << " hits ( ";
	for (const auto & [ miniKey, hitList ] : clstHitMap)
	{
	    std::cout << miniKey << "=" << hitList.size() << " ";
	}
	std::cout << ")" << std::endl;
	clstvolumesToHitsMap[key] = pCaloHitsOut;

	clusterNumber+=1;
    }

    if ( m_LookAllPlanes )
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputClusterListNameU, pClustersU));
	ClusterVector pClusterVecU(pClustersU->begin(),pClustersU->end());
	for ( const Cluster *const pCluster : pClusterVecU )
	{
	    CaloHitList clusterHitList;
	    pCluster->GetOrderedCaloHitList().FillCaloHitList(clusterHitList);

	    CaloHitList pCaloHitsOut;
	    std::set< std::tuple< unsigned int, unsigned int, HitType > > setClstVols; // tpcVol, daughterVol, view

	    // As in DaughterVolumeCatalog
	    for (const CaloHit *const pCaloHit : clusterHitList)
	    {
		const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
		if (!pLArCaloHit)
		  continue;
		pCaloHitsOut.push_back(pLArCaloHit);
		const HitType view{pLArCaloHit->GetHitType()};
		const unsigned int tpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
		const unsigned int daughterVolume{pLArCaloHit->GetDaughterVolumeId()};
		setClstVols.emplace( std::make_tuple(tpcVolume, daughterVolume, view) );
	    }

	    if ( setClstVols.size() < 2 ) continue; // not a multi-volume cluster...
	    std::string key = std::to_string(clusterNumber);
	    for ( auto const& volTuple : setClstVols )
	    {
	        key = key +
		      "__" + std::to_string( std::get<0>(volTuple) ) +
		      "_" + std::to_string( std::get<1>(volTuple) ) +
		      "_" + std::to_string( std::get<2>(volTuple) );
	    }

	    if (clstvolumesToHitsMap.find(key) != clstvolumesToHitsMap.end())
	    {
		std::cout << "ALREADY LOOKED AT THIS KEY!" << std::endl;
		std::abort();
	    }
	    clstvolumesToHitsMap[key] = pCaloHitsOut;

	    clusterNumber+=1;
	}

	PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputClusterListNameV, pClustersV));
	ClusterVector pClusterVecV(pClustersV->begin(),pClustersV->end());
	for ( const Cluster *const pCluster : pClusterVecV )
	{
	    CaloHitList clusterHitList;
	    pCluster->GetOrderedCaloHitList().FillCaloHitList(clusterHitList);

	    CaloHitList pCaloHitsOut;
	    std::set< std::tuple< unsigned int, unsigned int, HitType > > setClstVols; // tpcVol, daughterVol, view

	    // As in DaughterVolumeCatalog
	    for (const CaloHit *const pCaloHit : clusterHitList)
	    {
		const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
		if (!pLArCaloHit)
		  continue;
		pCaloHitsOut.push_back(pLArCaloHit);
		const HitType view{pLArCaloHit->GetHitType()};
		const unsigned int tpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
		const unsigned int daughterVolume{pLArCaloHit->GetDaughterVolumeId()};
		setClstVols.emplace( std::make_tuple(tpcVolume, daughterVolume, view) );
	    }

	    if ( setClstVols.size() < 2 ) continue; // not a multi-volume cluster...
	    std::string key = std::to_string(clusterNumber);
	    for ( auto const& volTuple : setClstVols )
	    {
	        key = key +
		      "__" + std::to_string( std::get<0>(volTuple) ) +
		      "_" + std::to_string( std::get<1>(volTuple) ) +
		      "_" + std::to_string( std::get<2>(volTuple) );
	    }

	    if (clstvolumesToHitsMap.find(key) != clstvolumesToHitsMap.end())
	    {
		std::cout << "ALREADY LOOKED AT THIS KEY!" << std::endl;
		std::abort();
	    }
	    clstvolumesToHitsMap[key] = pCaloHitsOut;

	    clusterNumber+=1;
        }
    }

    for (const auto & [ key, hits ] : clstvolumesToHitsMap)
    {
        PANDORA_MONITORING_API(VisualizeCaloHits(this->GetPandora(), &hits, key, AUTOITER));
    }

    PANDORA_MONITORING_API(ViewEvent(this->GetPandora()));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterMultiVolumeCatalogAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    // Don't think this is needed, is it?
    //(void)xmlHandle;

    // Get the Cluster List Names to draw. If m_LookAllPlanes then it expects U, V, and W. If !m_LookAllPlanes then expects just W.
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "LookAllPlanes", m_LookAllPlanes));
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameW", m_inputClusterListNameW));

    if ( m_LookAllPlanes )
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameU", m_inputClusterListNameU));
	PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListNameV", m_inputClusterListNameV));
    }

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
