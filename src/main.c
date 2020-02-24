#ifndef DEBUG
#define DEBUG
#endif

#include "pch.h"
#include <stdio.h>
#include "debug.c"
#include "allocator.c"
#include "maths.c"

struct State_Desc {
    struct {
        uint16_t x;
        uint16_t y;
        uint32_t width;
        uint32_t height;
    } window_coords;
    int16_t sensitivity;
    ID3D11RasterizerState *raster_cull_back;
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11InputLayout *input_layout;
    float scroll;
    uint16_t status_flags;
    unsigned char pad1[12];
    //64
    ID3D11Texture2D *depth_stencil_buffer;
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilView *depth_stencil_view;
    ID3D11Device *device;
    ID3D11DeviceContext *device_context;
    IDXGISwapChain *swap_chain;
    DXGI_MODE_DESC *dxgi_modes;
    uint32_t num_dxgi_modes;
    uint32_t cur_dxgi_mode_idx;
    //128
    ID3D11Buffer *vertex_buf;
    ID3D11Buffer *vertex_index_buf;
    ID3D11Buffer *constants_buf;
    uint32_t vertex_strides;
    uint32_t vertex_offsets;
    unsigned char pad3[32];
};

struct Render_desc {
    float3 camera_pos;
    float3 camera_lookat;
    float3 camera_u;               //
    float3 camera_v;               // basis vectors
    float3 camera_w;               //
    float camera_speed;

    float3 scale_vector;
    float wvp_matrix[4][4];
};

static unsigned long read_small_file(unsigned char *const file_buf, const wchar_t *const filename)
{
    const HANDLE file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ASSERT(file != INVALID_HANDLE_VALUE);
    LARGE_INTEGER file_size;
    ASSERT(GetFileSizeEx(file, &file_size));

    //256 Kb max
    ASSERT(file_size.QuadPart <= 0x40000);

    unsigned long bytes_read;
    ASSERT(ReadFile(file, file_buf, (uint32_t)file_size.QuadPart, &bytes_read, NULL));
    ASSERT(CloseHandle(file));
    return bytes_read;
}


static void window_resize_buffers(struct State_Desc *const restrict state_desc, const uint32_t width, const uint32_t height)
{
    ID3D11RenderTargetView *const render_target_view   = state_desc->render_target_view;
    ID3D11DepthStencilView *const depth_stencil_view   = state_desc->depth_stencil_view;
    ID3D11Texture2D        *const depth_stencil_buffer = state_desc->depth_stencil_buffer;
    IDXGISwapChain         *const swap_chain           = state_desc->swap_chain;
    ID3D11Device           *const device               = state_desc->device;
    ID3D11DeviceContext    *const device_context       = state_desc->device_context;
    
    ID3D11RenderTargetView_Release(render_target_view);
    ID3D11DepthStencilView_Release(depth_stencil_view);
    ID3D11Texture2D_Release(depth_stencil_buffer);
    ASSERT_POSIX(IDXGISwapChain_ResizeBuffers(swap_chain, 0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    ID3D11RenderTargetView *new_render_target_view;
    {
        ID3D11Texture2D* back_buffer;
        ASSERT_POSIX(IDXGISwapChain_GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void**)&back_buffer));

        {
            ID3D11Resource* res;
            ASSERT_POSIX(ID3D11Texture2D_QueryInterface(back_buffer, &IID_ID3D11Resource, (void**)&res));
            ASSERT_POSIX(ID3D11Device_CreateRenderTargetView(device, res, 0, &new_render_target_view));
            ID3D11Resource_Release(res);
        }

        ID3D11Texture2D_Release(back_buffer);
    }

    const D3D11_TEXTURE2D_DESC depth_stencil_desc = {
        .Width              = width,
        .Height             = height,
        .MipLevels          = 1,
        .ArraySize          = 1,
        .Format             = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc.Count   = 1,
        .SampleDesc.Quality = 0,
        .Usage              = D3D11_USAGE_DEFAULT,
        .BindFlags          = D3D11_BIND_DEPTH_STENCIL,
        .CPUAccessFlags     = 0,
        .MiscFlags          = 0
    };

    ID3D11DepthStencilView *new_depth_stencil_view;
    ID3D11Texture2D        *new_depth_stencil_buffer;
    ASSERT_POSIX(ID3D11Device_CreateTexture2D(device, &depth_stencil_desc, 0, &new_depth_stencil_buffer));

    {
        ID3D11Resource* res;
        ASSERT_POSIX(ID3D11Texture2D_QueryInterface(new_depth_stencil_buffer, &IID_ID3D11Resource, (void**)&res));
        ASSERT_POSIX(ID3D11Device_CreateDepthStencilView(device, res, 0, &new_depth_stencil_view));
        ID3D11Resource_Release(res);
    }

    const D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.f,
        .TopLeftY = 0.f,
        .Width    = width,
        .Height   = height,
        .MinDepth = 0.f,
        .MaxDepth = 1.f
    };

    ID3D11DeviceContext_RSSetViewports(device_context, 1, &viewport);

    state_desc->render_target_view   = new_render_target_view;
    state_desc->depth_stencil_view   = new_depth_stencil_view;
    state_desc->depth_stencil_buffer = new_depth_stencil_buffer;
}

