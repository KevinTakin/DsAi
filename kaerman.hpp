#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <cstring>
#include <map>
#include <tuple>
#include <vector>
#include <set>
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
// ------------------------------
// 一些常量设定
// ------------------------------
const int GENERATE_SET = 1;    // 设置航迹起始帧数
const int TERMINATE_SET = 5;  // 设置航迹终止帧数
const double IOU_THRESHOLD = 0.5; // IOU匹配阈值
const double MEASUREMENT_TRUST_RATIO =0.3; // 观测可靠时的信任度比例

// ------------------------------
// 工具函数：用于矩形框和状态向量/量测向量之间互相转换的辅助函数
// ------------------------------

// box格式：[x1, y1, x2, y2]，则中心坐标 (cx, cy)=((x1+x2)/2, (y1+y2)/2)，宽高 (w,h) = (x2-x1, y2-y1)
Eigen::VectorXd box2state(const std::vector<int>& box) {
    double center_x = (box[0] + box[2]) / 2.0;
    double center_y = (box[1] + box[3]) / 2.0;
    double w = box[2] - box[0];
    double h = box[3] - box[1];
    Eigen::VectorXd state(6);
    // 状态向量定义为 [cx, cy, w, h, dx, dy]
    state << center_x, center_y, w, h, 0, 0;
    return state;
}

std::vector<float> state2box(const Eigen::VectorXd& state) {
    double center_x = state(0);
    double center_y = state(1);
    double w = state(2);
    double h = state(3);
    std::vector<float> box = {
        static_cast<float>(center_x - w / 2.0),
        static_cast<float>(center_y - h / 2.0),
        static_cast<float>(center_x + w / 2.0),
        static_cast<float>(center_y + h / 2.0)
    };
    return box;
}

// 量测和box之间的转换（量测向量定义为 [cx, cy, w, h]）
Eigen::VectorXd box2meas(const std::vector<int>& box) {
    double center_x = (box[0] + box[2]) / 2.0;
    double center_y = (box[1] + box[3]) / 2.0;
    double w = box[2] - box[0];
    double h = box[3] - box[1];
    Eigen::VectorXd meas(4);
    meas << center_x, center_y, w, h;
    return meas;
}

std::vector<int> mea2box(const Eigen::VectorXd& mea) {
    double center_x = mea(0);
    double center_y = mea(1);
    double w = mea(2);
    double h = mea(3);
    std::vector<int> box = {
        static_cast<int>(center_x - w / 2.0),
        static_cast<int>(center_y - h / 2.0),
        static_cast<int>(center_x + w / 2.0),
        static_cast<int>(center_y + h / 2.0)
    };
    return box;
}

// 将量测映射到状态空间(初始化时使用)
Eigen::VectorXd mea2state(const Eigen::VectorXd& mea) {
    Eigen::VectorXd state(6);
    // 增加速度分量 dx, dy均初始化为0
    state << mea, 0, 0;
    return state;
}

// 将状态映射到量测空间(输出时使用)
Eigen::VectorXd state2mea(const Eigen::VectorXd& state) {
    return state.head(4);
}


// ----------------------------------------------------------------------------------
// 匈牙利算法实现，用于在状态列表和检测列表之间做最优匹配
// ----------------------------------------------------------------------------------
class HungarianAlgorithm {
public:
    HungarianAlgorithm() {}
    ~HungarianAlgorithm() {}

