/**
 *  @file   larpandoracontent/LArUtility/DaughterVolumeCatalogAlgorithm.h
 *
 *  @brief  Header file for the list pruning algorithm class.
 *
 *  $Log: $
 */
#ifndef LAR_DAUGHTER_VOLUME_CATALOG_H
#define LAR_DAUGHTER_VOLUME_CATALOG_H 1

#include "Pandora/Algorithm.h"

namespace lar_content
{

/**
 *  @brief  DaughterVolumeCatalogAlgorithm class
 */
class DaughterVolumeCatalogAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    DaughterVolumeCatalogAlgorithm();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);
};

} // namespace lar_content

#endif // #ifndef LAR_DAUGHTER_VOLUME_CATALOG_H
