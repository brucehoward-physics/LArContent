/**
 *  @file   larpandoracontent/LArThreeDReco/LArPfoMopUp/PfoNearGapsAlg.cc
 *
 *  @brief  Implementation of the PfoNearGaps mop-up class.
 *
 *  $Log: Created by Bruce Howard for ICARUS in November 2023, basing heavily on other Pandora modules.
 *        Previously tried this as a PfoRecovery algorithm based directly on pandora::Algorithm, but the MopUp
 *        algorithms have the MergeAndDelete function that I think I want...$
 */

#include "Pandora/AlgorithmHeaders.h"

//#include "larpandoracontent/LArHelpers/LArClusterHelper.h"
//#include "larpandoracontent/LArHelpers/LArGeometryHelper.h"
//#include "larpandoracontent/LArHelpers/LArPointingClusterHelper.h"

#include "larpandoracontent/LArHelpers/LArPfoHelper.h"
#include "larpandoracontent/LArHelpers/LArPcaHelper.h"

#include "larpandoracontent/LArObjects/LArPointingCluster.h"

#include "larpandoracontent/LArThreeDReco/LArPfoMopUp/PfoNearGapsAlg.h"

#include <algorithm>

using namespace pandora;

namespace lar_content
{

PfoNearGapsAlg::PfoNearGapsAlg() :
    m_minClusterCaloHits(40),
    m_zGapHalfLength(4.1),
    m_zLookoutRange(100.),
    m_minAbsCosTh12(0.7), /*0.85*/
    m_minAbsCosThDisp(0.85)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoNearGapsAlg::Run()
{
    const PfoList *pPfoList(nullptr);
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_INITIALIZED, !=, PandoraContentApi::GetList(*this, m_inputPfoListName, pPfoList));

    if (!pPfoList || pPfoList->empty())
    {
        if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
            std::cout << "PfoCharacterisationBaseAlgorithm: unable to find pfo list " << m_inputPfoListName << std::endl;

        return STATUS_CODE_SUCCESS;
    }

    std::map<unsigned int, int> seedPfosToSideOfGap; // -1 means upstream, +1 means downstream
    std::map<unsigned int, std::vector<CartesianVector>> seedPfosToSpsNearGap; // the sps to use to find related tracks

    std::map<unsigned int, std::vector<unsigned int>> seedPfosToAssns; // the association map