    // 在此函数中执行匈牙利算法
    // costMatrix 为代价矩阵 (Rows x Cols)。
    // assignment 为输出的匹配关系：assignment[i] = j 表示第 i 行被分配到第 j 列
    // 返回值为最小匹配代价
    double Solve(Eigen::MatrixXd& costMatrix, std::vector<int>& assignment) {
        int nRows = costMatrix.rows();
        int nCols = costMatrix.cols();
        int nElements = nRows * nCols;

        // 将 Eigen 矩阵转换为普通数组，便于后续操作
        double* costMat = new double[nElements];
        for (int i = 0; i < nRows; ++i) {
            for (int j = 0; j < nCols; ++j) {
                costMat[i + nRows * j] = costMatrix(i, j);
            }
        }

        // assignmentData 用于存储每一行匹配到的列信息
        int* assignmentData = new int[nRows];
        double cost = 0.0;

        // 下面是算法所需要的辅助结构
        bool* coveredColumns = new bool[nCols];
        bool* coveredRows = new bool[nRows];
        bool* starMatrix = new bool[nElements];       // 标记"星形"零元素
        bool* primeMatrix = new bool[nElements];      // 标记"撇形"零元素
        bool* newStarMatrix = new bool[nElements];    // 用于在调整路径时，临时存储新的星形矩阵

        // 初始化辅助结构
        memset(coveredColumns, 0, nCols * sizeof(bool));
        memset(coveredRows, 0, nRows * sizeof(bool));
        memset(starMatrix, 0, nElements * sizeof(bool));
        memset(primeMatrix, 0, nElements * sizeof(bool));
        memset(newStarMatrix, 0, nElements * sizeof(bool));

        // 步骤1：对每一行减去该行的最小值(行减)
        for (int row = 0; row < nRows; ++row) {
            double minVal = std::numeric_limits<double>::infinity();
            for (int col = 0; col < nCols; ++col) {
                if (costMat[row + nRows * col] < minVal) {
                    minVal = costMat[row + nRows * col];
                }
            }
            for (int col = 0; col < nCols; ++col) {
                costMat[row + nRows * col] -= minVal;
            }
        }

        // 步骤1b：对每一列减去该列的最小值(列减)
        for (int col = 0; col < nCols; ++col) {
            double minVal = std::numeric_limits<double>::infinity();
            for (int row = 0; row < nRows; ++row) {
                if (costMat[row + nRows * col] < minVal) {
                    minVal = costMat[row + nRows * col];
                }
            }
            for (int row = 0; row < nRows; ++row) {
                costMat[row + nRows * col] -= minVal;
            }
        }

        // 步骤2：在每行每列寻找零元素，如当前行和列都未被覆盖，则打上星标并覆盖该行和列
        for (int row = 0; row < nRows; ++row) {
            for (int col = 0; col < nCols; ++col) {
                if (std::fabs(costMat[row + nRows * col]) < 1e-12) {
                    if (!coveredRows[row] && !coveredColumns[col]) {
                        starMatrix[row + nRows * col] = true;
                        coveredRows[row] = true;
                        coveredColumns[col] = true;
                    }
                }
            }
        }

        // 将行和列覆盖标记重置，以便后续步骤使用
        memset(coveredRows, 0, nRows * sizeof(bool));
        memset(coveredColumns, 0, nCols * sizeof(bool));

        // 主循环：不断执行直到完成匹配
        while (true) {
            // 步骤2a：覆盖所有带星标零的列，并检查是否完成
            {
                for (int row = 0; row < nRows; ++row) {
                    for (int col = 0; col < nCols; ++col) {
                        if (starMatrix[row + nRows * col]) {
                            coveredColumns[col] = true;
                        }
                    }
                }
                int coveredCount = 0;
                for (int col = 0; col < nCols; ++col) {
                    if (coveredColumns[col]) {
                        coveredCount++;
                    }
                }
                int minDim = (std::min)(nRows, nCols);
                if (coveredCount >= minDim) {
                    // 匹配完成，构建结果
                    for (int row = 0; row < nRows; ++row) {
                        assignmentData[row] = -1;
                        for (int col = 0; col < nCols; ++col) {
                            if (starMatrix[row + nRows * col]) {
                                assignmentData[row] = col;
                                break;
                            }
                        }
                    }
                    break;
                }
            }

            // 步骤2b：寻找未覆盖的零元素
            bool doneStep2b = false;
            bool needToRepeatStep2b = true;
            while (needToRepeatStep2b) {
                needToRepeatStep2b = false;
                bool foundZero = false;

                for (int col = 0; col < nCols; ++col) {
                    if (!coveredColumns[col]) {
                        for (int row = 0; row < nRows; ++row) {
                            if (!coveredRows[row] &&
                                std::fabs(costMat[row + nRows * col]) < 1e-12) {
                                primeMatrix[row + nRows * col] = true;
                                foundZero = true;

                                // 检查该行是否有星标零
                                int starCol = -1;
                                for (int j = 0; j < nCols; ++j) {
                                    if (starMatrix[row + nRows * j]) {
                                        starCol = j;
                                        break;
                                    }
                                }

                                if (starCol >= 0) {
                                    // 如有星标零，覆盖该行并取消星标列覆盖
                                    coveredRows[row] = true;
                                    coveredColumns[starCol] = false;
                                    needToRepeatStep2b = true;
                                    break;
                                }
                                else {
                                    // 如无星标零，开始增广路径过程
                                    memcpy(newStarMatrix, starMatrix, nElements * sizeof(bool));
                                    newStarMatrix[row + nRows * col] = true;

                                    // 从该撇形零出发，沿列找星标零，再沿行找撇形零
                                    int curRow = row, curCol = col;
                                    while (true) {
                                        // 找星标零
                                        int starRow = -1;
                                        for (int r = 0; r < nRows; r++) {
                                            if (starMatrix[r + nRows * curCol]) {
                                                starRow = r;
                                                break;
                                            }
                                        }
                                        if (starRow < 0) break;
                                        // 取消该星标零
                                        newStarMatrix[starRow + nRows * curCol] = false;

                                        // 找撇形零
                                        int primeCol = -1;
                                        for (int c = 0; c < nCols; c++) {
                                            if (primeMatrix[starRow + nRows * c]) {
                                                primeCol = c;
                                                break;
                                            }
                                        }
                                        // 将撇形零变为星标零
                                        newStarMatrix[starRow + nRows * primeCol] = true;

                                        curCol = primeCol;
                                        curRow = starRow;
                                    }

                                    // 更新星标矩阵
                                    memcpy(starMatrix, newStarMatrix, nElements * sizeof(bool));

                                    // 重置覆盖与撇形标记
                                    memset(coveredRows, 0, nRows * sizeof(bool));
                                    memset(coveredColumns, 0, nCols * sizeof(bool));
                                    memset(primeMatrix, 0, nElements * sizeof(bool));

                                    doneStep2b = true;
                                }

                                if (doneStep2b || needToRepeatStep2b) break;
                            }
                        }
                    }
                    if (doneStep2b || needToRepeatStep2b) break;
                }

                // 如未找到新的可处理零，且未完成增广路径，则调整矩阵
                if (!foundZero && !doneStep2b && !needToRepeatStep2b) {
                    // 找出未覆盖元素中的最小值 h
                    double h = std::numeric_limits<double>::infinity();
                    for (int row = 0; row < nRows; ++row) {
                        if (!coveredRows[row]) {
                            for (int col = 0; col < nCols; ++col) {
                                if (!coveredColumns[col]) {
                                    double val = costMat[row + nRows * col];
                                    if (val < h) {
                                        h = val;
                                    }
                                }
                            }
                        }
                    }

                    // 调整矩阵：对覆盖行加h，对未覆盖列减h
                    for (int row = 0; row < nRows; ++row) {
                        for (int col = 0; col < nCols; ++col) {
                            if (coveredRows[row]) {
                                costMat[row + nRows * col] += h;
                            }
                            if (!coveredColumns[col]) {
                                costMat[row + nRows * col] -= h;
                            }
                        }
                    }

                    needToRepeatStep2b = true;
                }
            }
        }

        // 构建最终结果
        assignment.clear();
        for (int i = 0; i < nRows; ++i) {
            assignment.push_back(assignmentData[i]);
        }

        // 计算最终代价
        cost = 0.0;
        for (int row = 0; row < nRows; ++row) {
            int col = assignmentData[row];
            if (col >= 0) {
                cost += costMatrix(row, col);
            }
        }

        // 释放内存
        delete[] costMat;
        delete[] assignmentData;
        delete[] coveredColumns;
        delete[] coveredRows;
        delete[] starMatrix;
        delete[] primeMatrix;
        delete[] newStarMatrix;

        return cost;
    }
};


