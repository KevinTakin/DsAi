#include "cap_dx.h"
#include <iostream>
#pragma comment(lib, "Dxgi.lib")
#pragma comment(lib, "D3D11.lib")
// Device类实现
Device::Device(IDXGIAdapter1* adapter) : adapter(adapter), device(nullptr), context(nullptr), im_context(nullptr) {
    if (!adapter) {
        throw std::invalid_argument("Adapter cannot be null");
    }

    HRESULT hr = adapter->GetDesc1(&desc);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get adapter description");
    }

    // 支持多个特性级别以确保兼容性
    constexpr D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    // 创建D3D11设备
    hr = D3D11CreateDevice(
        adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, // 启用BGRA支持以提高与Direct2D的兼容性
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &device,
        nullptr,
        &context
    );

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create D3D11 device");
    }

    // 获取立即上下文
    device->GetImmediateContext(&im_context);
    if (!im_context) {
        device->Release();
        device = nullptr;
        context = nullptr;
        throw std::runtime_error("Failed to get immediate context");
    }
}

Device::~Device() {
    if (im_context) {
        im_context->Release();
        im_context = nullptr;
    }
    if (context) {
        context->Release();
        context = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
    // 注意：IDXGIAdapter1* adapter 不在此释放，因为它由外部管理
}

std::vector<IDXGIOutput1*> Device::enum_outputs() {
    std::vector<IDXGIOutput1*> outputs;
    IDXGIOutput* output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
        if (output) {
            IDXGIOutput1* output1 = nullptr;
            HRESULT hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&output1));
            if (SUCCEEDED(hr) && output1) {
                outputs.push_back(output1);
            }
            output->Release();
        }
    }
    return outputs;
}

std::wstring Device::description() const {
    return desc.Description;
}

SIZE_T Device::vram_size() const {
    return desc.DedicatedVideoMemory;
}

UINT Device::vendor_id() const {
    return desc.VendorId;
}

ID3D11Device* Device::get_device() const {
    return device;
}

ID3D11DeviceContext* Device::get_context() const {
    return context;
}

ID3D11DeviceContext* Device::get_immediate_context() const {
    return im_context;
}

// Output类实现
Output::Output(IDXGIOutput1* output) : output(output), rotation_angle_(0) {
    if (!output) {
        throw std::invalid_argument("Output cannot be null");
    }

    // 初始化旋转映射
    rotation_mapping[DXGI_MODE_ROTATION_IDENTITY] = 0;
    rotation_mapping[DXGI_MODE_ROTATION_ROTATE90] = 90;
    rotation_mapping[DXGI_MODE_ROTATION_ROTATE180] = 180;
    rotation_mapping[DXGI_MODE_ROTATION_ROTATE270] = 270;
    update_desc();
}

Output::~Output() {
    if (output) {
        output->Release();
        output = nullptr;
    }
}

void Output::update_desc() {
    if (output) {
        HRESULT hr = output->GetDesc(&desc);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to get output description");
        }
        // 获取旋转角度
        auto it = rotation_mapping.find(desc.Rotation);
        if (it != rotation_mapping.end()) {
            rotation_angle_ = it->second;
        }
        else {
            rotation_angle_ = 0; // 默认无旋转
        }
    }
}

HMONITOR Output::hmonitor() const {
    return desc.Monitor;
}

std::string Output::devicename() const {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::tuple<int, int> Output::resolution() const {
    return {
        desc.DesktopCoordinates.right - desc.DesktopCoordinates.left,
        desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top
    };
}

std::tuple<int, int> Output::surface_size() const {
    const auto [width, height] = resolution();
    if (rotation_angle_ == 90 || rotation_angle_ == 270) {
        return { height, width };
    }
    return { width, height };
}

bool Output::attached_to_desktop() const {
    return desc.AttachedToDesktop;
}

int Output::rotation_angle() const {
    return rotation_angle_;
}

IDXGIOutput1* Output::get_output() const {
    return output;
}

// Duplicator类实现
Duplicator::Duplicator(Output* output, Device* device)
    : output(output), device(device), updated(false), texture(nullptr), duplicator(nullptr)
{
    if (!output || !device) {
        throw std::invalid_argument("Output and Device cannot be null");
    }

    // 初始化屏幕复制器
    HRESULT hr = output->get_output()->DuplicateOutput(device->get_device(), &duplicator);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to duplicate output");
    }
}

