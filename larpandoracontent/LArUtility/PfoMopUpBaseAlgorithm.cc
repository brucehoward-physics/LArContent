/**
 *  @file   larpandoracontent/LArUtility/PfoMopUpBaseAlgorithm.cc
 *
 *  @brief  Implementation of the pfo mop up algorithm base class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArClusterHelper.h"

#include "larpandoracontent/LArUtility/PfoMopUpBaseAlgorithm.h"

using namespace pandora;

namespace lar_content
{

void PfoMopUpBaseAlgorithm::MergeAndDeletePfos(const ParticleFlowObject *const pPfoToEnlarge, const ParticleFlowObject *const pPfoToDelete) const
{
    if (pPfoToEnlarge == pPfoToDelete)
        throw StatusCodeException(STATUS_CODE_NOT_ALLOWED);

    std::cout << " ~~~ MERGE AND DELETE thinks these are different PFOs... " << std::endl;

    const PfoList daughterPfos(pPfoToDelete->GetDaughterPfoList());
    const ClusterVector daughterClusters(pPfoToDelete->GetClusterList().begin(), pPfoToDelete->GetClusterList().end());
    const VertexVector daughterVertices(pPfoToDelete->GetVertexList().begin(), pPfoToDelete->GetVertexList().end());

    std::cout << " ~~~ Attempting to delete pPfoToDelete... " << std::endl;
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Delete(*this, pPfoToDelete, this->GetListName(pPfoToDelete)));
    std::cout << "                                      ... DONE." << std::endl;

    std::cout << " ~~~ Handling daughter PFOs... " << std::endl;
    for (const ParticleFlowObject *const pDaughterPfo : daughterPfos)
    {
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SetPfoParentDaughterRelationship(*this, pPfoToEnlarge, pDaughterPfo));
    }
    std::cout << "                           ... DONE." << std::endl;

    std::cout << " ~~~ Handling daughter vertices... " << std::endl;
    for (const Vertex *const pDaughterVertex : daughterVertices)
    {
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Delete(*this, pDaughterVertex, this->GetListName(pDaughterVertex)));
    }
    std::cout << "                               ... DONE." << std::endl;

    std::cout << " ~~~ Handling daughter clusters... " << std::endl;
    unsigned int bh_idx=0;
    for (const Cluster *const pDaughterCluster : daughterClusters)
    {
        std::cout << "                               ... Cluster " << bh_idx << std::endl;
        bh_idx+=1;
        const HitType daughterHitType(LArClusterHelper::GetClusterHitType(pDaughterCluster));
        const Cluster *pParentCluster(PfoMopUpBaseAlgorithm::GetParentCluster(pPfoToEnlarge->GetClusterList(), daughterHitType));

        if ( daughterHitType == TPC_VIEW_U ) std::cout << "                               ... Type TpcU" << std::endl;
        if ( daughterHitType == TPC_VIEW_V ) std::cout << "                               ... Type TpcV" << std::endl;
        if ( daughterHitType == TPC_VIEW_W ) std::cout << "                               ... Type TpcW" << std::endl;
        if ( daughterHitType == TPC_3D ) std::cout << "                               ... Type Tpc3D" << std::endl;

        // BH: TEMP?
        CaloHitList parentCaloHits;
        pParentCluster->GetOrderedCaloHitList().FillCaloHitList(parentCaloHits);
        CaloHitList daughterCaloHits;
        pDaughterCluster->GetOrderedCaloHitList().FillCaloHitList(daughterCaloHits);
        std::cout << "                               ... Parent cluster hits:   " << parentCaloHits.size() << std::endl;
        std::cout << "                               ... Daughter cluster hits: " << daughterCaloHits.size() << std::endl;
        //////////////////////////////

        if (pParentCluster)
        {
            std::cout << "                               ... Found parent cluster: merging..." << std::endl;
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=,
                PandoraContentApi::MergeAndDeleteClusters(
                    *this, pParentCluster, pDaughterCluster, this->GetListName(pParentCluster), this->GetListName(pDaughterCluster)));
            std::cout << "                                                                ...Done." << std::endl;
        }
        else
        {
            std::cout << "                               ... NO parent cluster: making..." << std::endl;
            PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::AddToPfo(*this, pPfoToEnlarge, pDaughterCluster));
            std::cout << "                                                            ...Done." << std::endl;
        }
    }
    std::cout << "                               ... DONE." << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

const Cluster *PfoMopUpBaseAlgorithm::GetParentCluster(const ClusterList &clusterList, const HitType hitType)
{
    unsigned int mostHits(0);
    const Cluster *pBestParentCluster(nullptr);

    for (const Cluster *const pParentCluster : clusterList)
    {
        if (hitType != LArClusterHelper::GetClusterHitType(pParentCluster))
            continue;

        const unsigned int nParentHits(pParentCluster->GetNCaloHits());

        if (nParentHits > mostHits)
        {
            mostHits = nParentHits;
            pBestParentCluster = pParentCluster;
        }
    }

    return pBestParentCluster;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoMopUpBaseAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    return MopUpBaseAlgorithm::ReadSettings(xmlHandle);
}

} // namespace lar_content