// ----------------------------------------------------------------------------------
// 匹配器类：用于计算成本矩阵并调用匈牙利算法进行关联
// ----------------------------------------------------------------------------------
class Matcher {
public:
    Matcher() {}

    // 匹配接口：输入状态列表和量测列表，返回匹配关系（状态索引 -> 量测索引）
    static std::map<int, int> match(std::vector<Eigen::VectorXd>& state_list,
        std::vector<Eigen::VectorXd>& measure_list) {
        int num_states = state_list.size();
        int num_measures = measure_list.size();
        Eigen::MatrixXd cost_matrix = Eigen::MatrixXd::Zero(num_states, num_measures);

        // 使用IOU计算代价：cost = 1 - IOU
        for (int i = 0; i < num_states; ++i) {
            for (int j = 0; j < num_measures; ++j) {
                double score = cal_iou(state_list[i], measure_list[j]);
                cost_matrix(i, j) = 1.0 - score;
            }
        }

        HungarianAlgorithm HungAlgo;
        std::vector<int> assignment;
        HungAlgo.Solve(cost_matrix, assignment);

        // 只接受IOU大于阈值的匹配
        std::map<int, int> res;
        for (int i = 0; i < static_cast<int>(assignment.size()); ++i) {
            if (assignment[i] >= 0 && cost_matrix(i, assignment[i]) < (1.0 - IOU_THRESHOLD)) {
                res[i] = assignment[i];
            }
        }
        return res;
    }

