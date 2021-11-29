/**
 *  @file   larpandoracontent/LArUtility/DaughterVolumeCatalogAlgorithm.cc
 *
 *  @brief  Implementation of the list pruning algorithm class.
 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArUtility/DaughterVolumeCatalogAlgorithm.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"

using namespace pandora;

namespace lar_content
{

DaughterVolumeCatalogAlgorithm::DaughterVolumeCatalogAlgorithm()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterVolumeCatalogAlgorithm::Run()
{
    const CaloHitList *pCaloHitList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, "CaloHitList2D", pCaloHitList));

    PANDORA_MONITORING_API(SetEveDisplayParameters(this->GetPandora(), true, DETECTOR_VIEW_XZ, -1.f, 1.f, 1.f));

    std::map<std::string, CaloHitList> volumeToHitsMap;
    for (const CaloHit *const pCaloHit : *pCaloHitList)
    {
        const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
        if (!pLArCaloHit)
            continue;
        const HitType view{pLArCaloHit->GetHitType()};
        const unsigned int tpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
        const unsigned int daughterVolume{pLArCaloHit->GetDaughterVolumeId()};
        std::string key{std::to_string(tpcVolume) + "_" + std::to_string(daughterVolume) + "_" + std::to_string(view)};
        if (volumeToHitsMap.find(key) == volumeToHitsMap.end())
            volumeToHitsMap[key] = CaloHitList();
        volumeToHitsMap[key].emplace_back(pLArCaloHit);
    }

    for (const auto & [ key, hits ] : volumeToHitsMap)
    {
        PANDORA_MONITORING_API(VisualizeCaloHits(this->GetPandora(), &hits, key, AUTOITER));
    }

    PANDORA_MONITORING_API(ViewEvent(this->GetPandora()));

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterVolumeCatalogAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    (void)xmlHandle;

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
