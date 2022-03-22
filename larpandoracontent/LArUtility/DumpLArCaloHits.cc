/**
 *  @file   larpandoracontent/LArUtility/DumpLArCaloHits.cc
 *
 *  @brief  Dump calo hit info...
 
 *  $Log: $
 */

// BH: Big thanks to Andy C for helping me think of how to approach the problem and pointing me toward 
//       DaughterVolumeCatalogAlgorithm that this copies from, and then I copy and edit from other
//       Pandora stuff as well for sure...

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArUtility/DumpLArCaloHits.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include <set>

using namespace pandora;

namespace lar_content
{

DumpLArCaloHits::DumpLArCaloHits()
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DumpLArCaloHits::Run()
{
  const CaloHitList *pCaloHitList(nullptr);
  PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputCaloHitListName, pCaloHitList));

  CaloHitVector pCaloHitVector(pCaloHitList->begin(),pCaloHitList->end());

  std::cout << "NHits:" << pCaloHitVector.size() << std::endl;;
  for ( const CaloHit *const pCaloHit : pCaloHitVector )
  {
    const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(pCaloHit)};
    if (!pLArCaloHit)
      continue;
    // dump info
    const unsigned int tpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
    const unsigned int daughterVolume{pLArCaloHit->GetDaughterVolumeId()};
    const float        hitX0{pCaloHit->GetX0()};
    std::cout << "tpc " << tpcVolume << " dv " << daughterVolume
	      << " pos[0] " << pCaloHit->GetPositionVector().GetX() << " pos[2] " << pCaloHit->GetPositionVector().GetZ()
	      << " x0 " << hitX0 << std::endl;
  }

  return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode DumpLArCaloHits::ReadSettings(const TiXmlHandle xmlHandle)
{
    // Don't think this is needed, is it?
    //(void)xmlHandle;

    // Get the Calo Hit List Names to dump.
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName", m_inputCaloHitListName));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