    // 计算IOU的辅助函数
    static double cal_iou(const Eigen::VectorXd& state, const Eigen::VectorXd& measure) {
        // 将状态向量前4维转换为box
        std::vector<int> s_box = mea2box(state.head(4));
        std::vector<int> m_box = mea2box(measure);
        int s_tl_x = s_box[0], s_tl_y = s_box[1], s_br_x = s_box[2], s_br_y = s_box[3];
        int m_tl_x = m_box[0], m_tl_y = m_box[1], m_br_x = m_box[2], m_br_y = m_box[3];
        int x_min = (std::max)(s_tl_x, m_tl_x);
        int y_min = (std::max)(s_tl_y, m_tl_y);
        int x_max = (std::max)(s_br_x, m_br_x);
        int y_max = (std::max)(s_br_y, m_br_y);
        int inter_w = (std::max)(x_max - x_min + 1, 0);
        int inter_h = (std::max)(y_max - y_min + 1, 0);
        int inter = inter_h * inter_w;
        if (inter == 0) {
            return 0.0;
        }
        else {
            int area_s = (s_br_x - s_tl_x + 1) * (s_br_y - s_tl_y + 1);
            int area_m = (m_br_x - m_tl_x + 1) * (m_br_y - m_tl_y + 1);
            double iou = static_cast<double>(inter) / (area_s + area_m - inter);
            return iou;
        }
    }
};

// ----------------------------------------------------------------------------------
// 改进的卡尔曼滤波器类：目标消失后预测框保持在原位
// ----------------------------------------------------------------------------------
class Kalman {
public:
    int id;
    // 状态转移矩阵 A, 控制矩阵 B (未使用), 观测矩阵 H, 过程噪声协方差 Q, 观测噪声协方差 R
    Eigen::MatrixXd A;
    Eigen::MatrixXd B;
    Eigen::MatrixXd H;
    Eigen::MatrixXd Q;
    Eigen::MatrixXd R;

    // 后验状态和协方差
    Eigen::VectorXd X_posterior;
    Eigen::MatrixXd P_posterior;

    // 先验状态和协方差
    Eigen::VectorXd X_prior;
    Eigen::MatrixXd P_prior;

    // 卡尔曼增益
    Eigen::MatrixXd K;

    // 当前观测
    Eigen::VectorXd Z;

    // 终止计数器，当连续若干帧没有匹配时终止
    int terminate_count;

    // 航迹记录（中心点历史）
    std::vector<std::pair<int, int>> track;

    // 航迹颜色（随机生成）
    std::tuple<int, int, int> track_color;

    // 目标所属类别标签
    int label;

    // 目标置信度
    float confidence;

    // 用于新目标确认的计数，新目标需连续获得GENERATE_SET次匹配后方可确认
    int init_frames;
    bool confirmed;  // 确认后的目标才输出

    // 目标是否丢失
    bool is_lost;

    // 最后一次有效观测的状态（新增：用于在目标丢失时保持位置不变）
    Eigen::VectorXd last_valid_state;

