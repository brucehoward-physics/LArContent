/**
 *  @file   larpandoracontent/LArUtility/DumpLArCaloHits.h
 *
 *  @brief  Header file for the algorithm dumping calo hit info
 *
 *  $Log: $
 */
#ifndef DUMP_LAR_CALO_HITS_H
#define DUMP_LAR_CALO_HITS_H 1

#include "Pandora/Algorithm.h"

namespace lar_content
{

/**
 *  @brief  DumpLArCaloHits class
 */
class DumpLArCaloHits : public pandora::Algorithm
{
public:
    /**
     *  @brief  Default constructor
     */
    DumpLArCaloHits();

private:
    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    std::string m_inputCaloHitListName; ///< The name of the calo hit list
};

} // namespace lar_content

#endif // #ifndef DUMP_LAR_CALO_HITS_H
