/**
 *  @file   larpandoracontent/LArTwoDReco/LArClusterSplitting/TrackConsolidationAlgorithm.cc
 *
 *  @brief  Implementation of the track consolidation algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "larpandoracontent/LArTwoDReco/LArClusterSplitting/TrackConsolidationAlgorithm.h"

using namespace pandora;

namespace lar_content
{

TrackConsolidationAlgorithm::TrackConsolidationAlgorithm() :
    m_maxTransverseDisplacement(1.f),
    m_minAssociatedSpan(1.f),
    m_minAssociatedFraction(0.5f)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

void TrackConsolidationAlgorithm::GetReclusteredHits(const TwoDSlidingFitResultList &slidingFitResultListI,
    const ClusterVector &showerClustersJ, ClusterToHitMap &caloHitsToAddI, ClusterToHitMap &caloHitsToRemoveJ) const
{
    for (const TwoDSlidingFitResult &slidingFitResultI : slidingFitResultListI)
    {
        const Cluster *const pClusterI = slidingFitResultI.GetCluster();
        const float thisLengthSquaredI(LArClusterHelper::GetLengthSquared(pClusterI));

        for (const Cluster *const pClusterJ : showerClustersJ)
        {
            const float thisLengthSquaredJ(LArClusterHelper::GetLengthSquared(pClusterJ));

            if (pClusterI == pClusterJ)
                continue;

            if (2.f * thisLengthSquaredJ > thisLengthSquaredI)
                continue;

            this->GetReclusteredHits(slidingFitResultI, pClusterJ, caloHitsToAddI, caloHitsToRemoveJ);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void TrackConsolidationAlgorithm::GetReclusteredHits(const TwoDSlidingFitResult &slidingFitResultI, const Cluster *const pClusterJ,
    ClusterToHitMap &caloHitsToAddI, ClusterToHitMap &caloHitsToRemoveJ) const
{
    const Cluster *const pClusterI(slidingFitResultI.GetCluster());

    CaloHitList associatedHits, caloHitListJ;
    pClusterJ->GetOrderedCaloHitList().FillCaloHitList(caloHitListJ);

    // Check inter-volume TPC mixing - for now copy the code here...
    //if( !this->CheckInterTPCVolumeAssociations(pClusterI, pClusterJ) ) return;
    // Re-implementing Andy Chappell's function here basically
    //std::cout << "doing the intervolume check in TrackConsolidationAlg" << std::endl;
    CaloHitList caloHitList_Check;
    pClusterI->GetOrderedCaloHitList().FillCaloHitList(caloHitList_Check);
    if (caloHitList_Check.empty())
      return;
    // ATTN: Early 2D clustering should preclude input clusters containing mixed volumes, so just check the first hit
    const LArCaloHit *const pTargetLArCaloHit{dynamic_cast<const LArCaloHit *const>(caloHitList_Check.front())};
    if (!pTargetLArCaloHit)
      return;
    const unsigned int clusterTpcVolume{pTargetLArCaloHit->GetLArTPCVolumeId()};
    const unsigned int clusterDaughterVolume{pTargetLArCaloHit->GetDaughterVolumeId()};

    CaloHitList caloHitListOther_Check;
    pClusterJ->GetOrderedCaloHitList().FillCaloHitList(caloHitListOther_Check);
    if (caloHitListOther_Check.empty())
      return;
    // ATTN: Early 2D clustering should preclude input clusters containing mixed volumes, so just check the first hit
    const LArCaloHit *const pOtherLArCaloHit{dynamic_cast<const LArCaloHit *const>(caloHitListOther_Check.front())};
    if (!pOtherLArCaloHit)
      return;
    const unsigned int otherTpcVolume{pOtherLArCaloHit->GetLArTPCVolumeId()};
    const unsigned int otherDaughterVolume{pOtherLArCaloHit->GetDaughterVolumeId()};

    if ( clusterTpcVolume != otherTpcVolume || clusterDaughterVolume != otherDaughterVolume ) {
      // Volumes differ, confirm association valid
      // just skip all for now
      return;
      // ----
      float clusterXmin{0.f}, clusterXmax{0.f}, otherXmin{0.f}, otherXmax{0.f};
      pClusterI->GetClusterSpanX(clusterXmin, clusterXmax);
      pClusterJ->GetClusterSpanX(otherXmin, otherXmax);
      const bool overlap{(clusterXmin >= otherXmin && clusterXmin < otherXmax) ||
	  (clusterXmax > otherXmin && clusterXmax <= otherXmax) || (clusterXmin <= otherXmin && clusterXmax >= otherXmax)};
      if (overlap) return;
    }
    // -------------------------------------------------------------

    float minL(std::numeric_limits<float>::max());
    float maxL(std::numeric_limits<float>::max());

    // Loop over hits from shower clusters, and make associations with track clusters
    // (Determine if hits from shower clusters can be used to fill gaps in track cluster)
    //
    // Apply the following selection:
    //  rJ = candidate hit from shower cluster
    //  rI = nearest hit on track cluster
    //  rK = projection of shower hit onto track cluster
    //
    //                  o rJ
    //  o o o o o o - - x - - - - o o o o o o o
    //           rI    rK
    //
    //  Require: rJK < std::min(rCut, rIJ, rKI)

    for (CaloHitList::const_iterator iterJ = caloHitListJ.begin(), iterEndJ = caloHitListJ.end(); iterJ != iterEndJ; ++iterJ)
    {
        const CaloHit *const pCaloHitJ = *iterJ;

        const CartesianVector positionJ(pCaloHitJ->GetPositionVector());
        const CartesianVector positionI(LArClusterHelper::GetClosestPosition(positionJ, pClusterI));

        float rL(0.f), rT(0.f);
        CartesianVector positionK(0.f, 0.f, 0.f);

        if (STATUS_CODE_SUCCESS != slidingFitResultI.GetGlobalFitProjection(positionJ, positionK))
            continue;

        slidingFitResultI.GetLocalPosition(positionK, rL, rT);

        const float rsqIJ((positionI - positionJ).GetMagnitudeSquared());
        const float rsqJK((positionJ - positionK).GetMagnitudeSquared());
        const float rsqKI((positionK - positionI).GetMagnitudeSquared());

        if (rsqJK < std::min(m_maxTransverseDisplacement * m_maxTransverseDisplacement, std::min(rsqIJ, rsqKI)))
        {
            if (associatedHits.empty())
            {
                minL = rL;
                maxL = rL;
            }
            else
            {
                minL = std::min(minL, rL);
                maxL = std::max(maxL, rL);
            }

            associatedHits.push_back(pCaloHitJ);
        }
    }

    const float associatedSpan(maxL - minL);
    const float associatedFraction(
        associatedHits.empty() ? 0.f : static_cast<float>(associatedHits.size()) / static_cast<float>(pClusterJ->GetNCaloHits()));

    if (associatedSpan > m_minAssociatedSpan || associatedFraction > m_minAssociatedFraction)
    {
        for (CaloHitList::const_iterator iterK = associatedHits.begin(), iterEndK = associatedHits.end(); iterK != iterEndK; ++iterK)
        {
            const CaloHit *const pCaloHit = *iterK;
            const CaloHitList &caloHitList(caloHitsToRemoveJ[pClusterJ]);

            if (caloHitList.end() != std::find(caloHitList.begin(), caloHitList.end(), pCaloHit))
                continue;

            caloHitsToAddI[pClusterI].push_back(pCaloHit);
            caloHitsToRemoveJ[pClusterJ].push_back(pCaloHit);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

/*
bool CheckInterTPCVolumeAssociations( const pandora::Cluster *const pTargetCluster, const pandora::Cluster *const pOtherCluster ) const
{
    // Re-implementing Andy Chappell's function here basically
    CaloHitList caloHitList;
    pTargetCluster->GetOrderedCaloHitList().FillCaloHitList(caloHitList);
    if (caloHitList.empty())
        return false;
    // ATTN: Early 2D clustering should preclude input clusters containing mixed volumes, so just check the first hit
    const LArCaloHit *const pTargetLArCaloHit{dynamic_cast<const LArCaloHit *const>(caloHitList.front())};
    if (!pTargetLArCaloHit)
        return false;
    const unsigned int clusterTpcVolume{pTargetLArCaloHit->GetLArTPCVolumeId()};
    const unsigned int clusterDaughterVolume{pTargetLArCaloHit->GetDaughterVolumeId()};

    CaloHitList caloHitListOther;
    pOtherCluster->GetOrderedCaloHitList().FillCaloHitList(caloHitListOther);
    if (caloHitListOther.empty())
      return false;
    // ATTN: Early 2D clustering should preclude input clusters containing mixed volumes, so just check the first hit
    const LArCaloHit *const pOtherLArCaloHit{dynamic_cast<const LArCaloHit *const>(caloHitListOther.front())};
    if (!pOtherLArCaloHit)
      return false;
    const unsigned int otherTpcVolume{pOtherLArCaloHit->GetLArTPCVolumeId()};
    const unsigned int otherDaughterVolume{pOtherLArCaloHit->GetDaughterVolumeId()};

    if ( clusterTpcVolume != otherTpcVolume || clusterDaughterVolume != otherDaughterVolume ) {
        // Volumes differ, confirm association valid
        float clusterXmin{0.f}, clusterXmax{0.f}, otherXmin{0.f}, otherXmax{0.f};
	pTargetCluster->GetClusterSpanX(clusterXmin, clusterXmax);
	pOtherCluster->GetClusterSpanX(otherXmin, otherXmax);
	const bool overlap{(clusterXmin >= otherXmin && clusterXmin < otherXmax) ||
	    (clusterXmax > otherXmin && clusterXmax <= otherXmax) || (clusterXmin <= otherXmin && clusterXmax >= otherXmax)};
	if (overlap) return false;
    }

    return true;
}
*/

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode TrackConsolidationAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
        XmlHelper::ReadValue(xmlHandle, "MaxTransverseDisplacement", m_maxTransverseDisplacement));

    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MinAssociatedSpan", m_minAssociatedSpan));

    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MinAssociatedFraction", m_minAssociatedFraction));

    return TwoDSlidingFitConsolidationAlgorithm::ReadSettings(xmlHandle);
}

} // namespace lar_content
