/**
 *  @file   larpandoracontent/LArVertex/VertexCreationFromExternal.h
 *
 *  @brief  Header file for the candidate vertex creation USING EXTERNAL recob::Vertex saved as Custom Hit List...
 *
 *  $Log: $
 */
#ifndef LAR_VERTEX_CREATION_FROM_EXTERNAL_H
#define LAR_VERTEX_CREATION_FROM_EXTERNAL_H 1

#include "Pandora/Algorithm.h"

namespace lar_content
{

/**
 *  @brief  VertexCreationFromExternal::Algorithm class
 */
class VertexCreationFromExternal : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    VertexCreationFromExternal();

private:
    pandora::StatusCode Run();

    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string m_inputCaloHitListName; ///< The name of the list where the vertex is saved
    std::string m_outputVertexListName; ///< The name under which to save the output vertex list
    bool m_replaceCurrentVertexList;    ///< Whether to replace the current vertex list with the output list
};

} // namespace lar_content

#endif // #ifndef LAR_VERTEX_CREATION_FROM_EXTERNAL_H
