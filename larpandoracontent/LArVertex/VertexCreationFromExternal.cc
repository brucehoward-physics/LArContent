/**
 *  @file   larpandoracontent/LArVertex/CandidateVertexCreationAlgorithm.cc
 *
 *  @brief  Implementation of the candidate vertex creation algorithm class.
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArVertex/VertexCreationFromExternal.h"

#include <utility>

using namespace pandora;

namespace lar_content
{

VertexCreationFromExternal::VertexCreationFromExternal() :
    m_replaceCurrentVertexList(true)
{
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode VertexCreationFromExternal::Run()
{
    std::cout << " --> Getting calo hit list " << m_inputCaloHitListName << "..." << std::endl;
    const CaloHitList *pCaloHitList(nullptr);
    PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetList(*this, m_inputCaloHitListName, pCaloHitList));
    std::cout << " --> Got that list!" << std::endl;

    std::cout << " --> Creating temporary vertex list and setting as current..." << std::endl;
    const VertexList *pVertexList(NULL);
    std::string temporaryListName;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pVertexList, temporaryListName));
    std::cout << " --> Done." << std::endl;

    unsigned int idxHit=0;
    for (const CaloHit *const pCaloHit : *pCaloHitList)
    {
        std::cout << "     --> Custom hit " << idxHit << std::endl;
        // If hit is NOT type HIT_CUSTOM return STATUS_CODE_FAILURE
        if (HIT_CUSTOM != pCaloHit->GetHitType())
        {
            throw STATUS_CODE_FAILURE;
        }

        // Add to the vertex list
        CartesianVector position3D = pCaloHit->GetPositionVector();

        PandoraContentApi::Vertex::Parameters parameters;
        parameters.m_position = position3D;
        parameters.m_vertexLabel = VERTEX_INTERACTION;
        parameters.m_vertexType = VERTEX_3D;

        std::cout << "!!!! IN THE VERTEX CREATION ALG --> Vertex at ("
                  << position3D.GetX() << ", " << position3D.GetY() << ", " << position3D.GetZ() << ")" << std::endl;

        const Vertex *pVertex(NULL);
        PANDORA_THROW_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::Vertex::Create(*this, parameters, pVertex));
    }

    if (!pVertexList->empty())
    {
        std::cout << "!!!! SAVING LIST!" << std::endl;
        PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<Vertex>(*this, m_outputVertexListName));

        if (m_replaceCurrentVertexList)
            PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<Vertex>(*this, m_outputVertexListName));
    }

    // LOOK AT WHAT IS IN THE LIST
    /*
    const VertexList *pInputVertexList(NULL);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pInputVertexList));

    if (!pInputVertexList || pInputVertexList->empty())
    {
        if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
            std::cout << "VertexSelectionBaseAlgorithm: unable to find current vertex list " << std::endl;

        return STATUS_CODE_SUCCESS;
    }
    else {
        for (const Vertex *const pVertex : *pInputVertexList)
        {
            std::cout << "VERTEX IN LIST AT [" << pVertex->GetPosition().GetX() << ", " << pVertex->GetPosition().GetY() << ", " << pVertex->GetPosition().GetZ() << "]" << std::endl;
        }
    }
    */

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode VertexCreationFromExternal::ReadSettings(const TiXmlHandle xmlHandle)
{

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "InputCaloHitListName", m_inputCaloHitListName));

    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, XmlHelper::ReadValue(xmlHandle, "OutputVertexListName", m_outputVertexListName));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
        XmlHelper::ReadValue(xmlHandle, "ReplaceCurrentVertexList", m_replaceCurrentVertexList));

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