    unsigned int iPFO=0;
    for (const ParticleFlowObject *const pPfo : *pPfoList)
    {
        // Get the 3d hits, as in NeutrinoID Tool
        ClusterList clusters3D;
        LArPfoHelper::GetThreeDClusterList(pPfo, clusters3D);

        if (clusters3D.size() > 1) {
            iPFO+=1;
            throw StatusCodeException(STATUS_CODE_OUT_OF_RANGE);
        }

        if (clusters3D.empty()) {
            iPFO+=1;
            continue;
        }

        CaloHitList caloHits;
        clusters3D.front()->GetOrderedCaloHitList().FillCaloHitList(caloHits);

        if ( caloHits.size() < m_minClusterCaloHits ) {
            iPFO+=1;
            continue;
        }

        // Check if its start or end is near a gap:
        // -- For now, consider it to be if at least 6 spacepoints are within -zGapHalfLength < 0 < zGapHalfLength
        unsigned int pointsInGapRegion = 0;
        unsigned int pointsBeforeGapRegion = 0;
        unsigned int pointsAfterGapRegion = 0;
        for (const CaloHit *const pCaloHit : caloHits) {
            if ( pCaloHit->GetPositionVector().GetZ() > -m_zGapHalfLength && pCaloHit->GetPositionVector().GetZ() < m_zGapHalfLength )
                pointsInGapRegion+=1;
            else if ( pCaloHit->GetPositionVector().GetZ() <= -m_zGapHalfLength )
                pointsBeforeGapRegion+=1;
            else if ( pCaloHit->GetPositionVector().GetZ() >= m_zGapHalfLength )
                pointsAfterGapRegion+=1;
        }
        if ( pointsInGapRegion < 6 || (pointsAfterGapRegion > 2 && pointsBeforeGapRegion > 2) ) {
            iPFO+=1;
            continue; // either this doesn't really go much into gap neighboring region, or it spans across it
        }

        bool pfoIsBeforeGap = (pointsBeforeGapRegion > pointsAfterGapRegion);

        // Save the 20 closest spacepoints outside the gap region
        const unsigned int nSavePoints = 20;
        double closestPtXs[20];
        double closestPtYs[20];
        double closestPtZs[20];

        for ( unsigned int idx=0; idx<nSavePoints; ++idx ) {
            closestPtXs[idx] = 9999.;
            closestPtYs[idx] = 9999.;
            closestPtZs[idx] = 9999.;
        }

        for (const CaloHit *const pCaloHit : caloHits) {
            double posZ = pCaloHit->GetPositionVector().GetZ();
            if ( fabs(posZ) < m_zGapHalfLength ) continue;
            for ( unsigned int idx = 0; idx < nSavePoints; ++idx ) {
                if ( posZ < closestPtZs[idx] ) {
                    double tmpX[20];
                    double tmpY[20];
                    double tmpZ[20];
                    for ( unsigned int idx2 = 0; idx2 < nSavePoints; ++idx2 ) {
                        tmpX[idx2] = closestPtXs[idx2];
                        tmpY[idx2] = closestPtYs[idx2];
                        tmpZ[idx2] = closestPtZs[idx2];
                    }
                    closestPtXs[idx] = pCaloHit->GetPositionVector().GetX();
                    closestPtYs[idx] = pCaloHit->GetPositionVector().GetY();
                    closestPtZs[idx] = posZ;
                    for ( unsigned int idx2 = idx+1; idx2 < nSavePoints; ++idx2 ) {
                        closestPtXs[idx2] = tmpX[idx2-1];
                        closestPtYs[idx2] = tmpY[idx2-1];
                        closestPtZs[idx2] = tmpZ[idx2-1];
                    }
                    break;
                }
            } // loop spacepoints to be saved
        } // loop spacepoints

        // Save the points in the map
        std::vector<CartesianVector> spsNearGap;
        for ( unsigned int idx=0; idx<nSavePoints; ++idx ) {
            if ( closestPtZs[idx] > 9998. ) continue;
            spsNearGap.push_back( CartesianVector(closestPtXs[idx],closestPtYs[idx],closestPtZs[idx]) );
        }

        if ( seedPfosToSpsNearGap.find(iPFO)==seedPfosToSpsNearGap.end() ) {
            seedPfosToSideOfGap[iPFO] = (pfoIsBeforeGap ? -1 : 1);
            seedPfosToSpsNearGap[iPFO] = spsNearGap;
        }
        else {
            std::cout << "Already have this Pfo in the map..." << std::endl;
            iPFO+=1;
            throw StatusCodeException(STATUS_CODE_FAILURE);
        }

        iPFO+=1;
    } // loop PFOs

    if ( seedPfosToSideOfGap.size() == 0 )
        return STATUS_CODE_SUCCESS;

