
#include "yolox.h"

YoloV5Focus::YoloV5Focus() {
    one_blob_only = true;
}

int YoloV5Focus::forward(const ncnn::Mat& bottom_blob, ncnn::Mat& top_blob, const ncnn::Option& opt) const {
    int w = bottom_blob.w;
    int h = bottom_blob.h;
    int channels = bottom_blob.c;

    int outw = w / 2;
    int outh = h / 2;
    int outc = channels * 4;

    top_blob.create(outw, outh, outc, 4u, 1, opt.blob_allocator);
    if (top_blob.empty())
        return -100;

#pragma omp parallel for num_threads(opt.num_threads)
    for (int p = 0; p < outc; p++) {
        const float* ptr = bottom_blob.channel(p % channels).row((p / channels) % 2) + ((p / channels) / 2);
        float* outptr = top_blob.channel(p);

        for (int i = 0; i < outh; i++) {
            for (int j = 0; j < outw; j++) {
                *outptr = *ptr;

                outptr += 1;
                ptr += 2;
            }

            ptr += w;
        }
    }

    return 0;
}

DEFINE_LAYER_CREATOR(YoloV5Focus)

void YoloXDetector::init(const char* param_path, const char* bin_path, const int numThreads) {

    num_threads = numThreads;
    yolox.opt.num_threads = num_threads;
    yolox.opt.use_vulkan_compute = true;
    yolox.register_custom_layer(("YoloV5Focus"), YoloV5Focus_layer_creator);

    yolox.load_param(param_path);
    yolox.load_model(bin_path);

}

int YoloXDetector::detect(const cv::Mat& img, std::vector<Object>& result, int imgsize, float conf, float iou) {
    const int img_w = img.cols;
    const int img_h = img.rows;

    int w = img_w;
    int h = img_h;
    float scale = 1.f;

    if (w > h) {
        scale = static_cast<float>(imgsize) / w;
        w = imgsize;
        h = static_cast<int>(h * scale);
    }
    else {
        scale = static_cast<float>(imgsize) / h;
        h = imgsize;
        w = static_cast<int>(w * scale);
    }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(img.data, ncnn::Mat::PIXEL_BGR, img_w, img_h, w, h);

    // pad to YOLOX_TARGET_SIZE rectangle
    const int wpad = imgsize - w;
    const int hpad = imgsize - h;
    ncnn::Mat in_pad;
    // different from yolov5, yolox only pad on bottom and right side,
    // which means users don't need to extra padding info to decode boxes coordinate.
    ncnn::copy_make_border(in, in_pad, 0, hpad, 0, wpad, ncnn::BORDER_CONSTANT, 114.f);

    ncnn::Extractor ex = yolox.create_extractor();

    ex.input(("images"), in_pad);

    std::vector<Object> proposals;

    {
        ncnn::Mat out;
        ex.extract(("output"), out);

        static constexpr int stride_arr[] = { 8, 16, 32 }; // might have stride=64 in YOLOX
        const std::vector<int> strides(stride_arr, stride_arr + sizeof(stride_arr) / sizeof(stride_arr[0]));
        std::vector<GridAndStride> grid_strides;
        generate_grids_and_stride(imgsize, strides, grid_strides);
        generate_yolox_proposals(grid_strides, out, conf, proposals);
    }

    // sort all proposals by score from highest to lowest
    qsort_descent_inplace_yolox(proposals);

    // apply nms with nms_threshold
    std::vector<int> picked;
    nms_sorted_bboxes_yolox(proposals, picked, iou);

    const int count = picked.size();

    result.resize(count);
    for (int i = 0; i < count; i++) {
        result[i] = proposals[picked[i]];

        // adjust offset to original unpadded
        const float x0 = (result[i].rect.x) / scale;
        const float y0 = (result[i].rect.y) / scale;
        const float x1 = (result[i].rect.x + result[i].rect.width) / scale;
        const float y1 = (result[i].rect.y + result[i].rect.height) / scale;

        // clip
        result[i].rect.x = std::max(std::min(x0, static_cast<float>(img_w - 1)), 0.f);
        result[i].rect.y = std::max(std::min(y0, static_cast<float>(img_h - 1)), 0.f);
        result[i].rect.width = std::max(std::min(x1, static_cast<float>(img_w - 1)), 0.f) - result[i].rect.x;
        result[i].rect.height = std::max(std::min(y1, static_cast<float>(img_h - 1)), 0.f) - result[i].rect.y;
    }

    return 0;
}

