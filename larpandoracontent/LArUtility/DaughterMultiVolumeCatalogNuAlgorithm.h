/**
 *  @file   larpandoracontent/LArUtility/DaughterMultiVolumeCatalogNuAlgorithm.h
 *
 *  @brief  Header file for the algorithm showing multi-volume clusters (trying to identify mis-matches) -- Nu Version, just for easier debugging.
 *
 *  $Log: $
 */
#ifndef LAR_DAUGHTER_MULTIVOLUME_CATALOG_NU_H
#define LAR_DAUGHTER_MULTIVOLUME_CATALOG_NU_H 1

#include "Pandora/Algorithm.h"

namespace lar_content
{

/**
 *  @brief  DaughterMultiVolumeCatalogNuAlgorithm class
 */
class DaughterMultiVolumeCatalogNuAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    DaughterMultiVolumeCatalogNuAlgorithm();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    bool        m_LookAllPlanes;         ///< Whether to look for just W plane or all planes...
    std::string m_inputClusterListNameU; ///< The name of the view U cluster list
    std::string m_inputClusterListNameV; ///< The name of the view V cluster list
    std::string m_inputClusterListNameW; ///< The name of the view W cluster list
};

} // namespace lar_content

#endif // #ifndef LAR_DAUGHTER_MULTIVOLUME_CATALOG_NU_H