    // Try to associate the PFOs that belong together...
    for ( auto const& [pfoId, sideOfGap] : seedPfosToSideOfGap ) {
        // Get the centroid and the direction for this based on the (up to 20) points near the end of this PFO
        LArPcaHelper::WeightedPointVector weightedPointVector;
        for (const auto &point : seedPfosToSpsNearGap[pfoId] )
            weightedPointVector.push_back(std::make_pair(point, 1.));

        CartesianVector thisCentroid(0.f, 0.f, 0.f);
        LArPcaHelper::EigenVectors thisEigenVecs;
        LArPcaHelper::EigenValues thisEigenVals(0.f, 0.f, 0.f);
        LArPcaHelper::RunPca(weightedPointVector, thisCentroid, thisEigenVals, thisEigenVecs);

        CartesianVector thisDir(thisEigenVecs[0]);

        // Look for another PFO to join to this PFO
        std::cout << "Checking for a match for PFO " << pfoId << std::endl;

        unsigned int iPFO=0;
        for (const ParticleFlowObject *const pPfo : *pPfoList) {
            if ( iPFO == pfoId ) {
                iPFO+=1;
                continue;
            }

            // if PFP is already selected as a merge candidate, don't re-select
            bool alreadyUsed = false;
            for ( auto const& [checkPfoId, vecOfCheckPfoMatches] : seedPfosToAssns ) {
                for ( auto const& checkPfoId2 : vecOfCheckPfoMatches ) {
                    if ( iPFO == checkPfoId2 ) {
                        alreadyUsed = true;
                        break;
                    }
                }
                if ( alreadyUsed ) break;
            }
            if ( alreadyUsed ) {
                iPFO+=1;
                continue;
            }

            std::cout << "  - Looking at PFO " << iPFO << std::endl;

            // get this PFOs 3d hits (aka spacepoints)
            ClusterList clusters3D;
            LArPfoHelper::GetThreeDClusterList(pPfo, clusters3D);

            if (clusters3D.size() > 1) {
                iPFO+=1;
                throw StatusCodeException(STATUS_CODE_OUT_OF_RANGE);
            }

            if (clusters3D.empty()) {
                iPFO+=1;
                continue;
            }

            CaloHitList caloHits;
            clusters3D.front()->GetOrderedCaloHitList().FillCaloHitList(caloHits);

            // Check for either the start or end of this PFO to be on the opposite side of z=0
            // and to have at least 6 points within that side of z=0
            unsigned int pointsInNeededRegion = 0;
            unsigned int pointsBeforeGapRegion = 0;
            unsigned int pointsAfterGapRegion = 0;
            LArPcaHelper::WeightedPointVector otherWeightedPointVector;

            for (const CaloHit *const pCaloHit : caloHits) {
                if ( (sideOfGap < 0 &&
                      pCaloHit->GetPositionVector().GetZ() > m_zGapHalfLength &&
                      pCaloHit->GetPositionVector().GetZ() < m_zLookoutRange) ||
                     (sideOfGap > 0 &&
                      pCaloHit->GetPositionVector().GetZ() < -m_zGapHalfLength &&
                      pCaloHit->GetPositionVector().GetZ() > -m_zLookoutRange) ) {
                    pointsInNeededRegion += 1;
                    otherWeightedPointVector.push_back(std::make_pair(pCaloHit->GetPositionVector(), 1.));
                }
                if ( pCaloHit->GetPositionVector().GetZ() <= -m_zGapHalfLength )
                    pointsBeforeGapRegion += 1;
                if ( pCaloHit->GetPositionVector().GetZ() >= m_zGapHalfLength )
                    pointsAfterGapRegion += 1;
            }
            if ( pointsInNeededRegion < 6 || (pointsAfterGapRegion > 2 && pointsBeforeGapRegion > 2) ) {
                iPFO+=1;
                continue; // either this doesn't really go much into needed region, or it spans across it
            }

            std::cout << "    - This PFO is potentially of interest. Note: it has " << caloHits.size() << " spacepoints." << std::endl;
            if ( iPFO==0 || iPFO==1 ) {
                iPFO+=1;
                continue; // JUST FOR THIS TEST...
            }

            CartesianVector otherCentroid(0.f, 0.f, 0.f);
            LArPcaHelper::EigenVectors otherEigenVecs;
            LArPcaHelper::EigenValues otherEigenVals(0.f, 0.f, 0.f);
            LArPcaHelper::RunPca(otherWeightedPointVector, otherCentroid, otherEigenVals, otherEigenVecs);

            CartesianVector otherDir(otherEigenVecs[0]);

            // Now check if they are associated
            // displacement vector
            CartesianVector displacementVector( otherCentroid.GetX()-thisCentroid.GetX(),
                                                otherCentroid.GetY()-thisCentroid.GetY(),
                                                otherCentroid.GetZ()-thisCentroid.GetZ() );

            // check angle between displacement and this pfo vector
            float absCosThisDisp = fabs(thisDir.GetCosOpeningAngle(displacementVector));
            // check angle between this pfo vector and check pfo vector
            float absCosThisOther = fabs(thisDir.GetCosOpeningAngle(otherDir));

            std::cout << "    - This PFO has: absCosThisDisp(" << absCosThisDisp << ") and absCosThisOther(" << absCosThisOther << ")" << std::endl;

            if ( absCosThisDisp > m_minAbsCosThDisp && absCosThisOther > m_minAbsCosTh12 ) {
                if ( seedPfosToAssns.find(pfoId) == seedPfosToAssns.end() )
                    seedPfosToAssns[pfoId] = { iPFO };
                else
                    seedPfosToAssns[pfoId].push_back( iPFO );
            }

            iPFO+=1;
        } // loop pfos to check if they match the seed pfo
    } // loop seed pfos for making the Assn matches

