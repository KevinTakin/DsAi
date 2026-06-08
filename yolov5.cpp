#include "yolov5.h"

static std::string feature_layer_1;
static std::string feature_layer_2;
static std::string feature_layer_3;

static int img_h, img_w, w_pad, h_pad;
static float scale = 1.f;



void Get_Output_Layer_Name_v5(const std::string& temp) {
    const auto idx = temp.find_last_of("0=1");
    if (idx != std::string::npos) {
        const auto temp_str = temp.substr(0, idx);
        const auto idx2 = temp_str.find_last_of(" ");
        if (idx2 != std::string::npos) {
            const auto output_name = temp_str.substr(idx2 + 1);
            const auto temp_str2 = temp_str.substr(0, idx2);
            const auto idx3 = temp_str2.find_last_of(" ");
            if (idx3 != std::string::npos) {
                const auto layer_name = temp_str2.substr(idx3 + 1);

                if (feature_layer_1.empty()) {
                    feature_layer_1 = layer_name;
                }
                else if (feature_layer_2.empty() && layer_name != feature_layer_1) {
                    feature_layer_2 = layer_name;
                }
                else if (feature_layer_3.empty() && layer_name != feature_layer_1 && layer_name != feature_layer_2) {
                    feature_layer_3 = layer_name;
                }
            }
        }
    }
}

bool Find_Output_Layer_v5(const char* pathname) {
    std::ifstream fileline(pathname);
    if (!fileline.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(fileline, line)) {
        if (line.find("Permute") != std::string::npos) {
            Get_Output_Layer_Name_v5(line);
        }
    }

    return true;
}

bool Subtit_v5(const char* s1, const char* s2, const char* pathname) {
    std::ifstream fileline(pathname);
    if (!fileline.is_open()) {
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(fileline)), std::istreambuf_iterator<char>());
    fileline.close();

    size_t pos = 0;
    while ((pos = content.find(s1, pos)) != std::string::npos) {
        content.replace(pos, strlen(s1), s2);
        pos += strlen(s2);
    }

    std::ofstream outfile(pathname);
    outfile << content;
    outfile.close();

    return true;
}

bool Modify_Model_Reshape_v5(const char* model_path) {
    return Subtit_v5("0=6400", "0=-1", model_path) &&
        Subtit_v5("0=1600", "0=-1", model_path) &&
        Subtit_v5("0=400", "0=-1", model_path);
}

bool Init_Param(const char* param_path, const char* bin_path) {
    return Modify_Model_Reshape_v5(param_path) && Find_Output_Layer_v5(param_path);
}




void YoloV5Detector::init(const char* param_path, const char* bin_path, const int numThreads) {

    num_threads = numThreads;
    yolov5.opt.num_threads = num_threads;
    yolov5.opt.use_vulkan_compute = true;
    Init_Param(param_path, bin_path);




    yolov5.load_param(param_path);
    yolov5.load_model(bin_path);
}

int YoloV5Detector::detect(const cv::Mat& img, std::vector<Object>& result, int imgsize, float conf, float iou) {
    img_w = img.cols;
    img_h = img.rows;

    ncnn::Mat input = ncnn::Mat::from_pixels(img.data, ncnn::Mat::PIXEL_BGR2RGB, img_w, img_h);
    const float norm_vals[3] = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
    input.substract_mean_normalize(0, norm_vals);
    result.clear();
    ncnn::Extractor ex = yolov5.create_extractor();
    ex.input("images", input);
    std::vector<Object> objects8;
    std::vector<Object> objects16;
    std::vector<Object> objects32;
    {
        ncnn::Mat anchors8(6);
        anchors8[0] = 10.f;
        anchors8[1] = 13.f;
        anchors8[2] = 16.f;
        anchors8[3] = 30.f;
        anchors8[4] = 33.f;
        anchors8[5] = 23.f;
        ncnn::Mat anchors16(6);
        anchors16[0] = 30.f;
        anchors16[1] = 61.f;
        anchors16[2] = 62.f;
        anchors16[3] = 45.f;
        anchors16[4] = 59.f;
        anchors16[5] = 119.f;
        ncnn::Mat anchors32(6);
        anchors32[0] = 116.f;
        anchors32[1] = 90.f;
        anchors32[2] = 156.f;
        anchors32[3] = 198.f;
        anchors32[4] = 373.f;
        anchors32[5] = 326.f;

        ncnn::Mat out;
        ex.extract(feature_layer_1.c_str(), out);
        generate_proposals_yolov5(anchors8, 8, input, out, conf, objects8);
        result.insert(result.end(), objects8.begin(), objects8.end());

        ex.extract(feature_layer_2.c_str(), out);
        generate_proposals_yolov5(anchors16, 16, input, out, conf, objects16);
        result.insert(result.end(), objects16.begin(), objects16.end());

        ex.extract(feature_layer_3.c_str(), out);
        generate_proposals_yolov5(anchors32, 32, input, out, conf, objects32);
        result.insert(result.end(), objects32.begin(), objects32.end());
    }




    qsort_descent_inplace(result);
    std::vector<int> picked;
    nms_sorted_bboxes(result, picked, iou);
    const int count = picked.size();
    result.resize(count);
    for (int i = 0; i < count; i++) {
        result[i] = result[picked[i]];
        const float x0 = (result[i].rect.x - (w_pad * 0.5f)) / scale;
        const float y0 = (result[i].rect.y - (h_pad * 0.5f)) / scale;
        const float x1 = (result[i].rect.x + result[i].rect.width - (w_pad * 0.5f)) / scale;
        const float y1 = (result[i].rect.y + result[i].rect.height - (h_pad * 0.5f)) / scale;
        result[i].rect.x = std::max(std::min(x0, static_cast<float>(img_w - 1)), 0.f);
        result[i].rect.y = std::max(std::min(y0, static_cast<float>(img_h - 1)), 0.f);
        result[i].rect.width = std::max(std::min(x1, static_cast<float>(img_w - 1)), 0.f) - result[i].rect.x;
        result[i].rect.height = std::max(std::min(y1, static_cast<float>(img_h - 1)), 0.f) - result[i].rect.y;
    }
    return 0;

}

