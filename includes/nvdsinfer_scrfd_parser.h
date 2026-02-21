#ifndef __NVDSINFER_SCRFD_PARSER_H__
#define __NVDSINFER_SCRFD_PARSER_H__

#include <string>
#include "nvdsinfer_custom_impl.h"

// Custom SCRFD parser
extern "C" bool NvDsInferParseCustomSCRFD(
    std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
    NvDsInferNetworkInfo const &networkInfo,
    NvDsInferParseDetectionParams const &detectionParams,
    std::vector<NvDsInferObjectDetectionInfo> &objectList);

#endif 