    // 构造函数
    Kalman(const Eigen::MatrixXd& A_,
        const Eigen::MatrixXd& B_,
        const Eigen::MatrixXd& H_,
        const Eigen::MatrixXd& Q_,
        const Eigen::MatrixXd& R_,
        const Eigen::VectorXd& X_,
        const Eigen::MatrixXd& P_,
        int obj_label,
        float obj_confidence,
        int obj_id)
        :id(obj_id), A(A_), B(B_), H(H_), Q(Q_), R(R_), X_posterior(X_), P_posterior(P_), label(obj_label), confidence(obj_confidence)
    {
        terminate_count = TERMINATE_SET;
        X_prior = Eigen::VectorXd::Zero(X_.size());
        P_prior = Eigen::MatrixXd::Zero(P_.rows(), P_.cols());
        K = Eigen::MatrixXd::Zero(X_.size(), H_.rows());
        Z = Eigen::VectorXd::Zero(H_.rows());

        // 新建目标初始化确认计数
        init_frames = 1;
        confirmed = false;

        // 目标未丢失
        is_lost = false;

        // 初始化最后有效状态为当前状态
        last_valid_state = X_;

        record_track();
    }

    // 卡尔曼预测
    // 修改：目标丢失时不进行位置预测，而是维持原位置
    std::pair<Eigen::VectorXd, Eigen::MatrixXd> predict() {
        if (!is_lost) {
            // 目标未丢失时，正常预测
            X_prior = A * X_posterior;
            P_prior = A * P_posterior * A.transpose() + Q;
        }
        else {
            // 目标丢失时，保持最后观测位置不变
            X_prior = last_valid_state;
            // 但增加位置不确定性
            P_prior = P_posterior;
            P_prior.diagonal().head(4) += Eigen::VectorXd::Ones(4) * 0.5; // 略微增加不确定性
        }
        return { X_prior, P_prior };
    }

    // 卡尔曼更新
    std::tuple<bool, Eigen::VectorXd, Eigen::MatrixXd, float, int> update(const Eigen::VectorXd* mea = nullptr, float new_confidence = 0.0f) {
        bool status = true;
        if (mea != nullptr) {
            // 有量测时，进行正常的卡尔曼更新
            Z = *mea;
            is_lost = false; // 目标找到

            // 计算卡尔曼增益
            K = P_prior * H.transpose() * (H * P_prior * H.transpose() + R).inverse();

            // 更新状态
            X_posterior = X_prior + K * (Z - H * X_prior);
            P_posterior = (Eigen::MatrixXd::Identity(6, 6) - K * H) * P_prior;

            // 更新最后有效状态
            last_valid_state = X_posterior;

            // 如果观测与预测差异太大，可能是因为相机移动，此时直接采用观测值
            Eigen::VectorXd innovation = Z - H * X_prior;
            double innovation_magnitude = innovation.norm();

            if (innovation_magnitude > 20.0) {  // 差异阈值
                // 直接用观测值更新位置
                X_posterior.head(4) = Z;
                // 速度设为0，相机移动后目标相对速度不可靠
                X_posterior(4) = 0;
                X_posterior(5) = 0;

                // 更新最后有效状态
                last_valid_state = X_posterior;
            }

            confidence = new_confidence;

            // 若目标尚未确认，则累计匹配次数
            if (!confirmed) {
                init_frames++;
                if (init_frames >= GENERATE_SET) {
                    confirmed = true;
                }
            }
            terminate_count = TERMINATE_SET; // 重置终止计数器
        }
        else {
            // 无量测时，目标已丢失
            is_lost = true;

            // 修改：目标丢失时，保持位置不变，速度为0
            X_posterior = last_valid_state;
            // 将速度分量设为0
            X_posterior(4) = 0;
            X_posterior(5) = 0;

            // 终止计数递减
            if (terminate_count <= 0) {
                status = false;
            }
            else {
                terminate_count -= 1;
            }
        }

        // 记录航迹
        if (status) {
            record_track();
        }
        return std::make_tuple(status, X_posterior, P_posterior, confidence, label);

    }