static LRESULT CALLBACK wnd_proc(HWND main_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct State_Desc *const restrict state_desc = ALLOCATOR_CONSTANTS.STATE_SPACE;
    
    switch(msg) {
        case WM_CLOSE: {
            if(MessageBox(main_hwnd, TEXT("Are you sure you want to quit?"), TEXT("Quit?"), MB_YESNO | MB_ICONQUESTION) == IDYES)
                DestroyWindow(main_hwnd);
        }
        break;
        case WM_SIZE: {
            if(wParam == SIZE_MINIMIZED) {
                state_desc->status_flags |= 1;
                break;
            }
            #ifdef DEBUG
            printf("On resize\n\n");
            #endif

            const uint16_t width = LOWORD(lParam);
            const uint16_t height = HIWORD(lParam);
            state_desc->window_coords.width = width;
            state_desc->window_coords.height = height;

            window_resize_buffers(state_desc, width, height);
        }
        break;
        case WM_MOVE: {
            state_desc->window_coords.x = LOWORD(lParam);
            state_desc->window_coords.y = HIWORD(lParam);
        }
        break;
        case WM_ENTERSIZEMOVE: {
            state_desc->status_flags |= 1;
        }
        break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        }
        break;
        case WM_ACTIVATE: {
            if((wParam & 0xFF) == WA_INACTIVE)
                state_desc->status_flags |= 1;
        }
        break;
        case WM_KEYDOWN: {
            const uint8_t key = wParam & 0xFF;
            switch(key) {
                //'ESC' key
                case VK_ESCAPE: {
                    state_desc->status_flags ^= 1;
                }
                break;
                //',' key
                case VK_OEM_COMMA: {
                    IDXGISwapChain *const swap_chain = state_desc->swap_chain;
                    DXGI_MODE_DESC *const restrict dxgi_modes = state_desc->dxgi_modes;
                    const uint32_t mode_idx = state_desc->cur_dxgi_mode_idx;
                    const uint32_t idx      = ~(uint32_t)((int32_t)(mode_idx - 1) >> 31) & (mode_idx - 1);
                    const uint32_t width    = dxgi_modes[idx].Width;
                    const uint32_t height   = dxgi_modes[idx].Height;

                    ASSERT_POSIX(IDXGISwapChain_ResizeTarget(swap_chain, &dxgi_modes[idx]));
                    window_resize_buffers(state_desc, width, height);
                    state_desc->cur_dxgi_mode_idx = idx;
                    #ifdef DEBUG
                    printf("ID: %u\nWidth: %u\nHeight: %u\nRefreshRate: %u\\%u\nFORMAT: %d\nScanlineOrdering: %d\nScaling: %d\n\n",
                        idx,
                        state_desc->dxgi_modes[idx].Width, state_desc->dxgi_modes[idx].Height,
                        state_desc->dxgi_modes[idx].RefreshRate.Numerator, state_desc->dxgi_modes[idx].RefreshRate.Denominator,
                        state_desc->dxgi_modes[idx].Format, state_desc->dxgi_modes[idx].ScanlineOrdering, state_desc->dxgi_modes[idx].Scaling);
                    #endif
                }
                break;
                //'.' key
                case VK_OEM_PERIOD: {
                    IDXGISwapChain *const swap_chain = state_desc->swap_chain;
                    DXGI_MODE_DESC *const restrict dxgi_modes = state_desc->dxgi_modes;
                    const uint32_t mode_idx     = state_desc->cur_dxgi_mode_idx;
                    const uint32_t max_mode_idx = state_desc->num_dxgi_modes - 1;
                    const uint32_t pred         = (uint32_t)((int32_t)(max_mode_idx - 1 - mode_idx) >> 31);
                    const uint32_t idx          = (~pred & (mode_idx + 1)) | (pred & mode_idx);
                    const uint32_t width        = dxgi_modes[idx].Width;
                    const uint32_t height       = dxgi_modes[idx].Height;

                    ASSERT_POSIX(IDXGISwapChain_ResizeTarget(swap_chain, &dxgi_modes[idx]));
                    window_resize_buffers(state_desc, width, height);
                    state_desc->cur_dxgi_mode_idx = idx;
                    #ifdef DEBUG
                    printf("ID: %u\nWidth: %u\nHeight: %u\nRefreshRate: %u\\%u\nFORMAT: %d\nScanlineOrdering: %d\nScaling: %d\n\n",
                        idx,
                        state_desc->dxgi_modes[idx].Width, state_desc->dxgi_modes[idx].Height,
                        state_desc->dxgi_modes[idx].RefreshRate.Numerator, state_desc->dxgi_modes[idx].RefreshRate.Denominator,
                        state_desc->dxgi_modes[idx].Format, state_desc->dxgi_modes[idx].ScanlineOrdering, state_desc->dxgi_modes[idx].Scaling);
                    #endif
                }
                break;
            }
        }
        break;
        case WM_SYSKEYDOWN: {
            if(wParam == VK_RETURN) {
                ASSERT_POSIX(IDXGISwapChain_SetFullscreenState(state_desc->swap_chain, state_desc->status_flags >> 1, NULL));
                RECT rt;
                GetClientRect(main_hwnd, &rt);
                window_resize_buffers(state_desc, (uint32_t)(rt.right - rt.left), (uint32_t)(rt.bottom - rt.top));
                state_desc->status_flags ^= 2;
            }
        }
        break;
        case WM_SETCURSOR: {
            SetCursor((HCURSOR)((uintptr_t)(((intptr_t)state_desc->status_flags << 63) >> 63) & (uintptr_t)(LoadCursor(NULL, IDC_ARROW))));
        }
        break;
        default: {
            return DefWindowProc(main_hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

static LRESULT CALLBACK wheel_proc(_In_ const int nCode, _In_ const WPARAM wParam, _In_ const LPARAM lParam)
{
    if(nCode < 0)
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    if(wParam == WM_MOUSEWHEEL) {
        MOUSEHOOKSTRUCTEX *const restrict mouse_info = (MOUSEHOOKSTRUCTEX *)lParam;
        struct State_Desc *const restrict state_desc = ALLOCATOR_CONSTANTS.STATE_SPACE;
        const int16_t state_scroll = state_desc->scroll;
        const int16_t val = -(int16_t)HIWORD(mouse_info->mouseData);
        const int16_t scroll = state_scroll + val;
        const int16_t scroll_f = state_scroll + 
            (int16_t)(((uint16_t)~((int16_t)(scroll - 1) >> 15) & (uint16_t)((int16_t)(scroll - 2000) >> 15)) & val);
        state_desc->scroll = scroll_f;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int WINAPI wWinMain(_In_ const HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ const int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    const struct Allocator allocator = allocator_init();

    {
        const WNDCLASSEX main_window = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = wnd_proc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = hInstance,
            .hIcon = LoadIcon(NULL, IDI_APPLICATION),
            .hCursor = NULL,
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
            .lpszMenuName = NULL,
            .lpszClassName = TEXT("main_window"),
            .hIconSm = LoadIcon(NULL, IDI_APPLICATION)
        };
        ASSERT(RegisterClassEx(&main_window));
    }

    const HWND main_hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        TEXT("main_window"),
        TEXT("game"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        650,
        512,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    #ifdef DEBUG
        if(!AllocConsole()) {
            if(FreeConsole()) {
                ASSERT(AllocConsole());
            }
        }
        ASSERT(SetConsoleTitle(TEXT("Debug Output")));
        ASSERT(freopen("CONIN$", "r", stdin));
        ASSERT(freopen("CONOUT$", "w", stdout));
        //ASSERT(freopen("log.txt", "w", stdout));
    #endif

    struct State_Desc *const restrict state_desc = page_alloc(ALLOCATOR_CONSTANTS.STATE_SPACE, sizeof(struct State_Desc)
        + sizeof(struct Render_desc), 1);
    struct Render_desc *const restrict render_desc = (void *)((unsigned char *)state_desc + sizeof(struct State_Desc));

    render_desc->camera_pos.a = 0.f;
    render_desc->camera_pos.b = 0.f;
    render_desc->camera_pos.c = 0.f;
    render_desc->camera_lookat.a = 0.f;
    render_desc->camera_lookat.b = 0.f;
    render_desc->camera_lookat.c = 1.f;

    render_desc->scale_vector.a = 10.f;
    render_desc->scale_vector.b = 10.f;
    render_desc->scale_vector.c = 10.f;

    {
        ID3D11Device *device;
        ID3D11DeviceContext *device_context;
        IDXGISwapChain *swap_chain;

        {
            const DXGI_SWAP_CHAIN_DESC sd = {
                .BufferDesc.Width = 0,
                .BufferDesc.Height = 0,
                .BufferDesc.RefreshRate.Numerator = 0,
                .BufferDesc.RefreshRate.Denominator = 0,
                .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
                .SampleDesc.Count = 1,
                .SampleDesc.Quality = 0,
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = 2,
                .OutputWindow = main_hwnd,
                .Windowed = TRUE,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
                .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
            };
            const D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1};
            #ifdef DEBUG
            const uint32_t createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
            #else
            const uint32_t createDeviceFlags = 0;
            #endif

            ASSERT_POSIX(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                createDeviceFlags, feature_levels, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &device, NULL, &device_context));
        }

        {
            IDXGIDevice *dxgi_device;
            IDXGIAdapter *dxgi_adapter;
            IDXGIFactory1 *dxgi_factory;
            ASSERT_POSIX(ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device));
            ASSERT_POSIX(IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter));
            ASSERT_POSIX(IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory1, (void **)&dxgi_factory));
            ASSERT_POSIX(IDXGIFactory1_MakeWindowAssociation(dxgi_factory, main_hwnd, DXGI_MWA_NO_ALT_ENTER));
            IDXGIDevice_Release(dxgi_device);
            IDXGIAdapter_Release(dxgi_adapter);
            IDXGIFactory1_Release(dxgi_factory);
        }

        ID3D11RenderTargetView *render_target_view;

        {
            ID3D11Texture2D *back_buffer;
            ASSERT_POSIX(IDXGISwapChain_GetBuffer(swap_chain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer));

            {
                ID3D11Resource *res;
                ASSERT_POSIX(ID3D11Texture2D_QueryInterface(back_buffer, &IID_ID3D11Resource, (void **)&res));
                ASSERT_POSIX(ID3D11Device_CreateRenderTargetView(device, res, 0, &render_target_view));
                ID3D11Resource_Release(res);
            }
            ID3D11Texture2D_Release(back_buffer);
        }

        ID3D11Texture2D *depth_stencil_buffer;
        ID3D11DepthStencilView *depth_stencil_view;

        {
            const D3D11_TEXTURE2D_DESC depth_stencil_desc = {
                .Width = 640,
                .Height = 480,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_D32_FLOAT,
                .SampleDesc.Count = 1,
                .SampleDesc.Quality = 0,
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_DEPTH_STENCIL,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };
            ASSERT_POSIX(ID3D11Device_CreateTexture2D(device, &depth_stencil_desc, 0, &depth_stencil_buffer));
        }

        {
            ID3D11Resource *res;
            ASSERT_POSIX(ID3D11Texture2D_QueryInterface(depth_stencil_buffer, &IID_ID3D11Resource, (void **)&res));
            ASSERT_POSIX(ID3D11Device_CreateDepthStencilView(device, res, 0, &depth_stencil_view));
            ID3D11Resource_Release(res);
        }

        ID3D11DeviceContext_OMSetRenderTargets(device_context, 1, &render_target_view, depth_stencil_view);

        {
            const D3D11_VIEWPORT viewport = {
                .TopLeftX = 0.f,
                .TopLeftY = 0.f,
                .Width = 640.f,
                .Height = 480.f,
                .MinDepth = 0.f,
                .MaxDepth = 1.f
            };
            ID3D11DeviceContext_RSSetViewports(device_context, 1, &viewport);
        }

        ID3D11RasterizerState *raster_cull_back;

        {
            const D3D11_RASTERIZER_DESC rs_desc = {
                .FillMode = D3D11_FILL_WIREFRAME,
                .CullMode = D3D11_CULL_BACK,
                .FrontCounterClockwise = FALSE,
                .DepthBias = 0,
                .DepthBiasClamp = 0.f,
                .SlopeScaledDepthBias = 0.f,
                .DepthClipEnable = TRUE,
                .ScissorEnable = FALSE,
                .MultisampleEnable = FALSE,
                .AntialiasedLineEnable = FALSE
            };
            ID3D11Device_CreateRasterizerState(device, &rs_desc, &raster_cull_back);
        }

        ID3D11DeviceContext_RSSetState(device_context, raster_cull_back);

        ID3D11Buffer *vertex_buf;

        {
            const float vertex_position[] =
            {
                -1.f, 1.f,
                1.f, 1.f,
                -1.f, -1.f,
                1.f, -1.f
            };

            const D3D11_BUFFER_DESC position_buf_desc = {
                .ByteWidth = sizeof(vertex_position),
                .Usage = D3D11_USAGE_IMMUTABLE,
                .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };

            const D3D11_SUBRESOURCE_DATA position_init = {
                .pSysMem = vertex_position,
                .SysMemPitch = 0,
                .SysMemSlicePitch = 0
            };

            ASSERT_POSIX(ID3D11Device_CreateBuffer(device, &position_buf_desc, &position_init, &vertex_buf));
        }

        ID3D11Buffer *vertex_index_buf;

        {
            const uint16_t vertex_index[] =
            {
                0, 1, 2, 3
            };

            const D3D11_BUFFER_DESC index_buf_desc = {
                .ByteWidth = sizeof(vertex_index),
                .Usage = D3D11_USAGE_IMMUTABLE,
                .BindFlags = D3D11_BIND_INDEX_BUFFER,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };

            const D3D11_SUBRESOURCE_DATA index_init = {
                .pSysMem = vertex_index,
                .SysMemPitch = 0,
                .SysMemSlicePitch = 0
            };

            ASSERT_POSIX(ID3D11Device_CreateBuffer(device, &index_buf_desc, &index_init, &vertex_index_buf));
        }

        ID3D11Buffer *constants_buf;

        {
            unsigned char *const wvp_matrix_adr = (void *)render_desc->wvp_matrix;
            const D3D11_BUFFER_DESC constants_buf_desc = {
                .ByteWidth = sizeof(float [4][4]),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                .MiscFlags = 0
            };

            const D3D11_SUBRESOURCE_DATA constants_init = {
                .pSysMem = render_desc->wvp_matrix,
                .SysMemPitch = 0,
                .SysMemSlicePitch = 0
            };

            ASSERT_POSIX(ID3D11Device_CreateBuffer(device, &constants_buf_desc, &constants_init, &constants_buf));
        }

        ID3D11VertexShader *vertex_shader;
        ID3D11PixelShader *pixel_shader;
        ID3D11InputLayout *input_layout;
        {
            const D3D11_INPUT_ELEMENT_DESC layout_desc = {
                "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
            };

            unsigned char *const vertex_shader_blob = page_alloc(ALLOCATOR_CONSTANTS.STRING_SPACE, 0x80000, 1);
            unsigned char *const pixel_shader_blob = vertex_shader_blob + 0x40000;
            const unsigned long vertex_shader_size = read_small_file(vertex_shader_blob, TEXT("..\\hlsl\\vertex_shader.cso"));
            const unsigned long pixel_shader_size = read_small_file(pixel_shader_blob, TEXT("..\\hlsl\\pixel_shader.cso"));

            ASSERT_POSIX(ID3D11Device_CreateVertexShader(device, vertex_shader_blob, vertex_shader_size, NULL, &vertex_shader));
            ASSERT_POSIX(ID3D11Device_CreateInputLayout(device, &layout_desc, 1, vertex_shader_blob, vertex_shader_size, &input_layout));
            ASSERT_POSIX(ID3D11Device_CreatePixelShader(device, pixel_shader_blob, pixel_shader_size, NULL, &pixel_shader));

            page_free(ALLOCATOR_CONSTANTS.STRING_SPACE, 0x80000, 1);
        }

        uint32_t vertex_strides = sizeof(float) * 2;
        uint32_t vertex_offsets = 0;

        ID3D11DeviceContext_IASetVertexBuffers(device_context, 0, 1, &vertex_buf, &vertex_strides, &vertex_offsets);
        ID3D11DeviceContext_IASetIndexBuffer(device_context, vertex_index_buf, DXGI_FORMAT_R16_UINT, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(device_context, 0, 1, &constants_buf);
        ID3D11DeviceContext_VSSetShader(device_context, vertex_shader, NULL, 0);
        ID3D11DeviceContext_PSSetShader(device_context, pixel_shader, NULL, 0);

        ID3D11DeviceContext_IASetInputLayout(device_context, input_layout);
        ID3D11DeviceContext_IASetPrimitiveTopology(device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        state_desc->sensitivity = 0.001f;
        state_desc->scroll = 1000.f;
        state_desc->status_flags = 0;
        state_desc->raster_cull_back = raster_cull_back;
        state_desc->vertex_shader = vertex_shader;
        state_desc->pixel_shader = pixel_shader;
        state_desc->input_layout = input_layout;
        state_desc->depth_stencil_buffer = depth_stencil_buffer;
        state_desc->render_target_view = render_target_view;
        state_desc->depth_stencil_view = depth_stencil_view;
        state_desc->swap_chain = swap_chain;
        state_desc->device_context = device_context;
        state_desc->device = device;

        {
            IDXGIOutput *dxgi_out;
            ASSERT_POSIX(IDXGISwapChain_GetContainingOutput(swap_chain, &dxgi_out));
            uint32_t num_modes = 0;
            ASSERT_POSIX(IDXGIOutput_GetDisplayModeList(dxgi_out, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &num_modes, NULL));
            DXGI_MODE_DESC *const dxgi_modes = page_alloc(ALLOCATOR_CONSTANTS.STATE_SPACE, sizeof(DXGI_MODE_DESC) * num_modes, 1);
            ASSERT_POSIX(IDXGIOutput_GetDisplayModeList(dxgi_out, DXGI_FORMAT_R8G8B8A8_UNORM, 0, &num_modes, dxgi_modes));
            IDXGIOutput_Release(dxgi_out);
            state_desc->dxgi_modes = dxgi_modes;
            state_desc->num_dxgi_modes = num_modes;
            state_desc->cur_dxgi_mode_idx = num_modes - 1;
            ASSERT_POSIX(IDXGISwapChain_ResizeTarget(swap_chain, &dxgi_modes[num_modes - 1]));
            window_resize_buffers(state_desc, dxgi_modes[num_modes - 1].Width, dxgi_modes[num_modes - 1].Height);
        }

        state_desc->vertex_buf = vertex_buf;
        state_desc->vertex_index_buf = vertex_index_buf;
        state_desc->constants_buf = constants_buf;
        state_desc->vertex_strides = vertex_strides;
        state_desc->vertex_offsets = vertex_offsets;
    }

    ShowWindow(main_hwnd, nShowCmd);
    UpdateWindow(main_hwnd);

    ASSERT_POSIX(IDXGISwapChain_SetFullscreenState(state_desc->swap_chain, TRUE, NULL));

    {
        const HOOKPROC hookproc = &wheel_proc;
        const HHOOK hook = SetWindowsHookExW(WH_MOUSE, hookproc, NULL, GetCurrentThreadId());
        ASSERT(hook);
    }

    double freq;
    {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        freq = 1. / (double)li.QuadPart;
    }

    float (*restrict wvp_matrix)[4] = page_alloc(ALLOCATOR_CONSTANTS.STATE_SPACE, sizeof(float [4][4]), 1);
    double time_start = 0.;

    for(;;) {
        MSG msg;
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                ASSERT_POSIX(IDXGISwapChain_SetFullscreenState(state_desc->swap_chain, FALSE, NULL));
                ID3D11RasterizerState_Release(state_desc->raster_cull_back);
                ID3D11Texture2D_Release(state_desc->depth_stencil_buffer);
                ID3D11InputLayout_Release(state_desc->input_layout);
                ID3D11VertexShader_Release(state_desc->vertex_shader);
                ID3D11PixelShader_Release(state_desc->pixel_shader);
                ID3D11Buffer_Release(state_desc->vertex_buf);
                ID3D11Buffer_Release(state_desc->vertex_index_buf);
                ID3D11Buffer_Release(state_desc->constants_buf);
                ID3D11DeviceContext_Release(state_desc->device_context);
                ID3D11Device_Release(state_desc->device);
                ID3D11DepthStencilView_Release(state_desc->depth_stencil_view);
                ID3D11RenderTargetView_Release(state_desc->render_target_view);
                IDXGISwapChain_Release(state_desc->swap_chain);
                allocator_deinit(allocator);
                return (int)msg.wParam;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if(state_desc->status_flags & 1) {
            Sleep(5);
            continue;
        }
        //camera position transform
        {
            const int32_t w_key_state          = ((int32_t)GetAsyncKeyState(0x57) << 16) >> 31;
            const int32_t s_key_state          = ((int32_t)GetAsyncKeyState(0x53) << 16) >> 31;
            const int32_t a_key_state          = ((int32_t)GetAsyncKeyState(0x41) << 16) >> 31;
            const int32_t d_key_state          = ((int32_t)GetAsyncKeyState(0x44) << 16) >> 31;
            const __m128 neg_sign              = _mm_set1_ps(-0.0f);
            const float3 camera_lookat         = render_desc->camera_lookat;
            const ALIGN(16) float3 w           = render_desc->camera_w;
            const __m128 camera_speed          = _mm_load1_ps(&render_desc->camera_speed);
            const __m128 z_pack                = _mm_set_ps(0.f, w.c, w.b, w.a);

            const ALIGN(16) union {
                int32_t i[4];
                float f[4];
            } u = {.i = {w_key_state, s_key_state, a_key_state, d_key_state}};

            const __m128 w_key_pack  = _mm_set1_ps(u.f[0]);
            const __m128 s_key_pack  = _mm_set1_ps(u.f[1]);
            const __m128 a_key_pack  = _mm_set1_ps(u.f[2]);
            const __m128 d_key_pack  = _mm_set1_ps(u.f[3]);

            //cross-product of w vector(look at direction) with up vector(0, 1, 0) -> camera x-direction vector denorm
            const ALIGN(16) float3 x_denorm = {
                .a = w.c,
                .b = 0.f,
                .c = -w.a
            };
            const ALIGN(16) float3 x_norm = vector3_normal(x_denorm);
            const __m128 x_pack        = _mm_set_ps(0.f, x_norm.c, x_norm.b, x_norm.a);

            const __m128 ad_xor        = _mm_xor_ps(a_key_pack, d_key_pack);
            const __m128 ws_xor        = _mm_xor_ps(w_key_pack, s_key_pack);
            const __m128 ws_sign       = _mm_and_ps(s_key_pack, neg_sign);
            const __m128 ad_sign       = _mm_and_ps(a_key_pack, neg_sign);
            const __m128 x_translation = _mm_mul_ps(x_pack, camera_speed);
            const __m128 z_translation = _mm_mul_ps(z_pack, camera_speed);
            const __m128 x_abs         = _mm_and_ps(x_translation, ad_xor);
            const __m128 z_abs         = _mm_and_ps(z_translation, ws_xor);
            const __m128 x_res         = _mm_xor_ps(x_abs, ad_sign);
            const __m128 z_res         = _mm_xor_ps(z_abs, ws_sign);
            const __m128 res           = _mm_add_ps(x_res, z_res);
            const float3 w_new         = vector3_normal(camera_lookat);
            const float3 v_new         = vector3_cross(w_new, x_norm);

            float4 res_f;
            _mm_store_ps(res_f.arr, res);
            const float3 camera_pos = render_desc->camera_pos;
            render_desc->camera_pos.a = res_f.a + camera_pos.a;
            render_desc->camera_pos.b = res_f.b + camera_pos.b;
            render_desc->camera_pos.c = res_f.c + camera_pos.c;

            render_desc->camera_u = x_norm;
            render_desc->camera_v = v_new;
            render_desc->camera_w = w_new;
        }
        //camera rotation transform
        {
            const uint16_t window_coords_x      = state_desc->window_coords.x;
            const uint16_t window_coords_y      = state_desc->window_coords.y;
            const uint32_t window_coords_width  = state_desc->window_coords.width;
            const uint32_t window_coords_height = state_desc->window_coords.height;
            const float   sensitivity           = state_desc->sensitivity;
            const uint16_t window_center_x      = (uint16_t)(window_coords_width >> 1)  + window_coords_x;
            const uint16_t window_center_y      = (uint16_t)(window_coords_height >> 1) + window_coords_y;
            POINT mouse_coords;
            ASSERT(GetCursorPos(&mouse_coords));
            ASSERT(SetCursorPos(window_center_x, window_center_y));

            const float x   = mouse_coords.x - window_center_x;
            const float y_u = mouse_coords.y - window_center_y;
            const float wb  = render_desc->camera_w.b;
            ASSERT(fabsf(wb) < 1.f);

            const __m128 pack = _mm_set_ps(0.f, wb, wb, y_u);
            const __m128 comp = _mm_set_ps(0.f, 0.99f, -0.99f, 0.f);
            const uint8_t mask = (uint8_t)_mm_movemask_ps(_mm_add_ps(pack, comp));
            union {
                float f;
                uint32_t u;
            } u = {.f = y_u};
            u.u &= ~(uint32_t)((int32_t)(((mask & ~(mask >> 1)) | ((~mask) & (mask >> 2))) << 31) >> 31);
            const float y = u.f;

            if(fabsf(x + y) > 0) {
                const float3    camera_lookat   = render_desc->camera_lookat;
                const ALIGN(16) float4 camera_u = {.f123 = render_desc->camera_u, .f4 = 0.0f};
                const ALIGN(16) float4 camera_v = {.f123 = render_desc->camera_v, .f4 = 0.0f};
                const __m128    camera_u_pack   = _mm_load_ps(camera_u.arr);
                const __m128    camera_v_pack   = _mm_load_ps(camera_v.arr);
                const __m128    y_pack          = _mm_set1_ps(y);
                const __m128    x_pack          = _mm_set1_ps(x);

                const float dist = sqrtf(x * x + y * y);
                const float angle = (dist * sensitivity);
                const float q_sin = sinf(angle);
                const float q_cos = sqrtf(1 - q_sin * q_sin);

                const __m128 rot_u      = _mm_mul_ps(camera_u_pack, y_pack);
                const __m128 rot_v      = _mm_mul_ps(camera_v_pack, x_pack);
                const __m128 rot        = _mm_add_ps(rot_u, rot_v);
                float4 rot_unpack;
                _mm_store_ps(rot_unpack.arr, rot);
                const float3 rot_unit   = vector3_normal(rot_unpack.f123);

                float4 q = {.a = rot_unit.a * q_sin, .b = rot_unit.b * q_sin, .c = rot_unit.c * q_sin, .d = q_cos};
                float4 qc = {.a = -q.a, .b = -q.b, .c = -q.c, .d = q.d};
                float4 interm = {
                    .a = (q.b * camera_lookat.c) + (q.d * camera_lookat.a) - (q.c * camera_lookat.b),
                    .b = (q.c * camera_lookat.a) + (q.d * camera_lookat.b) - (q.a * camera_lookat.c),
                    .c = (q.a * camera_lookat.b) - (q.b * camera_lookat.a) + (q.d * camera_lookat.c),
                    .d = -(q.a * camera_lookat.a) - (q.b * camera_lookat.b) - (q.c * camera_lookat.c)
                };
                float3 new_front = {
                    .a = (interm.b * qc.c) + (interm.a * qc.d) - (interm.c * qc.b) + (interm.d * qc.a),
                    .b = (interm.c * qc.a) - (interm.a * qc.c) + (interm.b * qc.d) + (interm.d * qc.b),
                    .c = (interm.a * qc.b) - (interm.b * qc.a) + (interm.c * qc.d) + (interm.d * qc.c)
                };
                render_desc->camera_lookat = new_front;
            }
        }
        //copy new frame world-view-projection, draw current frame
        {
            ID3D11RenderTargetView *const render_target_view = state_desc->render_target_view;
            ID3D11DepthStencilView *const depth_stencil_view = state_desc->depth_stencil_view;
            ID3D11Device           *const device             = state_desc->device;
            ID3D11DeviceContext    *const device_context     = state_desc->device_context;
            IDXGISwapChain         *const swap_chain         = state_desc->swap_chain;
            ID3D11Buffer           *const constants_buf      = state_desc->constants_buf;
            const hva4 wvp_matrix_pack = {
                .x = _mm_load_ps(wvp_matrix[0]),
                .y = _mm_load_ps(wvp_matrix[1]),
                .z = _mm_load_ps(wvp_matrix[2]),
                .w = _mm_load_ps(wvp_matrix[3])
            };
            const hva4 wvp_matrix_tr_pack = matrix4x4_transpose(wvp_matrix_pack);
            _mm_store_ps(render_desc->wvp_matrix[0], wvp_matrix_tr_pack.x);
            _mm_store_ps(render_desc->wvp_matrix[1], wvp_matrix_tr_pack.y);
            _mm_store_ps(render_desc->wvp_matrix[2], wvp_matrix_tr_pack.z);
            _mm_store_ps(render_desc->wvp_matrix[3], wvp_matrix_tr_pack.w);
            D3D11_MAPPED_SUBRESOURCE constants_buf_sub;
            memset(&constants_buf_sub, 0, sizeof(constants_buf_sub));
            {
                ID3D11Resource *res;
                ASSERT_POSIX(ID3D11Buffer_QueryInterface(constants_buf, &IID_ID3D11Resource, (void **)&res));
                ASSERT_POSIX(ID3D11DeviceContext_Map(device_context, res, 0, D3D11_MAP_WRITE_DISCARD, 0, &constants_buf_sub));
                memcpy(constants_buf_sub.pData, &render_desc->wvp_matrix, sizeof(render_desc->wvp_matrix));
                ID3D11DeviceContext_Unmap(device_context, res, 0);
                ID3D11Resource_Release(res);
            }

            ASSERT_POSIX(IDXGISwapChain_Present(swap_chain, 0, 0));
            ID3D11DeviceContext_OMSetRenderTargets(device_context, 1, &render_target_view, depth_stencil_view);

            static const float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            ID3D11DeviceContext_ClearRenderTargetView(device_context, render_target_view, clear_color);
            ID3D11DeviceContext_ClearDepthStencilView(device_context, depth_stencil_view, 
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

            ID3D11DeviceContext_DrawIndexedInstanced(device_context, 4, 4096, 0, 0, 0);
        }
        //build view and projection matrices, multiply everything
        {
            const float3 camera_pos = render_desc->camera_pos;
            const float3 camera_u   = render_desc->camera_u;
            const float3 camera_v   = render_desc->camera_v;
            const float3 camera_w   = render_desc->camera_w;
            const float ALIGN(64) view_matrix[4][4] = {
                [0][0] = camera_u.a, [0][1] = camera_v.a, 
                [0][2] = camera_w.a, [0][3] = 0.f,
                [1][0] = camera_u.b, [1][1] = camera_v.b, 
                [1][2] = camera_w.b, [1][3] = 0.f,
                [2][0] = camera_u.c, [2][1] = camera_v.c, 
                [2][2] = camera_w.c, [2][3] = 0.f,
                [3][0] = -vector3_dot(camera_u, camera_pos),
                [3][1] = -vector3_dot(camera_v, camera_pos),
                [3][2] = -vector3_dot(camera_w, camera_pos),
                [3][3] = 1.f
            };

            float ALIGN(64) projection_matrix[4][4];
            matrix_projection_LH(projection_matrix, (float)M_PI * (state_desc->scroll * 0.0003f), 
                (float)state_desc->window_coords.width / (float)state_desc->window_coords.height, 0.1f, 1000.f);
            
            float ALIGN(64) wv_matrix[4][4];
            matrix4x4_vector4_mul(wv_matrix, render_desc->world_matrix, view_matrix);
            matrix4x4_mul(wvp_matrix, wv_matrix, projection_matrix);
        }

        double time_end;
        {
            LARGE_INTEGER li;
            QueryPerformanceCounter(&li);
            time_end = (double)li.QuadPart;
        }
        render_desc->camera_speed = 22.5f * (float)((time_end - time_start) * freq);
        #ifdef DEBUG
        printf("Time elapsed: %.6f ms\n", (double)((time_end - time_start) * 1000. * freq));
        #endif
        time_start = time_end;

        //threads_clear();
        page_free_all(ALLOCATOR_CONSTANTS.FRAME_SPACE);
    }
}