Duplicator::~Duplicator() {
    release();
}

bool Duplicator::update_frame() {
    while (true) {
        DXGI_OUTDUPL_FRAME_INFO info;
        IDXGIResource* res = nullptr;
        HRESULT hr = duplicator->AcquireNextFrame(0, &info, &res);

        switch (hr) {
        case S_OK:
            break;
        case DXGI_ERROR_ACCESS_LOST:
        case DXGI_ERROR_DEVICE_REMOVED:
        case ERROR_ABANDONED_WAIT_0:
            release();
            return false;
        case DXGI_ERROR_WAIT_TIMEOUT:
            updated = false;
            return true;
        }

        // 处理有效帧
        if (info.AccumulatedFrames > 0 || info.LastPresentTime.QuadPart != 0) {
            hr = res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture);
            res->Release();
            if (SUCCEEDED(hr)) {
                updated = true;
                return true;
            }
        }

        duplicator->ReleaseFrame();
        res->Release();
    }
}

void Duplicator::release_frame() {
    if (duplicator) {
        duplicator->ReleaseFrame();
    }
}

void Duplicator::release() {
    if (duplicator != nullptr) {
        duplicator->Release();
        duplicator = nullptr;
    }
    if (texture != nullptr) {
        texture->Release();
        texture = nullptr;
    }
}

bool Duplicator::is_updated() const {
    return updated;
}

ID3D11Texture2D* Duplicator::get_texture() const {
    return texture;
}

// StageSurface类实现
StageSurface::StageSurface(Output* output, Device* device, int capture_width, int capture_height)
    : output(output), device(device), width(0), height(0), dxgi_format(DXGI_FORMAT_B8G8R8A8_UNORM), texture(nullptr)
{
    rebuild(output, device, capture_width, capture_height);
}

StageSurface::~StageSurface() {
    release();
}

void StageSurface::release() {
    if (texture != nullptr) {
        texture->Release();
        texture = nullptr;
        width = 0;
        height = 0;
    }
}

void StageSurface::rebuild(Output* output, Device* device, int capture_width, int capture_height) {
    if (!output || !device) {
        throw std::invalid_argument("Output and Device cannot be null");
    }

    // 使用指定的捕获宽度和高度
    width = capture_width;
    height = capture_height;

    if (texture == nullptr) {
        // 创建阶段纹理，用于CPU读取
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Format = dxgi_format;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        desc.BindFlags = 0;

        HRESULT hr = device->get_device()->CreateTexture2D(&desc, nullptr, &texture);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create staging texture");
        }
    }
}

D3D11_MAPPED_SUBRESOURCE StageSurface::map() {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = device->get_immediate_context()->Map(texture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map staging texture");
    }
    return mappedResource;
}

void StageSurface::unmap() {
    device->get_immediate_context()->Unmap(texture, 0);
}

ID3D11Texture2D* StageSurface::get_texture() const {
    return texture;
}

// NumpyProcessor类实现
cv::Mat NumpyProcessor::process(D3D11_MAPPED_SUBRESOURCE& mappedResource, std::tuple<int, int, int, int> region, int rotation_angle) {
    int width = std::get<2>(region) - std::get<0>(region);
    int height = std::get<3>(region) - std::get<1>(region);
    if (rotation_angle == 90 || rotation_angle == 270) {
        std::swap(width, height);
    }

    cv::Mat image(height, width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);

    // 根据旋转角度进行旋转
    switch (rotation_angle) {
    case 90:
        cv::rotate(image, image, cv::ROTATE_90_CLOCKWISE);
        break;
    case 180:
        cv::rotate(image, image, cv::ROTATE_180);
        break;
    case 270:
        cv::rotate(image, image, cv::ROTATE_90_COUNTERCLOCKWISE);
        break;
    default:
        break;
    }

    // 转换颜色格式
    cv::Mat cvtcolor;
    cv::cvtColor(image, cvtcolor, cv::COLOR_BGRA2BGR);
    return cvtcolor;
}