    // 判断是否应该输出预测结果
    // 修改：目标丢失且已确认的情况下输出预测框
    bool shouldOutputPrediction() {
        // 目标已丢失且已被确认，则输出预测框
        return is_lost && confirmed;
    }

private:
    Eigen::VectorXd prev_position_;  // 上一帧位置 (cx, cy)
    bool has_prev_ = false;          // 是否有历史位置
    double stability_threshold_ = 3.0; // 微动判定阈值(像素)
    // 将当前状态中心点记录到轨迹中
    void record_track() {
        track.emplace_back(static_cast<int>(X_posterior(0)),
            static_cast<int>(X_posterior(1)));
    }
};


// ----------------------------------------------------------------------------------
// 多目标追踪管理类：目标丢失后预测框保持在原位
// ----------------------------------------------------------------------------------
class MultiTracker {
public:
    MultiTracker() : next_id_(1)
    {

        // 状态转移矩阵 A

        A_ = Eigen::MatrixXd::Zero(6, 6);
        A_(0, 0) = 1.0;
        A_(0, 4) = 0.0; // 修改：不使用速度进行预测
        A_(1, 1) = 1.0;
        A_(1, 5) = 0.0; // 修改：不使用速度进行预测
        A_(2, 2) = 1.0;
        A_(3, 3) = 1.0;
        A_(4, 4) = 0.0; // 速度递减到0
        A_(5, 5) = 0.0; // 速度递减到0

        // 控制输入矩阵 B
        B_ = Eigen::MatrixXd::Zero(6, 6); // 不考虑外部控制量

        // 状态观测矩阵 H
        H_ = Eigen::MatrixXd::Zero(4, 6);
        H_(0, 0) = 1.0;
        H_(1, 1) = 1.0;
        H_(2, 2) = 1.0;
        H_(3, 3) = 1.0;

        // 过程噪声协方差 Q - 更小的噪声使预测更稳定
        Q_ = Eigen::MatrixXd::Identity(6, 6);
        Q_(0, 0) = 0.1; // 位置 x
        Q_(1, 1) = 0.1; // 位置 y
        Q_(2, 2) = 0.1; // 宽度 w
        Q_(3, 3) = 0.1; // 高度 h
        Q_(4, 4) = 0.0; // 速度 dx
        Q_(5, 5) = 0.0; // 速度 dy

        // 观测噪声协方差 R - 较小，更信任观测值
        R_ = Eigen::MatrixXd::Identity(4, 4) * 0.1;
    }

