/**
 *  @file   larpandoracontent/LArTwoDReco/LArClusterAssociation/ClusterMergingAlgorithm.cc
 *
 *  @brief  Implementation of the cluster merging algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArHelpers/LArClusterHelper.h"
#include "larpandoracontent/LArObjects/LArCaloHit.h"

#include "larpandoracontent/LArTwoDReco/LArClusterAssociation/ClusterMergingAlgorithm.h"

using namespace pandora;

namespace lar_content
{

StatusCode ClusterMergingAlgorithm::Run()
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
            std::cout << "ClusterMergingAlgorithm: unable to find cluster list " << m_inputClusterListName << std::endl;

        return STATUS_CODE_SUCCESS;
    }

    while (true)
    {
        ClusterVector unsortedVector, clusterVector;
        this->GetListOfCleanClusters(pClusterList, unsortedVector);
        this->GetSortedListOfCleanClusters(unsortedVector, clusterVector);

        ClusterMergeMap clusterMergeMap;
        this->PopulateClusterMergeMap(clusterVector, clusterMergeMap);

        if (clusterMergeMap.empty())
            break;

	this->CheckInterTPCVolumeAssociations(clusterMergeMap);
	if (clusterMergeMap.empty())
	    break;

        this->MergeClusters(clusterVector, clusterMergeMap);
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

// Copied from ClusterGrowingAlg
void ClusterMergingAlgorithm::CheckInterTPCVolumeAssociations(ClusterMergeMap &clusterMergeMap) const
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
		//  << caloHitList.size() << " hits, second (fwd) " << otherHitList.size() << " hits ... ";
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

void ClusterMergingAlgorithm::MergeClusters(ClusterVector &clusterVector, ClusterMergeMap &clusterMergeMap) const
{
    ClusterSet clusterVetoList;

    for (const Cluster *const pSeedCluster : clusterVector)
    {
        ClusterList mergeList;
        this->CollectAssociatedClusters(pSeedCluster, pSeedCluster, clusterMergeMap, clusterVetoList, mergeList);
        mergeList.sort(LArClusterHelper::SortByNHits);

        for (const Cluster *const pAssociatedCluster : mergeList)
        {
            if (clusterVetoList.count(pAssociatedCluster))
                throw StatusCodeException(STATUS_CODE_FAILURE);

            if (!pAssociatedCluster->IsAvailable())
                throw StatusCodeException(STATUS_CODE_FAILURE);

            (void)clusterVetoList.insert(pAssociatedCluster);

            if (m_inputClusterListName.empty())
            {
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pSeedCluster, pAssociatedCluster));
            }
            else
            {
                PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=,
                    PandoraContentApi::MergeAndDeleteClusters(*this, pSeedCluster, pAssociatedCluster, m_inputClusterListName, m_inputClusterListName));
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterMergingAlgorithm::CollectAssociatedClusters(
    const Cluster *const pSeedCluster, const ClusterMergeMap &clusterMergeMap, ClusterList &associatedClusterList) const
{
    ClusterSet clusterVetoList;
    this->CollectAssociatedClusters(pSeedCluster, pSeedCluster, clusterMergeMap, clusterVetoList, associatedClusterList);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterMergingAlgorithm::CollectAssociatedClusters(const Cluster *const pSeedCluster, const Cluster *const pCurrentCluster,
    const ClusterMergeMap &clusterMergeMap, const ClusterSet &clusterVetoList, ClusterList &associatedClusterList) const
{
    if (clusterVetoList.count(pCurrentCluster))
        return;

    ClusterMergeMap::const_iterator iter1 = clusterMergeMap.find(pCurrentCluster);

    if (iter1 == clusterMergeMap.end())
        return;

    ClusterVector associatedClusters(iter1->second.begin(), iter1->second.end());
    std::sort(associatedClusters.begin(), associatedClusters.end(), LArClusterHelper::SortByNHits);

    for (const Cluster *const pAssociatedCluster : associatedClusters)
    {
        if (pAssociatedCluster == pSeedCluster)
            continue;

        if (associatedClusterList.end() != std::find(associatedClusterList.begin(), associatedClusterList.end(), pAssociatedCluster))
            continue;

        associatedClusterList.push_back(pAssociatedCluster);
        this->CollectAssociatedClusters(pSeedCluster, pAssociatedCluster, clusterMergeMap, clusterVetoList, associatedClusterList);
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void ClusterMergingAlgorithm::GetSortedListOfCleanClusters(const ClusterVector &inputClusters, ClusterVector &outputClusters) const
{
    ClusterVector pfoClusters, availableClusters;

    for (ClusterVector::const_iterator iter = inputClusters.begin(), iterEnd = inputClusters.end(); iter != iterEnd; ++iter)
    {
        const Cluster *const pCluster = *iter;

        if (!pCluster->IsAvailable())
        {
            pfoClusters.push_back(pCluster);
        }
        else
        {
            availableClusters.push_back(pCluster);
        }
    }

    std::sort(pfoClusters.begin(), pfoClusters.end(), LArClusterHelper::SortByNHits);
    std::sort(availableClusters.begin(), availableClusters.end(), LArClusterHelper::SortByNHits);

    outputClusters.insert(outputClusters.end(), pfoClusters.begin(), pfoClusters.end());
    outputClusters.insert(outputClusters.end(), availableClusters.begin(), availableClusters.end());
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode ClusterMergingAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "InputClusterListName", m_inputClusterListName));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