// 枚举DXGI适配器
std::vector<IDXGIAdapter1*> enum_dxgi_adapters() {
    IDXGIFactory1* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || factory == nullptr) {
        std::cerr << "Failed to create DXGIFactory1. HRESULT=" << hr << std::endl;
        return {};
    }
    std::vector<IDXGIAdapter1*> adapters;
    for (UINT i = 0; ; ++i) {
        IDXGIAdapter1* adapter = nullptr;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (adapter) {
            adapters.push_back(adapter);
        }
    }
    factory->Release();
    return adapters;
}

// DXCamera类实现
DXCamera::DXCamera(Output* output, Device* device, int imgsize)
    : _output(output), _device(device), _imgsize(imgsize), _stagesurf(nullptr), _duplicator(nullptr), _processor(nullptr), _sourceRegion(nullptr), frame()
{
    if (!_output || !_device) {
        throw std::invalid_argument("Output and Device cannot be null");
    }

    _stagesurf = new StageSurface(_output, _device, _imgsize, _imgsize);
    _duplicator = new Duplicator(_output, _device);
    _processor = new NumpyProcessor();

    _sourceRegion = new D3D11_BOX();
    _sourceRegion->front = 0;
    _sourceRegion->back = 1;

    const auto [width, height] = _output->resolution();
    this->width = width;
    this->height = height;

    rotation_angle = _output->rotation_angle();

    calculateRegion();

    frame = cv::Mat();
}

DXCamera::~DXCamera() {
    if (_duplicator) {
        delete _duplicator;
        _duplicator = nullptr;
    }
    if (_stagesurf) {
        delete _stagesurf;
        _stagesurf = nullptr;
    }
    if (_processor) {
        delete _processor;
        _processor = nullptr;
    }
    if (_sourceRegion) {
        delete _sourceRegion;
        _sourceRegion = nullptr;
    }
}


