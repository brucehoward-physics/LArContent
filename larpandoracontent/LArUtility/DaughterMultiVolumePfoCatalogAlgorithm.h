/**
 *  @file   larpandoracontent/LArUtility/DaughterMultiVolumePfoCatalogAlgorithm.h
 *
 *  @brief  Header file for the algorithm showing multi-volume PFOs (trying to identify mis-matches)
 *
 *  $Log: $
 */
#ifndef LAR_DAUGHTER_MULTIVOLUME_PFO_CATALOG_H
#define LAR_DAUGHTER_MULTIVOLUME_PFO_CATALOG_H 1

#include "Pandora/Algorithm.h"

namespace lar_content
{

/**
 *  @brief  DaughterMultiVolumePfoCatalogAlgorithm class
 */
class DaughterMultiVolumePfoCatalogAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    DaughterMultiVolumePfoCatalogAlgorithm();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string m_inputPfoListName; ///< The name of the input ParticleFlowObject list to look at
};

} // namespace lar_content

#endif // #ifndef LAR_DAUGHTER_MULTIVOLUME_PFO_CATALOG_H