    // 更新每一帧检测结果
    void updateFrame(const std::vector<Object>& objects) {
        // 第一步：预测所有现有Kalman滤波器
        for (auto& kf : kalman_list_) {
            kf.predict();
        }

        // 分类：按照类别将检测和现有Kalman分别归类
        std::map<int, std::vector<int>> detection_indices_by_label;
        std::map<int, std::vector<int>> tracker_indices_by_label;
        std::vector<Eigen::VectorXd> all_measures;
        for (size_t i = 0; i < objects.size(); ++i) {
            int lbl = objects[i].label;
            detection_indices_by_label[lbl].push_back(i);
            // 将对象rect转换为box：[x1,y1,x2,y2]
            std::vector<int> box(4);
            box[0] = static_cast<int>(objects[i].rect.x);
            box[1] = static_cast<int>(objects[i].rect.y);
            box[2] = static_cast<int>(objects[i].rect.x + objects[i].rect.width);
            box[3] = static_cast<int>(objects[i].rect.y + objects[i].rect.height);
            Eigen::VectorXd meas = box2meas(box); // 得到 [cx,cy,w,h]
            all_measures.push_back(meas);
        }
        for (size_t i = 0; i < kalman_list_.size(); ++i) {
            int lbl = kalman_list_[i].label;
            tracker_indices_by_label[lbl].push_back(i);
        }

        // 标记哪些检测已经被匹配
        std::set<int> used_detection_indices;

        // 针对每个类别分别匹配
        std::set<int> all_labels;
        for (auto& p : detection_indices_by_label) {
            all_labels.insert(p.first);
        }
        for (auto& p : tracker_indices_by_label) {
            all_labels.insert(p.first);
        }

        // 对于每个类别，构造对应的状态和量测列表，然后调用匹配
        for (int lbl : all_labels) {
            std::vector<int> det_indices;
            if (detection_indices_by_label.find(lbl) != detection_indices_by_label.end()) {
                det_indices = detection_indices_by_label[lbl];
            }
            std::vector<int> tracker_indices;
            if (tracker_indices_by_label.find(lbl) != tracker_indices_by_label.end()) {
                tracker_indices = tracker_indices_by_label[lbl];
            }

            // 构造子列表
            std::vector<Eigen::VectorXd> sub_state_list, sub_measure_list;
            std::vector<int> tracker_index_mapping, det_index_mapping;
            for (int idx : tracker_indices) {
                // 使用Kalman预测结果的前4维作为量测比较
                sub_state_list.push_back(kalman_list_[idx].X_prior.head(4));
                tracker_index_mapping.push_back(idx);
            }
            for (int idx : det_indices) {
                sub_measure_list.push_back(all_measures[idx]);
                det_index_mapping.push_back(idx);
            }

            // 进行匹配
            if (!sub_state_list.empty() && !sub_measure_list.empty()) {
                std::map<int, int> match_dict = Matcher::match(sub_state_list, sub_measure_list);

                // 更新匹配到的Kalman
                for (auto& pair : match_dict) {
                    int sub_tracker_idx = pair.first;
                    int sub_det_idx = pair.second;
                    int global_tracker_idx = tracker_index_mapping[sub_tracker_idx];
                    int global_det_idx = det_index_mapping[sub_det_idx];
                    used_detection_indices.insert(global_det_idx);
                    kalman_list_[global_tracker_idx].update(&all_measures[global_det_idx], objects[global_det_idx].prob);
                }

                // 对该类别中未匹配到的Kalman，进行无量测update（保持原位）
                for (size_t i = 0; i < tracker_index_mapping.size(); ++i) {
                    if (match_dict.find(i) == match_dict.end()) {
                        int global_tracker_idx = tracker_index_mapping[i];
                        kalman_list_[global_tracker_idx].update(nullptr);
                    }
                }
            }
            else {
                // 如果没有检测或没有现有目标，则所有已有目标执行无量测更新
                for (int idx : tracker_indices) {
                    kalman_list_[idx].update(nullptr);
                }
            }
        }

        // 针对所有检测中未匹配到的，创建新的Kalman
        for (size_t i = 0; i < objects.size(); ++i) {
            if (used_detection_indices.find(i) == used_detection_indices.end()) {
                Eigen::VectorXd init_state = mea2state(all_measures[i]);  // [cx,cy,w,h,dx,dy]
                Eigen::MatrixXd init_P = Eigen::MatrixXd::Identity(6, 6) * 10.0; // 初始协方差
                Kalman new_kalman(A_, B_, H_, Q_, R_, init_state, init_P, objects[i].label, objects[i].prob, getNewId());
                kalman_list_.push_back(new_kalman);
            }
        }

        // 清除已经终止的Kalman滤波器
        auto it = kalman_list_.begin();
        while (it != kalman_list_.end()) {
            if (it->terminate_count <= 0) {
                free_ids_.push_back(it->id);
                it = kalman_list_.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // 获取当前所有存活且已确认的目标的 Object 信息
    void getTracks(std::vector<Object>& tracks) {
        tracks.clear();
        for (auto& kf : kalman_list_) {
            // 只输出已确认的目标
            if (kf.confirmed) {
                // 无论目标是否丢失都输出框，但使用的是保持不变的位置
                Eigen::VectorXd st = kf.X_posterior;
                std::vector<float> bbox = state2box(st);
                Object obj;
                obj.rect = cv::Rect_<float>(bbox[0], bbox[1], bbox[2] - bbox[0], bbox[3] - bbox[1]);
                obj.label = kf.label;
                obj.prob = kf.is_lost ? (kf.confidence * 0.8) : kf.confidence; // 丢失目标的置信度稍微降低
                obj.track_id = kf.id; // 设置ID
                tracks.push_back(obj);
            }
        }
    }

private:
    int getNewId() {
        if (!free_ids_.empty()) {
            int id = free_ids_.back();
            free_ids_.pop_back();
            return id;
        }
        else {
            return next_id_++;
        }
    }
    int next_id_ = 1; // ID计数器
    std::vector<int> free_ids_;
    std::vector<Kalman> kalman_list_;
    Eigen::MatrixXd A_, B_, H_, Q_, R_;
};