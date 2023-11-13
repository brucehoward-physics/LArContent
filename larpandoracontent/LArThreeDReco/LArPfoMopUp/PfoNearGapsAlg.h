/**
 *  @file   larpandoracontent/LArThreeDReco/LArPfoMopUp/ParticleRecoveryAlgorithm.h
 *
 *  @brief  Header file for the PfoNearGaps mop-up class.
 *
 *  $Log: Created by Bruce Howard for ICARUS in November 2023, basing heavily on other Pandora modules.$
 */
#ifndef LAR_PFO_NEAR_GAPS_ALG_H
#define LAR_PFO_NEAR_GAPS_ALG_H 1

#include "larpandoracontent/LArUtility/PfoMopUpBaseAlgorithm.h"

#include <unordered_map>

namespace lar_content
{

/**
 *  @brief  PfoNearGapsAlg class
 */
class PfoNearGapsAlg : public PfoMopUpBaseAlgorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    PfoNearGapsAlg();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string m_inputPfoListName; ///< The input pfo list name

    unsigned int m_minClusterCaloHits; ///< The min number of hits in base cluster selection method
    float m_zGapHalfLength; ///< Half the length of the gap at z=0: that is the gap goes from -HalfLength to +HalfLength
    float m_zLookoutRange; ///< How far to look forward or backward for tracks to associate (NB: could we do this more generally...)

    float m_minAbsCosTh12; ///< Minimum allowed abs(cos(ThetaThisPfoOtherPfo))
    float m_minAbsCosThDisp; ///< Minimum allowed abs(cos(ThetaThisPfoDisplacementVec))
};

//------------------------------------------------------------------------------------------------------------------------------------------

} // namespace lar_content

#endif // #ifndef LAR_PFO_NEAR_GAPS_ALG_H