inline float YoloV5Detector::intersection_area(const Object& a, const Object& b) noexcept {
    cv::Rect_<float> inter = a.rect & b.rect;
    return inter.area();
}

void YoloV5Detector::qsort_descent_inplace(std::vector<Object>& objects, int left, int right) {
    int i = left;
    int j = right;
    float p = objects[(left + right) / 2].prob;

    while (i <= j) {
        while (objects[i].prob > p)
            i++;

        while (objects[j].prob < p)
            j--;

        if (i <= j) {
            std::swap(objects[i], objects[j]);
            i++;
            j--;
        }
    }

#pragma omp parallel sections
    {
#pragma omp section
        {
            if (left < j)
                qsort_descent_inplace(objects, left, j);
        }
#pragma omp section
        {
            if (i < right)
                qsort_descent_inplace(objects, i, right);
        }
    }
}

void YoloV5Detector::qsort_descent_inplace(std::vector<Object>& objects) {
    if (objects.empty())
        return;
    qsort_descent_inplace(objects, 0, objects.size() - 1);
}

void YoloV5Detector::nms_sorted_bboxes(const std::vector<Object>& objects, std::vector<int>& picked, float nms_threshold, bool agnostic) {
    picked.clear();

    const int n = objects.size();
    std::vector<float> areas(n);
    for (int i = 0; i < n; i++) {
        areas[i] = objects[i].rect.area();
    }

    for (int i = 0; i < n; i++) {
        const Object& a = objects[i];

        int keep = 1;
        for (int j = 0; j < static_cast<int>(picked.size()); j++) {
            const Object& b = objects[picked[j]];

            if (!agnostic && a.label != b.label)
                continue;

            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            if (inter_area / union_area > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}

inline float YoloV5Detector::sigmoid_yolov5(float x) {
    return 1.f / (1.f + std::exp(-x));
}

void YoloV5Detector::generate_proposals_yolov5(const ncnn::Mat& anchors, int stride, const ncnn::Mat& in_pad, const ncnn::Mat& feat_blob, float prob_threshold, std::vector<Object>& proposals) {
    const int num_grid = feat_blob.h;

    int num_grid_x;
    int num_grid_y;

    // File 1: ¶¯Ì¬¼ÆËãÍø¸ñ
    if (in_pad.w > in_pad.h) {
        num_grid_x = in_pad.w / stride;
        num_grid_y = num_grid / num_grid_x;
    }
    else {
        num_grid_y = in_pad.h / stride;
        num_grid_x = num_grid / num_grid_y;
    }


    const int num_class = feat_blob.w - 5;
    const int num_anchors = anchors.w / 2;

    for (int q = 0; q < num_anchors; q++) {
        const float anchor_w = anchors[q * 2];
        const float anchor_h = anchors[q * 2 + 1];

        const ncnn::Mat feat = feat_blob.channel(q);

        for (int i = 0; i < num_grid_y; i++) {
            for (int j = 0; j < num_grid_x; j++) {
                const float* featptr = feat.row(i * num_grid_x + j);
                float box_confidence = sigmoid_yolov5(featptr[4]);
                if (box_confidence >= prob_threshold) {
                    int class_index = 0;
                    float class_score = -FLT_MAX;
                    for (int k = 0; k < num_class; k++) {
                        float score = featptr[5 + k];
                        if (score > class_score) {
                            class_index = k;
                            class_score = score;
                        }
                    }
                    float confidence = box_confidence * sigmoid_yolov5(class_score);
                    if (confidence >= prob_threshold) {
                        float dx = sigmoid_yolov5(featptr[0]);
                        float dy = sigmoid_yolov5(featptr[1]);
                        float dw = sigmoid_yolov5(featptr[2]);
                        float dh = sigmoid_yolov5(featptr[3]);

                        float pb_cx = (dx * 2.f - 0.5f + j) * stride;
                        float pb_cy = (dy * 2.f - 0.5f + i) * stride;
                      
                        float pb_w = std::pow(dw * 2.f, 2) * anchor_w;
                        float pb_h = std::pow(dh * 2.f, 2) * anchor_h;

                        float x0 = pb_cx - pb_w * 0.5f;
                        float y0 = pb_cy - pb_h * 0.5f;
                        float x1 = pb_cx + pb_w * 0.5f;
                        float y1 = pb_cy + pb_h * 0.5f;

                        Object obj;
                        obj.rect.x = x0;
                        obj.rect.y = y0;
                        obj.rect.width = x1 - x0;
                        obj.rect.height = y1 - y0;
                        obj.label = class_index;
                        obj.prob = confidence;

                        proposals.push_back(obj);
                    }
                }
            }
        }
    }
}