inline float YoloXDetector::intersection_area_yolox(const Object& a, const Object& b) {
    const cv::Rect_<float> inter = a.rect & b.rect;
    return inter.area();
}

void YoloXDetector::qsort_descent_inplace_yolox(std::vector<Object>& faceobjects, int left, int right) {
    int i = left;
    int j = right;
    const float p = faceobjects[(left + right) / 2].prob;
    while (i <= j) {
        while (faceobjects[i].prob > p)
            i++;
        while (faceobjects[j].prob < p)
            j--;
        if (i <= j) {
            std::swap(faceobjects[i], faceobjects[j]);
            i++;
            j--;
        }
    }

#pragma omp parallel sections
    {
#pragma omp section
        {
            if (left < j) {
                qsort_descent_inplace_yolox(faceobjects, left, j);
            }
        }
#pragma omp section
        {
            if (i < right) {
                qsort_descent_inplace_yolox(faceobjects, i, right);
            }
        }
    }
}

void YoloXDetector::qsort_descent_inplace_yolox(std::vector<Object>& objects) {

    if (!objects.empty()) {
        qsort_descent_inplace_yolox(objects, 0, objects.size() - 1);
    }
}

void YoloXDetector::nms_sorted_bboxes_yolox(const std::vector<Object>& faceobjects, std::vector<int>& picked, float nms_threshold, bool agnostic) {
    picked.clear();
    const int n = faceobjects.size();
    std::vector<float> areas(n);
    for (int i = 0; i < n; i++) {
        areas[i] = faceobjects[i].rect.area();
    }
    for (int i = 0; i < n; i++) {
        const Object& a = faceobjects[i];
        int keep = 1;
        for (int j = 0; j < static_cast<int>(picked.size()); j++) {
            const Object& b = faceobjects[picked[j]];
            if (!agnostic && a.label != b.label) {
                continue;
            }
            const float inter_area = intersection_area_yolox(a, b);
            const float union_area = areas[i] + areas[picked[j]] - inter_area;
            if (inter_area / union_area > nms_threshold) {
                keep = 0;
            }
        }
        if (keep) {
            picked.push_back(i);
        }
    }
}

void YoloXDetector::generate_grids_and_stride(int target_size, const std::vector<int>& strides, std::vector<GridAndStride>& grid_strides) {
    for (int stride : strides) {
        const int num_grid = target_size / stride;
        for (int g1 = 0; g1 < num_grid; g1++) {
            for (int g0 = 0; g0 < num_grid; g0++) {
                grid_strides.push_back({ g0, g1, stride });
            }
        }
    }

}

void YoloXDetector::generate_yolox_proposals(const std::vector<GridAndStride>& grid_strides, const ncnn::Mat& feat_blob, float prob_threshold, std::vector<Object>& objects) {
    const int num_grid = feat_blob.h;
    const int num_class = feat_blob.w - 5;
    const int num_anchors = grid_strides.size();

    const float* feat_ptr = feat_blob.channel(0);
    for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++) {
        const int grid0 = grid_strides[anchor_idx].grid0;
        const int grid1 = grid_strides[anchor_idx].grid1;
        const int stride = grid_strides[anchor_idx].stride;

        // yolox/models/yolo_head.py decode logic
        //  outputs[..., :2] = (outputs[..., :2] + grids) * strides
        //  outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
        const float x_center = (feat_ptr[0] + grid0) * stride;
        const float y_center = (feat_ptr[1] + grid1) * stride;
        const float w = exp(feat_ptr[2]) * stride;
        const float h = exp(feat_ptr[3]) * stride;
        const float x0 = x_center - w * 0.5f;
        const float y0 = y_center - h * 0.5f;

        const float box_objectness = feat_ptr[4];
        for (int class_idx = 0; class_idx < num_class; class_idx++) {
            const float box_cls_score = feat_ptr[5 + class_idx];
            const float box_prob = box_objectness * box_cls_score;
            if (box_prob > prob_threshold) {
                Object obj;
                obj.rect.x = x0;
                obj.rect.y = y0;
                obj.rect.width = w;
                obj.rect.height = h;
                obj.label = class_idx;
                obj.prob = box_prob;

                objects.push_back(obj);
            }
        } // class loop
        feat_ptr += feat_blob.w;
    } // point anchor loop
}