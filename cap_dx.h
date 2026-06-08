#ifndef CAP_DX_H
#define CAP_DX_H

#include <vector>
#include <map>
#include <tuple>
#include <string>
#include <opencv2/opencv.hpp>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <thread>
#include <chrono>

// 设备类，封装DXGI适配器和D3D11设备
class Device {
public:
    Device(IDXGIAdapter1* adapter);
    ~Device();

    std::vector<IDXGIOutput1*> enum_outputs();
    std::wstring description() const;
    SIZE_T vram_size() const;
    UINT vendor_id() const;
    ID3D11Device* get_device() const;
    ID3D11DeviceContext* get_context() const;
    ID3D11DeviceContext* get_immediate_context() const;

private:
    IDXGIAdapter1* adapter;
    DXGI_ADAPTER_DESC1 desc;
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11DeviceContext* im_context;
};

// 输出类，表示一个显示器
class Output {
public:
    Output(IDXGIOutput1* output);
    ~Output();

    void update_desc();
    HMONITOR hmonitor() const;
    std::string devicename() const;
    std::tuple<int, int> resolution() const;
    std::tuple<int, int> surface_size() const;
    bool attached_to_desktop() const;
    int rotation_angle() const;
    IDXGIOutput1* get_output() const;

private:
    IDXGIOutput1* output;
    DXGI_OUTPUT_DESC desc;
    std::map<DXGI_MODE_ROTATION, int> rotation_mapping;
    int rotation_angle_;
};

// 屏幕复制器类，负责获取屏幕帧
class Duplicator {
public:
    Duplicator(Output* output, Device* device);
    ~Duplicator();

    bool update_frame();
    void release_frame();
    void release();
    bool is_updated() const;
    ID3D11Texture2D* get_texture() const;

private:
    Output* output;
    Device* device;
    bool updated;
    ID3D11Texture2D* texture;
    IDXGIOutputDuplication* duplicator;
};

// 阶段纹理类，用于GPU到CPU的数据传输
class StageSurface {
public:
    StageSurface(Output* output, Device* device, int capture_width, int capture_height);
    ~StageSurface();

    void release();
    void rebuild(Output* output, Device* device, int capture_width, int capture_height);
    D3D11_MAPPED_SUBRESOURCE map();
    void unmap();
    ID3D11Texture2D* get_texture() const;

private:
    Output* output;
    Device* device;
    int width;
    int height;
    DXGI_FORMAT dxgi_format;
    ID3D11Texture2D* texture;
};

// 图像处理类，处理并转换捕获的图像数据
class NumpyProcessor {
public:
    cv::Mat process(D3D11_MAPPED_SUBRESOURCE& mappedResource, std::tuple<int, int, int, int> region, int rotation_angle);
};

// 枚举DXGI适配器
std::vector<IDXGIAdapter1*> enum_dxgi_adapters();

// DXCamera类，负责捕获屏幕帧
class DXCamera {
public:
    DXCamera(Output* output, Device* device, int imgsize);
    ~DXCamera();

    cv::Mat capture();

private:
    void _on_output_change();
    void calculateRegion();
    void validateRegion(std::tuple<int, int, int, int> region);

    Output* _output;
    Device* _device;
    int _imgsize;
    StageSurface* _stagesurf;
    Duplicator* _duplicator;
    NumpyProcessor* _processor;
    D3D11_BOX* _sourceRegion;
    std::tuple<int, int, int, int> region;
    int width;
    int height;
    int rotation_angle;
    cv::Mat frame;
};

// DXScreenCapture类，使用DXCamera进行屏幕捕获
class DXScreenCapture {
public:

    DXScreenCapture();
    ~DXScreenCapture();

    void init(int capture_width, int capture_height);
    cv::Mat capture();

private:
    DXCamera* camera;
};

#endif // CAP_DX_H
