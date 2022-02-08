/**
 *  @file   larpandoracontent/LArTwoDReco/LArClusterAssociation/ClusterExtensionAlgorithm.cc
 *
 *  @brief  Implementation of the cluster extension algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "larpandoracontent/LArTwoDReco/LArClusterAssociation/ClusterExtensionAlgorithm.h"

using namespace pandora;

namespace lar_content
{

void ClusterExtensionAlgorithm::PopulateClusterMergeMap(const ClusterVector &clusterVector, ClusterMergeMap &clusterMergeMap) const
{
    ClusterAssociationMatrix clusterAssociationMatrix;
    this->FillClusterAssociationMatrix(clusterVector, clusterAssociationMatrix);
    this->CheckInterTPCVolumeAssociations(clusterAssociationMatrix);
    this->FillClusterMergeMap(clusterAssociationMatrix, clusterMergeMap);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterExtensionAlgorithm::CheckInterTPCVolumeAssociations(ClusterAssociationMatrix &clusterAssociationMatrix) const
{
    ClusterSet unassociatedClusters;
    for (auto & [ pCluster, clusterAssociationMap ] : clusterAssociationMatrix)
    {
        CaloHitList caloHitList;
        pCluster->GetOrderedCaloHitList().FillCaloHitList(caloHitList);
        if (caloHitList.empty())
            continue;
        // ATTN: Early 2D clustering should preclude input clusters containing mixed volumes, so just check the first hit
        const LArCaloHit *const pLArCaloHit{dynamic_cast<const LArCaloHit *const>(caloHitList.front())};
        if (!pLArCaloHit)
            continue;
        const unsigned int clusterTpcVolume{pLArCaloHit->GetLArTPCVolumeId()};
        const unsigned int clusterDaughterVolume{pLArCaloHit->GetDaughterVolumeId()};

        auto assocIter{clusterAssociationMap.begin()};
        while (assocIter != clusterAssociationMap.end())
        {
            const Cluster *pOtherCluster{static_cast<const Cluster *>(assocIter->first)};
            CaloHitList otherHitList;
            pOtherCluster->GetOrderedCaloHitList().FillCaloHitList(otherHitList);
            if (otherHitList.empty())
                continue;
            const LArCaloHit *const pLArOtherHit{dynamic_cast<const LArCaloHit *const>(otherHitList.front())};
            if (!pLArOtherHit)
                continue;
            const unsigned int otherTpcVolume{pLArOtherHit->GetLArTPCVolumeId()};
            const unsigned int otherDaughterVolume{pLArOtherHit->GetDaughterVolumeId()};

            if (clusterTpcVolume == otherTpcVolume && clusterDaughterVolume == otherDaughterVolume)
            {
                // Same volume, move on
                ++assocIter;
            }
            else
            {
                // Volumes differ, confirm association valid
                float clusterXmin{0.f}, clusterXmax{0.f}, otherXmin{0.f}, otherXmax{0.f};
                float clusterZmin{0.f}, clusterZmax{0.f}, otherZmin{0.f}, otherZmax{0.f};
                pCluster->GetClusterSpanX(clusterXmin, clusterXmax);
                pCluster->GetClusterSpanZ(clusterXmin, clusterXmax, clusterZmin, clusterZmax);
                pOtherCluster->GetClusterSpanX(otherXmin, otherXmax);
                pOtherCluster->GetClusterSpanZ(otherXmin, otherXmax, otherZmin, otherZmax);
                otherXmin -= 0.5f; otherXmax += 0.5f;
                const bool xOverlap{(clusterXmin >= otherXmin && clusterXmin <= otherXmax) ||
                    (clusterXmax >= otherXmin && clusterXmax <= otherXmax) || (clusterXmin <= otherXmin && clusterXmax >= otherXmax)};
                if (xOverlap)
                {
                    // Drift coordinates overlap across volumes, veto
                    assocIter = clusterAssociationMap.erase(assocIter);
                }
                else
                {
                    // No X overlap, move on
                    ++assocIter;
                }
            }
        }
        if (clusterAssociationMap.empty())
            unassociatedClusters.insert(pCluster);
    }

    for (const Cluster *const pCluster : unassociatedClusters)
        clusterAssociationMatrix.erase(pCluster);
}

} // namespace lar_content
