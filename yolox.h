#ifndef YOLOX_H
#define YOLOX_H

#include "ncnn/layer.h"
#include "ncnn/net.h"
#include "mokuai.h"

class YoloV5Focus : public ncnn::Layer {
public:
    YoloV5Focus();

    int forward(const ncnn::Mat& bottom_blob, ncnn::Mat& top_blob, const ncnn::Option& opt) const override;
};



class YoloXDetector {
public:
    YoloXDetector() {};

    void init(const char* param_path, const char* bin_path, const int numThreads);

    int detect(const cv::Mat& img, std::vector<Object>& result, int imgsize, float conf, float iou);

private:
    ncnn::Net yolox;
    static inline int num_threads;

    static inline float intersection_area_yolox(const Object& a, const Object& b);

    static void qsort_descent_inplace_yolox(std::vector<Object>& faceobjects, int left, int right);

    static void qsort_descent_inplace_yolox(std::vector<Object>& objects);

    static void nms_sorted_bboxes_yolox(const std::vector<Object>& faceobjects, std::vector<int>& picked, float nms_threshold, bool agnostic = false);

    static void generate_grids_and_stride(int target_size, const std::vector<int>& strides, std::vector<GridAndStride>& grid_strides);

    static void generate_yolox_proposals(const std::vector<GridAndStride>& grid_strides, const ncnn::Mat& feat_blob, float prob_threshold, std::vector<Object>& objects);

};


#endif // YOLOX_H

/*免责声明 (Disclaimer)
本项目（包括但不限于源代码、文档、相关模型等）的开源仅为了促进计算机视觉、人工智能及C++编程技术的学习与交流。为了明确使用边界及规避相关风险，特此声明：

1. 仅限学习与交流用途
本项目的源代码仅供开发者进行技术学习、研究和代码交流使用。旨在探讨AI技术在图像识别、目标检测及自动化处理领域的应用原理。严禁将本项目用于任何破坏游戏公平性、违反游戏/软件服务条款（ToS）或任何非法用途。

2. 严禁商业用途
本项目的所有源代码及打包编译后的成品软件程序，绝对禁止用于任何形式的商业用途或盈利活动。任何人不得将本项目二次开发后作为商业外挂、辅助工具进行售卖或分发。

3. 遵守法律法规与用户协议
使用者在下载、编译或运行本项目时，必须严格遵守当地法律法规以及相关游戏/软件的用户协议。请勿将本项目应用于任何未经授权的真实游戏环境或多人在线竞技平台中。

4. 风险自担与免责条款
本项目按“原样”提供，作者不对代码的稳定性、安全性或适用性做任何明示或暗示的保证。
因下载、编译、修改、传播或使用本项目所产生的任何直接或间接后果（包括但不限于游戏账号被封禁、设备损坏、数据丢失、法律纠纷等），均由使用者本人自行承担。项目作者及贡献者不承担任何由此引发的法律责任及连带责任。

5. 最终解释权
一旦您下载、查看或使用本项目，即表示您已阅读、理解并同意接受本免责声明的全部条款。作者保留随时修改、更新本声明或终止该开源项目的权利。若发现有严重违反本声明的行为，作者有权要求相关方立即停止违规行为，并保留追究其责任的权利。*/