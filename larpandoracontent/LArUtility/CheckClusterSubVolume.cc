/**
 *  @file   larpandoracontent/LArUtility/CheckClusterSubVolume.cc
 *
 *  @brief  Dump calo hit info...
 
 *  $Log: $
 */

// BH: Big thanks to Andy C

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArUtility/CheckClusterSubVolume.h"

#include "larpandoracontent/LArHelpers/LArClusterHelper.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include <set>

using namespace pandora;

namespace lar_content
{

  CheckClusterSubVolume::CheckClusterSubVolume()
  {
  }

  //------------------------------------------------------------------------------------------------------------------------------------------

  StatusCode CheckClusterSubVolume::Run()
  {
    std::cout << "\n\n" << std::endl;
    std::cout << "CheckClusterSubVolume :: Auxiliary info: " << m_auxiliaryInfo << std::endl;

    // Get List of Clusters to check
    const ClusterList *pClusterList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pClusterList));

    // Loop through the clusters and perform the check
    //for ( unsigned int idxCluster = 0; idxCluster < pClusterList->size(); ++idxCluster ) {
    auto itCluster = pClusterList->begin();
    while ( itCluster!=pClusterList->end() ) {
      //Cluster *pCluster = pClusterList[idxCluster];
      const Cluster *pCluster = *itCluster;
      std::map< std::pair<unsigned int,unsigned int>, unsigned int > volumeHitsMap;

      HitType innerHT = pCluster->GetInnerLayerHitType();
      std::string innerView;
      if ( innerHT == 4 ) innerView = "TPC_VIEW_U";
      else if ( innerHT == 5 ) innerView = "TPC_VIEW_V";
      else if ( innerHT == 6 ) innerView = "TPC_VIEW_W";
      else innerView = "UNKNOWN";

      HitType outerHT = pCluster->GetOuterLayerHitType();
      std::string outerView;
      if ( outerHT == 4 ) outerView = "TPC_VIEW_U";
      else if (outerHT== 5 ) outerView = "TPC_VIEW_V";
      else if (outerHT== 6 ) outerView = "TPC_VIEW_W";
      else outerView = "UNKNOWN";

      CaloHitList caloHitList;
      pCluster->GetOrderedCaloHitList().FillCaloHitList(caloHitList);
      if (caloHitList.empty())
	continue;
      const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(caloHitList.front())};
      if (!pLArCaloHit)
	continue;
      const unsigned int clusterTpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
      const unsigned int clusterSubVolume{pLArCaloHit->GetSubVolumeId()};

      // Now check the rest of the calo hits for issues... (mixing...)
      unsigned int noLArCaloHit = 0;
      bool uniqueVolume = true;

      //for ( unsigned int idxHit = 0; idxHit < caloHitList.size(); ++idxHit ) {
      for ( auto const& pCaloHit : caloHitList ) {
	const LArCaloHit *const pOtherCaloHit{dynamic_cast<const LArCaloHit *const>( pCaloHit )}; //caloHitList[idxHit] )};
	if (!pOtherCaloHit) {
	  noLArCaloHit+=1;
	  continue;
	}
	const unsigned int otherTpcVolume{pOtherCaloHit->GetLArTPCVolumeId()};
	const unsigned int otherSubVolume{pOtherCaloHit->GetSubVolumeId()};

	if ( volumeHitsMap.find( std::make_pair(otherTpcVolume,otherSubVolume) ) == volumeHitsMap.end() )
	  volumeHitsMap[ std::make_pair(otherTpcVolume,otherSubVolume) ] = 1;
	else volumeHitsMap[ std::make_pair(otherTpcVolume,otherSubVolume) ] += 1;

	if ( clusterTpcVolume!=otherTpcVolume || clusterSubVolume!=otherSubVolume )
	  uniqueVolume = false;
      } // calo hits

      if ( !uniqueVolume ) {
	std::cout << "Cluster of (inner/outer) type (" << innerView << "/" << outerView << ") with " << caloHitList.size() << " hits in mixed volume." << std::endl;

	for ( auto const &[key, value] : volumeHitsMap )
	  std::cout << "  " << key.first << ":" << key.second << " = " << value << std::endl;
      }

      if ( noLArCaloHit > 0 )
	std::cout << "Cluster of (inner/outer) type (" << innerView << "/" << outerView << ") with " << caloHitList.size() << " hits has " << noLArCaloHit << " non-LArCaloHits..." << std::endl;

      ++itCluster;
    } // clusters

    std::cout << "\n\n" << std::endl;

    return STATUS_CODE_SUCCESS;
  }

  //------------------------------------------------------------------------------------------------------------------------------------------

  StatusCode CheckClusterSubVolume::ReadSettings(const TiXmlHandle xmlHandle)
  {
    // Don't think this is needed, is it?
    //(void)xmlHandle;

    // Auxiliary info to print
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "AuxInfo", m_auxiliaryInfo));

    return STATUS_CODE_SUCCESS;
  }

} // namespace lar_content
