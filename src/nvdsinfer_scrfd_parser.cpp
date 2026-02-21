#include "nvdsinfer_scrfd_parser.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

// Updated thresholds to match model config
#define CONF_THRESH 0.02f  // from test_cfg.score_thr
#define NMS_THRESH 0.45f   // from test_cfg.nms.iou_threshold

// Model preprocessing params
#define IMG_MEAN 127.5f
#define IMG_STD 128.0f

static float clip(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}

static float IoU(const float *b1, const float *b2) {
  float inter_x1 = std::max(b1[0], b2[0]);
  float inter_y1 = std::max(b1[1], b2[1]);
  float inter_x2 = std::min(b1[2], b2[2]);
  float inter_y2 = std::min(b1[3], b2[3]);
  float inter_area =
      std::max(inter_x2 - inter_x1, 0.f) * std::max(inter_y2 - inter_y1, 0.f);
  float b1_area = (b1[2] - b1[0]) * (b1[3] - b1[1]);
  float b2_area = (b2[2] - b2[0]) * (b2[3] - b2[1]);
  return inter_area / (b1_area + b2_area - inter_area + 1e-6f);
}

// Matches Python's single_distance2bbox function
static void distance2bbox(float center_x, float center_y, const float *distance, float stride, float *bbox) {
  bbox[0] = center_x - distance[0] * stride;  // x1
  bbox[1] = center_y - distance[1] * stride;  // y1
  bbox[2] = center_x + distance[2] * stride;  // x2
  bbox[3] = center_y + distance[3] * stride;  // y2
}

extern "C" bool NvDsInferParseCustomSCRFD(
    std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
    NvDsInferNetworkInfo const &networkInfo,
    NvDsInferParseDetectionParams const &detectionParams,
    std::vector<NvDsInferObjectDetectionInfo> &objectList) {

  const int strides[3] = {8, 16, 32};  // from anchor_generator.strides
  const int fmc = 3;  // Number of outputs per stride (score, bbox, keypoint)
  const int num_layers = 9;  // 3 strides * 3 outputs each

  if (outputLayersInfo.size() != num_layers) {
    std::cerr << "[ERROR] Expected " << num_layers << " output layers, got " 
              << outputLayersInfo.size() << std::endl;
    return false;
  }

  // TensorRT output order for each stride:
  // stride1: score(443), bbox(446), keypoint(449)
  // stride2: score(468), bbox(471), keypoint(474)
  // stride3: score(493), bbox(496), keypoint(499)
  const float *scores[3] = {nullptr, nullptr, nullptr};
  const float *bboxes[3] = {nullptr, nullptr, nullptr};

  // Map layer names to buffers based on TensorRT order
  for (int s = 0; s < 3; s++) {
    int base_idx = s * fmc;
    scores[s] = static_cast<const float*>(outputLayersInfo[base_idx].buffer);
    bboxes[s] = static_cast<const float*>(outputLayersInfo[base_idx + 1].buffer);
    // Keypoints at base_idx + 2 are ignored
  }

  // Verify all required layers were found
  for (int i = 0; i < 3; i++) {
    if (!scores[i] || !bboxes[i]) {
      std::cerr << "[ERROR] Missing output layers for stride " << strides[i] << std::endl;
      return false;
    }
  }

  struct Detection {
    float bbox[4]; // left, top, right, bottom
    float conf;
  };

  std::vector<Detection> dets;

  // Process each feature map
  for (int s = 0; s < 3; ++s) {
    int stride = strides[s];
    int fm_size = networkInfo.width / stride;
    int num_preds = fm_size * fm_size;

    for (int idx = 0; idx < num_preds; ++idx) {
      float confidence = scores[s][idx];

      if (confidence < CONF_THRESH)
        continue;

      int grid_x = idx % fm_size;
      int grid_y = idx / fm_size;

      float anchor_cx = (grid_x + 0.5f) * stride;
      float anchor_cy = (grid_y + 0.5f) * stride;

      const float *deltas = &bboxes[s][idx * 4];
      
      Detection det;
      distance2bbox(anchor_cx, anchor_cy, deltas, stride, det.bbox);

      // Clip to network input boundaries
      det.bbox[0] = clip(det.bbox[0], 0, networkInfo.width);
      det.bbox[1] = clip(det.bbox[1], 0, networkInfo.height);
      det.bbox[2] = clip(det.bbox[2], 0, networkInfo.width);
      det.bbox[3] = clip(det.bbox[3], 0, networkInfo.height);
      
      det.conf = confidence;
      dets.push_back(det);
    }
  }

  // Perform NMS
  std::sort(
      dets.begin(), dets.end(),
      [](const Detection &a, const Detection &b) { return a.conf > b.conf; });

  std::vector<bool> removed(dets.size(), false);

  for (size_t i = 0; i < dets.size(); ++i) {
    if (removed[i])
      continue;
    for (size_t j = i + 1; j < dets.size(); ++j) {
      if (removed[j])
        continue;
      if (IoU(dets[i].bbox, dets[j].bbox) > NMS_THRESH)
        removed[j] = true;
    }
  }

  // Convert to DeepStream detection format
  for (size_t i = 0; i < dets.size(); ++i) {
    if (removed[i])
      continue;
    NvDsInferObjectDetectionInfo obj{};

    obj.classId = 0; // face class
    obj.detectionConfidence = dets[i].conf;

    // Convert to relative coordinates
    obj.left = dets[i].bbox[0] / networkInfo.width;
    obj.top = dets[i].bbox[1] / networkInfo.height;
    obj.width = (dets[i].bbox[2] - dets[i].bbox[0]) / networkInfo.width;
    obj.height = (dets[i].bbox[3] - dets[i].bbox[1]) / networkInfo.height;

    objectList.push_back(obj);
  }

  return true;
} 