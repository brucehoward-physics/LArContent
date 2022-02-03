/**
 *  @file   larpandoracontent/LArUtility/DaughterMultiVolumePfoCatalogAlgorithm.cc
 *
 *  @brief  Implementation of the algorithm showing multi-volume PFOs (trying to identify mis-matches) 
 *  $Log: $
 */

// BH: Big thanks to Andy C for helping me think of how to approach the problem and pointing me toward 
//       DaughterVolumeCatalogAlgorithm that this copies from, and then I copy and edit from other
//       Pandora stuff as well for sure...

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArUtility/DaughterMultiVolumePfoCatalogAlgorithm.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include <set>

using namespace pandora;

namespace lar_content
{

DaughterMultiVolumePfoCatalogAlgorithm::DaughterMultiVolumePfoCatalogAlgorithm()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterMultiVolumePfoCatalogAlgorithm::Run()
{
    // GET LIST OF CLUSTERS FOR THE PFO THEN DO A SIMILAR CHECK TO THIS FOR EACH CUSTER, SO COUNT HOW MANY HITS ARE BEING SAVED ON EACH VOLUME PER PFO INSTEAD OF PER CLUSTER.
    const PfoList *pAllPfos(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputPfoListName, pAllPfos));

    unsigned int pfoNumber = 0;

    PfoVector pAllPfosVec(pAllPfos->begin(),pAllPfos->end());
    for ( const ParticleFlowObject *const pPfo : pAllPfosVec ) {
      const ClusterList pClusters = pPfo->GetClusterList();

      //CaloHitList pCaloHitsOut;
      std::set< std::tuple< unsigned int, unsigned int, HitType > > setPfoVols; // tpcVol, daughterVol, view
      std::map< std::string, CaloHitList > pfoHitMap; // count the hits in each plane

      ClusterVector pClusterVec(pClusters.begin(),pClusters.end());
      for ( const Cluster *const pCluster : pClusterVec ) {
	CaloHitList clusterHitList;
        pCluster->GetOrderedCaloHitList().FillCaloHitList(clusterHitList);

	for (const CaloHit *const pCaloHit : clusterHitList)
	{
            const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
            if (!pLArCaloHit)
	      continue;
            const HitType view{pLArCaloHit->GetHitType()};
            const unsigned int tpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
            const unsigned int daughterVolume{pLArCaloHit->GetDaughterVolumeId()};
            setPfoVols.emplace( std::make_tuple(tpcVolume, daughterVolume, view) );

            if ( pfoHitMap.find( std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view) ) == pfoHitMap.end() )
	      pfoHitMap[ std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view) ] = CaloHitList();
            pfoHitMap[std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view)].emplace_back(pLArCaloHit);
	}
      }

      // For ease of use, turn the set of volumes with hits into a vector...
      std::vector< std::tuple< unsigned int, unsigned int, HitType > > vecPfoVols(setPfoVols.begin(), setPfoVols.end());

      // Now look at the pfoHitMap for repeated Views across multiple tpcVolume, daughterVolume pairs...
      std::map< std::string, int > writtenPfoVols;
      for ( unsigned int iPfoVol=0; iPfoVol<vecPfoVols.size(); ++iPfoVol ) {
	const std::pair<unsigned int, unsigned int> checkTD = std::make_pair( std::get<0>( vecPfoVols[iPfoVol] ), std::get<1>( vecPfoVols[iPfoVol] ) );
	const HitType checkHT = std::get<2>( vecPfoVols[iPfoVol] );
	std::string statusString = std::to_string(checkTD.first) + "_" + std::to_string(checkTD.second) + "_" + std::to_string(checkHT);
	unsigned int volHits = pfoHitMap[ statusString ].size();

	if ( writtenPfoVols.find(statusString) != writtenPfoVols.end() ) continue;

	std::vector<std::string> printStrings;
	std::vector<unsigned int> printSizes;
	for ( unsigned int jPfoVol=iPfoVol+1; jPfoVol<vecPfoVols.size(); ++jPfoVol ) {
	  const std::pair<unsigned int, unsigned int> otherTD = std::make_pair( std::get<0>(  vecPfoVols[jPfoVol] ), std::get<1>(  vecPfoVols[jPfoVol] ) );
	  const HitType otherHT = std::get<2>(  vecPfoVols[jPfoVol] );

	  if ( otherHT != checkHT ) continue;

	  printStrings.push_back( std::to_string(otherTD.first) + "_" + std::to_string(otherTD.second)+ "_" +std::to_string(otherHT) );
	  printSizes.push_back( pfoHitMap[ std::to_string(otherTD.first) + "_" + std::to_string(otherTD.second)+ "_" +std::to_string(otherHT) ].size() );
	}

	if ( printStrings.empty() ) continue;

	printStrings.emplace( printStrings.begin(), statusString );
	printSizes.emplace( printSizes.begin(), volHits );

	std::cout << "Mixing ";
	for ( unsigned int iStr=0; iStr<printStrings.size(); ++iStr ) {
	  std::cout << printStrings[iStr] << " (" << printSizes[iStr] << ") ";
	  if ( writtenPfoVols.find( printStrings[iStr] ) == writtenPfoVols.end() )
	    writtenPfoVols[ printStrings[iStr] ] = 1;
	}
	std::cout << std::endl;
      }

      pfoNumber+=1;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterMultiVolumePfoCatalogAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    // Don't think this is needed, is it?
    //(void)xmlHandle;

    // Get the input Pfo List name:
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputPfoListName", m_inputPfoListName));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
