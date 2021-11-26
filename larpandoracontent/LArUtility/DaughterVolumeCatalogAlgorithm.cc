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

    for (const CaloHit *const pCaloHit : *pCaloHitList)
    {
        const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
        if (pLArCaloHit)
            std::cout << "DaughterVolumeCatalogAlgorithm: TPC Volume " << pLArCaloHit->GetLArTPCVolumeId() << " Daughter Volume " <<
                pLArCaloHit->GetDaughterVolumeId() << std::endl;
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DaughterVolumeCatalogAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    (void)xmlHandle;

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
