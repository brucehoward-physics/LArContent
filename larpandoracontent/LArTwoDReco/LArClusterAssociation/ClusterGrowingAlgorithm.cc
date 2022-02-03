/**
 *  @file   larpandoracontent/LArTwoDReco/LArClusterAssociation/ClusterGrowingAlgorithm.cc
 *
 *  @brief  Implementation of the cluster growing algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArClusterHelper.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "larpandoracontent/LArTwoDReco/LArClusterAssociation/ClusterGrowingAlgorithm.h"

using namespace pandora;

namespace lar_content
{

ClusterGrowingAlgorithm::ClusterGrowingAlgorithm() : m_maxClusterSeparation(2.5f)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterGrowingAlgorithm::Run()
{
    const ClusterList *pClusterList = NULL;

    if (m_inputClusterListName.empty())
    {
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pClusterList));
    }
    else
    {
        PANDORA_RETURN_RESULT_IF_AND_IF(
            STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=, PandoraContentApi::GetList(*this, m_inputClusterListName, pClusterList));
    }

    if (!pClusterList || pClusterList->empty())
    {
        if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
            std::cout << "ClusterGrowingAlgorithm: unable to find cluster list " << m_inputClusterListName << std::endl;

        return STATUS_CODE_SUCCESS;
    }

    ClusterVector inputClusters, seedClusters;
    this->GetListOfCleanClusters(pClusterList, inputClusters);
    this->GetListOfSeedClusters(inputClusters, seedClusters);

    while (true)
    {
        ClusterVector currentClusters, nonSeedClusters;
        this->GetListOfCleanClusters(pClusterList, currentClusters);
        this->GetListOfNonSeedClusters(currentClusters, seedClusters, nonSeedClusters);

        ClusterMergeMap clusterMergeMap;
        this->PopulateClusterMergeMap(seedClusters, nonSeedClusters, clusterMergeMap);

        if (clusterMergeMap.empty())
            break;

	this->CheckInterTPCVolumeAssociations(clusterMergeMap);
	if (clusterMergeMap.empty())
	    break;

        this->MergeClusters(clusterMergeMap);
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

// COPIED FROM CLUSTER ASSOCIATION AND EDITED...
void ClusterGrowingAlgorithm::CheckInterTPCVolumeAssociations(ClusterMergeMap &clusterMergeMap) const
{
    // BH
    std::cout << "Running alg by Andy C to check for unwanted multivolume matches... " << std::endl;
    unsigned int removedFwd = 0;
    unsigned int removedBwd = 0;
    unsigned int removedOther = 0;
    //////////

    // Loop over the associations and get the forward and backward association cluster sets for each one
    // Delete clusters from the sets as appropriate and if the association map ends up empty, delete that too
    ClusterSet unassociatedClusters;
    for (auto & [ pCluster, merges ] : clusterMergeMap)
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

	std::cout << "Merges: before " << merges.size() << ", ";

	auto mergeIter{merges.begin()};
        while (mergeIter != merges.end())
        {
	    std::cout << merges.size() << std::endl;
            const Cluster *const mergeCandidate{*mergeIter};

	    CaloHitList otherHitList;
	    mergeCandidate->GetOrderedCaloHitList().FillCaloHitList(otherHitList);
	    if (otherHitList.empty())
	        continue;
	    // ATTN: Early 2D clustering should preclude input clusters containing mixed volumes, so just check the first hit
	    const LArCaloHit *const pOtherLArCaloHit{dynamic_cast<const LArCaloHit *const>(otherHitList.front())};
	    if (!pOtherLArCaloHit)
	        continue;
	    const unsigned int otherTpcVolume{pOtherLArCaloHit->GetLArTPCVolumeId()};
	    const unsigned int otherDaughterVolume{pOtherLArCaloHit->GetDaughterVolumeId()};

	    if (clusterTpcVolume == otherTpcVolume && clusterDaughterVolume == otherDaughterVolume)
	    {
	        // Same volume
	        ++mergeIter;
	    }
	    else
	    {
	        // Different volume, needs checked
	        //std::cout << "|---------> Found different volumes. First cluster has "
		//	  << caloHitList.size() << " hits, second (fwd) " << otherHitList.size() << " hits ... ";
		float clusterXmin{0.f}, clusterXmax{0.f}, otherXmin{0.f}, otherXmax{0.f};
		pCluster->GetClusterSpanX(clusterXmin, clusterXmax);
		mergeCandidate->GetClusterSpanX(otherXmin, otherXmax);
		//const bool overlap{(clusterXmin >= otherXmin && clusterXmin < otherXmax) ||
		//    (clusterXmax > otherXmin && clusterXmax <= otherXmax) || (clusterXmin <= otherXmin && clusterXmax >= otherXmax)};
		if (true) //(overlap) -> BH: for now try removing all of them
		{
		    // Drift coordinates overlap across volumes, veto
		    mergeIter = merges.erase(mergeIter);
		    // BH
		    //std::cout << "Removed! (overlap " << overlap << ")" << std::endl;
		    removedFwd+=1;
		}
		else
		{
		    // No X overlap, move on
		    //std::cout << "Saved." << std::endl; // BH
		    ++mergeIter;
		}
	    }

	    std::cout << " after " << merges.size() << ", ";
	}

        if (merges.empty())
	    unassociatedClusters.insert(pCluster);
    }

    for (const Cluster *const pCluster : unassociatedClusters) {
        std::cout << " (removed b/c empty) " << std::endl;
        clusterMergeMap.erase(pCluster);
	removedOther += 1;
    }

    std::cout << " & done with this one." << std::endl;

    // BH
    std::cout << "Removed clusters === F:" << removedFwd << " B:" << removedBwd << " O:" << removedOther << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterGrowingAlgorithm::GetListOfNonSeedClusters(
    const ClusterVector &inputClusters, const ClusterVector &seedClusters, ClusterVector &nonSeedClusters) const
{
    for (ClusterVector::const_iterator iter = inputClusters.begin(), iterEnd = inputClusters.end(); iter != iterEnd; ++iter)
    {
        const Cluster *const pCluster = *iter;

        if (seedClusters.end() != std::find(seedClusters.begin(), seedClusters.end(), pCluster))
            continue;

        nonSeedClusters.push_back(pCluster);
    }

    std::sort(nonSeedClusters.begin(), nonSeedClusters.end(), LArClusterHelper::SortByNHits);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterGrowingAlgorithm::PopulateClusterMergeMap(
    const ClusterVector &seedClusters, const ClusterVector &nonSeedClusters, ClusterMergeMap &clusterMergeMap) const
{
    for (ClusterVector::const_iterator nIter = nonSeedClusters.begin(), nIterEnd = nonSeedClusters.end(); nIter != nIterEnd; ++nIter)
    {
        const Cluster *const pNonSeedCluster = *nIter;

        const Cluster *pBestSeedCluster(NULL);
        float bestDistance(m_maxClusterSeparation);

        for (ClusterVector::const_iterator sIter = seedClusters.begin(), sIterEnd = seedClusters.end(); sIter != sIterEnd; ++sIter)
        {
            const Cluster *const pThisSeedCluster = *sIter;
            const float thisDistance(LArClusterHelper::GetClosestDistance(pNonSeedCluster, pThisSeedCluster));

            if (thisDistance < bestDistance)
            {
                pBestSeedCluster = pThisSeedCluster;
                bestDistance = thisDistance;
            }
        }

        if (pBestSeedCluster)
            clusterMergeMap[pBestSeedCluster].push_back(pNonSeedCluster);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterGrowingAlgorithm::MergeClusters(const ClusterMergeMap &clusterMergeMap) const
{
    ClusterList parentClusterList;
    for (const auto &mapEntry : clusterMergeMap)
        parentClusterList.push_back(mapEntry.first);
    parentClusterList.sort(LArClusterHelper::SortByNHits);

    for (const Cluster *const pParentCluster : parentClusterList)
    {
        const ClusterList &clusterList(clusterMergeMap.at(pParentCluster));

        if (clusterList.empty())
            throw StatusCodeException(STATUS_CODE_FAILURE);

        for (const Cluster *const pAssociatedCluster : clusterList)
        {
            if (m_inputClusterListName.empty())
            {
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pParentCluster, pAssociatedCluster));
            }
            else
            {
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=,
                    PandoraContentApi::MergeAndDeleteClusters(*this, pParentCluster, pAssociatedCluster, m_inputClusterListName, m_inputClusterListName));
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterGrowingAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListName", m_inputClusterListName));

    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MaxClusterSeparation", m_maxClusterSeparation));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