    if ( seedPfosToAssns.size() == 0 )
        return STATUS_CODE_SUCCESS;

    // Check if any of the Assns are other seed pfos and if so join the Assns together
    for ( auto const& [pfoId, assnPfos] : seedPfosToAssns ) {
        for ( auto const& [otherPfoId, otherAssnPfos] : seedPfosToAssns ) {
            if ( pfoId == otherPfoId ) continue;
            for ( auto const& assnPfo : assnPfos ) {
                if ( assnPfo == otherPfoId ) {
                    // Check if this is claiming our pfoId first
                    for ( unsigned int idxCheck=0; idxCheck < otherAssnPfos.size(); ++idxCheck ) {
                        if ( idxCheck == pfoId ) continue;
                        seedPfosToAssns[pfoId].push_back( idxCheck );
                    }
                    seedPfosToAssns.erase(otherPfoId);
                }
            } // check if other seed Pfo is associated to the first Pfo
        } // loop other seed Pfos
    } // loop seed Pfos

    // Build the merge map...
    std::map<const ParticleFlowObject *, std::vector<const ParticleFlowObject *>> seedPfoToMergePfos;

    for ( auto const& [pfoId, assnPfos] : seedPfosToAssns ) {
        unsigned int iPFO=0;
        std::vector<const ParticleFlowObject *> joinedPfos;
        for (const ParticleFlowObject *const pPfo : *pPfoList) {
            for ( unsigned int idx=0; idx<assnPfos.size(); ++idx ) {
                if ( iPFO == assnPfos.at(idx) ){
                    joinedPfos.push_back(pPfo);
                    break;
                }
            } // loop idxs
            iPFO+=1;
        } // loop pfos

        iPFO=0;
        for (const ParticleFlowObject *const pPfo : *pPfoList) {
            if ( iPFO==pfoId ) {
                seedPfoToMergePfos[pPfo] = joinedPfos;
                break;
            }
            iPFO+=1;
        } // loop pfos
    } // loop seed pfos

    // Merge them all now... --> will this work?
    std::cout << "Will be attempting merges from " << seedPfoToMergePfos.size() << " seed PFOs..." << std::endl;
    for ( auto & [seedPfo, mergePfos] : seedPfoToMergePfos ) {
        std::cout << "Trying to merge together a PFO with " << mergePfos.size() << " others!" << std::endl;

        for ( auto mergePfo : mergePfos ) {
            this->MergeAndDeletePfos(seedPfo,mergePfo);
        }
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode PfoNearGapsAlg::ReadSettings(const TiXmlHandle xmlHandle)
{
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputPfoListName", m_inputPfoListName));

    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MinClusterCaloHits", m_minClusterCaloHits));
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "ZGapHalfLength", m_zGapHalfLength));
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "ZLookoutRange", m_zLookoutRange));
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MinAbsCosThDisp", m_minAbsCosThDisp));
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle, "MinAbsCosTh12", m_minAbsCosTh12));

    m_daughterListNames.insert(m_daughterListNames.end(), m_inputPfoListName);

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
