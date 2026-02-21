# People Count & Analytics with DeepStream SDK

<p align="center">
  <img src="assets/demo.gif" alt="Demo">
</p>

## Roadmap

- [x] RTSP multi-source occupancy pipeline via `config/deepstream_occupancy_fr_feeds.txt`
- [x] Occupancy analytics (entry/exit/occupancy) with NvDsAnalytics
- [x] Face re-identification stage integrated after occupancy flow
- [ ] Single streamlined DeepStream pipeline for `Occupancy + Face Re-identification`


## Credits

This project is stands on the shoulders of these upstream projects:

| Project | Role | License |
|--------|------|--------|
| **[NVIDIA-AI-IOT/deepstream-occupancy-analytics](https://github.com/NVIDIA-AI-IOT/deepstream-occupancy-analytics)** | Base DeepStream app: PeopleNet, NvDsAnalytics (line-crossing, tripwire), Kafka messaging. We extend it with crowd detection and face re-id stages. | [MIT](https://github.com/NVIDIA-AI-IOT/deepstream-occupancy-analytics/blob/master/LICENSE.md) |
| **[yakhyo/face-reidentification](https://github.com/yakhyo/face-reidentification)** | Face re-identification plugin: SCRFD (face detection) + ArcFace (recognition), ONNX Runtime inference. Used as a submodule under `plugins/face-reidentification`. | See [repo](https://github.com/yakhyo/face-reidentification) |



## Table of Contents

* [Open source & credits](#open-source--credits)
* [Description](#description)
* [Features](#features)
* [Project Structure](#project-structure)
* [Prerequisites](#prerequisites)
* [Roadmap](#roadmap)
* [Getting Started](#getting-started)
* [Build](#build)
* [Run](#run)
* [Output](#output)
* [References](#references)

## Description

This application is provide:

- **Occupancy** — Count people entering/leaving via tripwire (line-crossing) and track current occupancy.
- **Crowd detection** — Overcrowding alerts when the number of people in a configured ROI exceeds a threshold (NvDsAnalytics).
- **Face re-identification** — Identify customers using SCRFD for face detection and ArcFace for recognition (plugin).


Video is processed through the DeepStream pipeline (PeopleNet or custom models, tracker, NvDsAnalytics) and then used by the face re-identification stage.

## Features

| Feature | Description |
|--------|-------------|
| **Occupancy** | Line-crossing (tripwire) for entry/exit counts and live occupancy per source. |
| **Crowd detection** | Configurable ROI and object threshold for overcrowding alerts (NvDsAnalytics). |
| **Face re-identification** | SCRFD + ArcFace based stage after occupancy processing. |


## Prerequisites

- **DeepStream SDK** — [Quick Start Guide](https://docs.nvidia.com/metropolis/deepstream/dev-guide/index.html#page/DeepStream_Development_Guide/deepstream_quick_start.html)
- **PeopleNet model** (for occupancy/crowd) — [NGC PeopleNet](https://catalog.ngc.nvidia.com/orgs/nvidia/teams/tao/models/peoplenet/files)
- **Python 3** — For `plugins/mqtt` and `plugins/face-reidentification` (see their READMEs for pip requirements)

## Getting Started

1. Clone the repo (e.g. under `$DS_SDK_ROOT/sources/apps/sample_apps/` or your workspace):

   ```bash
   git clone --recurse-submodules <repo-url> deepstream_occupancy_fr
   cd deepstream_occupancy_fr
   ```

2. Download PeopleNet model:

   ```bash
   cd config && ./model.sh && cd ..
   ```

3. Set your RTSP sources in `config/deepstream_occupancy_fr_feeds.txt`:

   - Update `uri` under `[source0]`, `[source1]`, etc. with your camera RTSP URLs.
   - Example format: `rtsp://<your-username>:<your-password>@<camera-ip>:554/<stream-path>`

4. Save the file and run with this config.

## Build

1. Set `CUDA_VER` in the Makefile for your platform (e.g. Jetson: `11.4`, x86: `11.8` or `12.6` as in repo).
2. Set `DEEPSTREAM_SDK_ROOT` if not using the default path in the Makefile.

   ```bash
   make
   ```

3. Use `config/deepstream_occupancy_fr_feeds.txt` as the main runtime config.

## Run

Use this config for multi-source RTSP occupancy flow:

```bash
./deepstream-test5-analytics -c config/deepstream_occupancy_fr_feeds.txt
```

## Output

- **On-screen:** OSD with line-crossing counts, overcrowding, and optional face/bbox overlay depending on config.

## References

- [NVIDIA-AI-IOT/deepstream-occupancy-analytics](https://github.com/NVIDIA-AI-IOT/deepstream-occupancy-analytics) — upstream occupancy sample
- [yakhyo/face-reidentification](https://github.com/yakhyo/face-reidentification) — face re-id (SCRFD + ArcFace)
- [Creating Intelligent Places using DeepStream SDK](https://info.nvidia.com/iva-occupancy-webinar-reg-page.html?ondemandrgt=yes)
- [DeepStream SDK](https://developer.nvidia.com/deepstream-sdk)
- [DeepStream Quick Start](https://docs.nvidia.com/metropolis/deepstream/dev-guide/index.html#page/DeepStream_Development_Guide/deepstream_quick_start.html)
- [Transfer Learning Toolkit](https://developer.nvidia.com/transfer-learning-toolkit)