cv::Mat DXCamera::capture() {
    while (true) {
        try {
            if (_duplicator->update_frame()) {
                if (_duplicator->is_updated()) {
                    int _width = std::get<2>(region) - std::get<0>(region);
                    int _height = std::get<3>(region) - std::get<1>(region);

                    if (_stagesurf->get_texture() == nullptr) {
                        _stagesurf->release();
                        _stagesurf->rebuild(_output, _device, _width, _height);
                    }

                    _sourceRegion->left = std::get<0>(region);
                    _sourceRegion->top = std::get<1>(region);
                    _sourceRegion->right = std::get<2>(region);
                    _sourceRegion->bottom = std::get<3>(region);

                    _device->get_immediate_context()->CopySubresourceRegion(
                        _stagesurf->get_texture(), 0, 0, 0, 0,
                        _duplicator->get_texture(), 0, _sourceRegion
                    );
                    _duplicator->release_frame();

                    D3D11_MAPPED_SUBRESOURCE mappedResource = _stagesurf->map();
                    cv::Mat processedFrame = _processor->process(mappedResource, region, rotation_angle);
                    _stagesurf->unmap();

                    if (!processedFrame.empty()) {
                        return processedFrame;
                    }
                    else {
                        // 如果处理后的帧为空，继续循环等待
                        continue;
                    }
                }
                else {
                    // 如果没有更新的帧，等待一段时间再重试，避免忙等待
                    continue;
                }
            }
            else {
                // 如果无法更新帧，处理输出更改并等待
                _on_output_change();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        catch (...) {
            // 处理任何异常，处理输出更改并等待
            _on_output_change();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}


void DXCamera::_on_output_change() {
    // 等待一段时间，确保设备已经完全重置
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    try {
        if (_duplicator) {
            _duplicator->release();
            delete _duplicator;
            _duplicator = nullptr;
        }
        if (_stagesurf) {
            _stagesurf->release();
            delete _stagesurf;
            _stagesurf = nullptr;
        }
    }
    catch (...) {
        // 忽略清理资源时的异常
    }

    _output->update_desc();
    const auto [width, height] = _output->resolution();
    this->width = width;
    this->height = height;

    calculateRegion();
    rotation_angle = _output->rotation_angle();

    while (true) {
        try {
            _stagesurf = new StageSurface(_output, _device, _imgsize, _imgsize);
            _duplicator = new Duplicator(_output, _device);
            break; // 初始化成功，退出循环
        }
        catch (const std::exception&) {
            // 处理创建设备或复制器失败的情况，继续尝试
            if (_duplicator) {
                delete _duplicator;
                _duplicator = nullptr;
            }
            if (_stagesurf) {
                delete _stagesurf;
                _stagesurf = nullptr;
            }
            // 等待一段时间再重试
            std::this_thread::sleep_for(std::chrono::milliseconds(100));


        }
    }
}


void DXCamera::calculateRegion() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int regionWidth = _imgsize;
    int regionHeight = _imgsize;

    // 确保捕获区域不超过屏幕边界
    if (regionWidth > screenWidth) regionWidth = screenWidth;
    if (regionHeight > screenHeight) regionHeight = screenHeight;

    int left = (screenWidth - regionWidth) / 2;
    int top = (screenHeight - regionHeight) / 2;
    int right = left + regionWidth;
    int bottom = top + regionHeight;

    region = { left, top, right, bottom };
    validateRegion(region);
}

void DXCamera::validateRegion(std::tuple<int, int, int, int> region) {
    // 可添加更多验证逻辑，例如确保区域在屏幕内
    this->region = region;
    _sourceRegion->left = std::get<0>(region);
    _sourceRegion->top = std::get<1>(region);
    _sourceRegion->right = std::get<2>(region);
    _sourceRegion->bottom = std::get<3>(region);
}


DXScreenCapture::DXScreenCapture()
    : camera(nullptr)
{
}

// DXScreenCapture类实现
void DXScreenCapture::init(int capture_width, int capture_height)
{
    std::vector<IDXGIAdapter1*> p_adapters = enum_dxgi_adapters();

    std::vector<Device*> devices;
    std::vector<Output*> outputs;
    for (IDXGIAdapter1* p_adapter : p_adapters) {
        if (p_adapter) {
            try {
                Device* device = new Device(p_adapter);
                std::vector<IDXGIOutput1*> p_outputs_raw = device->enum_outputs();
                for (IDXGIOutput1* p_output : p_outputs_raw) {
                    if (p_output) {
                        Output* output = new Output(p_output);
                        outputs.push_back(output);
                    }
                }
                devices.push_back(device);
            }
            catch (const std::exception&) {
                // 处理创建设备或输出失败的情况
                if (p_adapter) p_adapter->Release();
                continue;
            }
        }
    }

    // 使用第一个设备和第一个输出
    Device* device = devices[0];
    Output* output = outputs[0];
    output->update_desc();

    camera = new DXCamera(output, device, capture_width);

    // 释放不需要的适配器指针
    for (size_t i = 1; i < devices.size(); ++i) {
        delete devices[i];
    }
    for (size_t i = 1; i < outputs.size(); ++i) {
        delete outputs[i];
    }
    // 注意：第一个设备和输出由DXCamera管理，不需要在此处释放
}

DXScreenCapture::~DXScreenCapture()
{
    if (camera) {
        delete camera;
        camera = nullptr;
    }
}

cv::Mat DXScreenCapture::capture() {
    return camera->capture();
}
