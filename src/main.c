/*#ifndef DEBUG
#define DEBUG
#endif*/

#include "pch.h"
#include "debug.c"
#include "allocator.c"
#include "directx.c"
#include "linear_math.c"
#include "graphics.c"
#include "threads.c"

struct State_Desc {
    struct {
        int32_t x;
        int32_t y;
    } window_center;
    float camera_speed;
    float sensitivity;
    uint16_t scroll;
    uint8_t status_flags;
    ID3D11Device *device;
    ID3D11DeviceContext *device_context;
    IDXGISwapChain *swap_chain;
    D3D11_TEXTURE2D_DESC depth_stencil_desc;
    D3D11_VIEWPORT viewport;
    ID3D11Texture2D *depth_stencil_buffer;
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilView *depth_stencil_view;
};

static LRESULT CALLBACK wnd_proc(HWND main_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct State_Desc *restrict state_desc = (struct State_Desc *)(void *)STATE_SPACE;
    static long max_width = 0;
    static long max_height = 0;

	switch(msg)
	{
		/*case WM_CREATE: {

		}
		break;*/
		case WM_CLOSE: {
            if(MessageBox(main_hwnd, L"Are you sure you want to quit?", L"Quit?", MB_YESNO | MB_ICONQUESTION) == IDYES)
                DestroyWindow(main_hwnd);
            else
                return 0;
		}
		break;
        case WM_SIZE: {
            if(wParam == SIZE_MINIMIZED) {
                state_desc->status_flags |= 0x01;
            } else {
                RECT rect;
                ASSERT(GetClientRect(main_hwnd, &rect));
                POINT p1 = {.x = rect.left, .y = rect.top};
                POINT p2 = {.x = rect.right, .y = rect.bottom};
                ASSERT(ClientToScreen(main_hwnd, &p1));
                ASSERT(ClientToScreen(main_hwnd, &p2));
                state_desc->window_center.x = ((p2.x - p1.x) >> 1) + p1.x;
                state_desc->window_center.y = ((p2.y - p1.y) >> 1) + p1.y;
            }
            if(state_desc->status_flags & 0x02) {
                BOOL fullscreen;
                ASSERT_POSIX(IDXGISwapChain_GetFullscreenState(state_desc->swap_chain, &fullscreen, NULL));
                uint16_t width = lParam & 0xFFFF;
                uint16_t height = (lParam & 0xFFFF0000) >> 16;
                BOOL cur_fullscreen = width == max_width && height == max_height;

                if(fullscreen == FALSE && cur_fullscreen == TRUE)
                    ASSERT_POSIX(IDXGISwapChain_SetFullscreenState(state_desc->swap_chain, TRUE, NULL));
                else if(fullscreen == TRUE && cur_fullscreen == FALSE)
                    ASSERT_POSIX(IDXGISwapChain_SetFullscreenState(state_desc->swap_chain, FALSE, NULL));

                ID3D11DeviceContext_OMSetRenderTargets(state_desc->device_context, 0, NULL, NULL);
                ID3D11RenderTargetView_Release(state_desc->render_target_view);
                ID3D11DepthStencilView_Release(state_desc->depth_stencil_view);
                ASSERT_POSIX(IDXGISwapChain_ResizeBuffers(state_desc->swap_chain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

                ID3D11Texture2D *back_buffer;
                ASSERT_POSIX(IDXGISwapChain_GetBuffer(state_desc->swap_chain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer));
                {
                    ID3D11Resource *res;
                    ASSERT_POSIX(ID3D11Texture2D_QueryInterface(back_buffer, &IID_ID3D11Resource, (void **)&res));
                    ASSERT_POSIX(ID3D11Device_CreateRenderTargetView(state_desc->device, res, 0, &state_desc->render_target_view));
                    ID3D11Resource_Release(res);
                }
                ID3D11Texture2D_Release(back_buffer);

                state_desc->depth_stencil_desc.Width = width;
                state_desc->depth_stencil_desc.Height = height;

                ID3D11Texture2D_Release(state_desc->depth_stencil_buffer);
                ASSERT_POSIX(ID3D11Device_CreateTexture2D(state_desc->device, &state_desc->depth_stencil_desc, 0, &state_desc->depth_stencil_buffer));
                {
                    ID3D11Resource *res;
                    ASSERT_POSIX(ID3D11Texture2D_QueryInterface(state_desc->depth_stencil_buffer, &IID_ID3D11Resource, (void **)&res));
                    ASSERT_POSIX(ID3D11Device_CreateDepthStencilView(state_desc->device, res, 0, &state_desc->depth_stencil_view));
                    ID3D11Resource_Release(res);
                }

                ID3D11DeviceContext_OMSetRenderTargets(state_desc->device_context, 1, &state_desc->render_target_view, state_desc->depth_stencil_view);

                state_desc->viewport.Width = width;
                state_desc->viewport.Height = height;

                ID3D11DeviceContext_RSSetViewports(state_desc->device_context, 1, &state_desc->viewport);
            }
        }
        break;
        case WM_ENTERSIZEMOVE: {
            state_desc->status_flags = 1;
        }
        break;
        case WM_EXITSIZEMOVE: {
            RECT rect;
            ASSERT(GetClientRect(main_hwnd, &rect));
            POINT p1 = {.x = rect.left, .y = rect.top};
            POINT p2 = {.x = rect.right, .y = rect.bottom};
            ASSERT(ClientToScreen(main_hwnd, &p1));
            ASSERT(ClientToScreen(main_hwnd, &p2));
            state_desc->window_center.x = ((p2.x - p1.x) >> 1) + p1.x;
            state_desc->window_center.y = ((p2.y - p1.y) >> 1) + p1.y;
        }
        break;
        case WM_GETMINMAXINFO: {
            PMINMAXINFO minmax_info = (PMINMAXINFO)lParam;
            max_width = minmax_info->ptMaxSize.x;
            max_height = minmax_info->ptMaxSize.y;
        }
        break;
		case WM_DESTROY: {
			PostQuitMessage(0);
		}
		break;
		case WM_ACTIVATE: {
            if((wParam & 0xFF) == WA_INACTIVE)
                state_desc->status_flags |= 0x01;
		}
		break;
        case WM_KEYDOWN: {
            if((wParam & 0xFF) == VK_ESCAPE) {
                state_desc->status_flags ^= 0x01;
            }
        }
        break;
        case WM_SETCURSOR: {
            SetCursor((void *)((uintptr_t)((state_desc->status_flags & 0x01) * UINTPTR_MAX) & (uintptr_t)(LoadCursor(NULL, IDC_ARROW))));
        }
        break;
	}
	return DefWindowProc(main_hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK wheel_proc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if(nCode < 0)
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	if(wParam == WM_MOUSEWHEEL) {
		MOUSEHOOKSTRUCTEX* mouse_info = (MOUSEHOOKSTRUCTEX*)lParam;
        struct State_Desc *restrict state_desc = (struct State_Desc *)(void *)STATE_SPACE;
		uint16_t scroll = state_desc->scroll;
		uint16_t val = HIWORD(mouse_info->mouseData);
		scroll += ((uint16_t)((int16_t)(1 - scroll + val) >> 15) & (uint16_t)((int16_t)(scroll - val - 2000) >> 15)) * val;
        state_desc->scroll = scroll;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void cleanup(void)
{
    struct State_Desc *restrict state_desc = (struct State_Desc *)(void *)STATE_SPACE;
    ASSERT_POSIX(IDXGISwapChain_SetFullscreenState(state_desc->swap_chain, FALSE, NULL));
    threads_deinit();
    allocator_deinit();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance)
	UNREFERENCED_PARAMETER(lpCmdLine)

	atexit(cleanup);

    allocator_init();
    threads_init();

	WNDCLASSEX main_window;
	HWND main_hwnd;
	MSG msg;

	const wchar_t g_szClassName[] = L"main_window";

	main_window.cbSize = sizeof(WNDCLASSEX);
	main_window.style = CS_HREDRAW | CS_VREDRAW;
	main_window.lpfnWndProc = wnd_proc;
	main_window.cbClsExtra = 0;
	main_window.cbWndExtra = 0;
	main_window.hInstance = hInstance;
	main_window.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	main_window.hCursor = NULL;
	main_window.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	main_window.lpszMenuName = NULL;
	main_window.lpszClassName = g_szClassName;
	main_window.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	ASSERT(RegisterClassEx(&main_window));

	main_hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		L"game",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1024,
		768,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	#ifdef DEBUG
		ASSERT(AllocConsole());
		ASSERT(SetConsoleTitle(L"Debug Output"));
		ASSERT(freopen("CONIN$", "r", stdin));
		ASSERT(freopen("CONOUT$", "w", stdout));
        //ASSERT(freopen("log.txt", "w", stdout));
	#endif

    struct State_Desc *state_desc = page_alloc(STATE_SPACE, sizeof(struct State_Desc), 1);

    {
        RECT rect;
        ASSERT(GetClientRect(main_hwnd, &rect));
        POINT p1 = {.x = rect.left, .y = rect.top};
        POINT p2 = {.x = rect.right, .y = rect.bottom};
        ASSERT(ClientToScreen(main_hwnd, &p1));
        ASSERT(ClientToScreen(main_hwnd, &p2));
        state_desc->window_center.x = ((p2.x - p1.x) >> 1) + p1.x;
        state_desc->window_center.y = ((p2.y - p1.y) >> 1) + p1.y;
    }

    state_desc->sensitivity = 0.001f;
    state_desc->scroll = 1000;
    state_desc->status_flags = 0;

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = 1024;
	sd.BufferDesc.Height = 768;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.OutputWindow = main_hwnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	unsigned int createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	ASSERT_POSIX(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
		createDeviceFlags, NULL, 0, D3D11_SDK_VERSION, &sd, &state_desc->swap_chain, &state_desc->device, NULL, &state_desc->device_context));
	ID3D11Texture2D *back_buffer;
	ASSERT_POSIX(IDXGISwapChain_GetBuffer(state_desc->swap_chain, 0, &IID_ID3D11Texture2D, (void **)&back_buffer));
    {
        ID3D11Resource *res;
        ASSERT_POSIX(ID3D11Texture2D_QueryInterface(back_buffer, &IID_ID3D11Resource, (void **)&res));
        ASSERT_POSIX(ID3D11Device_CreateRenderTargetView(state_desc->device, res, 0, &state_desc->render_target_view));
    	ID3D11Resource_Release(res);
    }
	ID3D11Texture2D_Release(back_buffer);

	state_desc->depth_stencil_desc.Width = 1024;
	state_desc->depth_stencil_desc.Height = 768;
	state_desc->depth_stencil_desc.MipLevels = 1;
	state_desc->depth_stencil_desc.ArraySize = 1;
	state_desc->depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	state_desc->depth_stencil_desc.SampleDesc.Count = 1;
	state_desc->depth_stencil_desc.SampleDesc.Quality = 0;
	state_desc->depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	state_desc->depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	state_desc->depth_stencil_desc.CPUAccessFlags = 0;
	state_desc->depth_stencil_desc.MiscFlags = 0;

	ASSERT_POSIX(ID3D11Device_CreateTexture2D(state_desc->device, &state_desc->depth_stencil_desc, 0, &state_desc->depth_stencil_buffer));
    {
        ID3D11Resource *res;
        ASSERT_POSIX(ID3D11Texture2D_QueryInterface(state_desc->depth_stencil_buffer, &IID_ID3D11Resource, (void **)&res));
        ASSERT_POSIX(ID3D11Device_CreateDepthStencilView(state_desc->device, res, 0, &state_desc->depth_stencil_view));
        ID3D11Resource_Release(res);
    }

	ID3D11DeviceContext_OMSetRenderTargets(state_desc->device_context, 1, &state_desc->render_target_view, state_desc->depth_stencil_view);

	state_desc->viewport.TopLeftX = 0;
	state_desc->viewport.TopLeftY = 0;
	state_desc->viewport.Width = 1024;
	state_desc->viewport.Height = 768;
	state_desc->viewport.MinDepth = 0;
	state_desc->viewport.MaxDepth = 1;

	ID3D11DeviceContext_RSSetViewports(state_desc->device_context, 1, &state_desc->viewport);

    D3D11_RASTERIZER_DESC rs_desc;
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = D3D11_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.f;
    rs_desc.SlopeScaledDepthBias = 0.f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = FALSE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;

    ID3D11RasterizerState *raster_cull_back;
    ID3D11Device_CreateRasterizerState(state_desc->device, &rs_desc, &raster_cull_back);
    ID3D11DeviceContext_RSSetState(state_desc->device_context, raster_cull_back);

    Render_desc *render_desc = page_alloc(STATIC_SPACE, sizeof(Render_desc), 1);

    render_desc->camera.a = 0.f;
    render_desc->camera.b = 0.f;
    render_desc->camera.c = 0.f;
    render_desc->front.a = 0.f;
    render_desc->front.b = 0.f;
    render_desc->front.c = 1.f;

    {
        render_desc->world_matrix[0][0] = 10.f;   render_desc->world_matrix[0][1] = 0.f;    render_desc->world_matrix[0][2] = 0.f;    render_desc->world_matrix[0][3] = 0.f;
        render_desc->world_matrix[1][0] = 0.f;    render_desc->world_matrix[1][1] = 10.f;   render_desc->world_matrix[1][2] = 0.f;    render_desc->world_matrix[1][3] = 0.f;
        render_desc->world_matrix[2][0] = 0.f;    render_desc->world_matrix[2][1] = 0.f;    render_desc->world_matrix[2][2] = 10.f;   render_desc->world_matrix[2][3] = 0.f;
        render_desc->world_matrix[3][0] = 0.f;    render_desc->world_matrix[3][1] = 0.f;    render_desc->world_matrix[3][2] = 0.f;    render_desc->world_matrix[3][3] = 1.f;
    }

    float vertices_position[] =
    {
        -1.f, 1.f, -1.f,
        -1.f, -1.f, -1.f,
        1.f, -1.f, -1.f,
        1.f, 1.f, -1.f,
        -1.f, 1.f, 1.f,
        -1.f, -1.f, 1.f,
        1.f, -1.f, 1.f,
        1.f, 1.f, 1.f
    };

    float vertices_color[] =
    {
        1.f, 0.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        1.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        1.f, 1.f, 0.5f, 1.f
    };

    uint16_t vertices_index[] =
    {
        //0, 3, 1, 2, 6, 3, 7, 0, 4, 1, 5, 6, 4, 7
        3, 0, 2, 1, 5, 0, 4, 3, 7, 2, 6, 5, 7, 4
    };

    ID3D11Buffer *vertices_buf[2];

    D3D11_BUFFER_DESC position_buf_desc;
    position_buf_desc.ByteWidth = sizeof(vertices_position);
    position_buf_desc.Usage = D3D11_USAGE_IMMUTABLE;
    position_buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    position_buf_desc.CPUAccessFlags = 0;
    position_buf_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA position_init;
    position_init.pSysMem = vertices_position;
    position_init.SysMemPitch = 0;
    position_init.SysMemSlicePitch = 0;

    ASSERT_POSIX(ID3D11Device_CreateBuffer(state_desc->device, &position_buf_desc, &position_init, &vertices_buf[0]));

    D3D11_BUFFER_DESC color_buf_desc;
    color_buf_desc.ByteWidth = sizeof(vertices_color);
    color_buf_desc.Usage = D3D11_USAGE_IMMUTABLE;
    color_buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    color_buf_desc.CPUAccessFlags = 0;
    color_buf_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA color_init;
    color_init.pSysMem = vertices_color;
    color_init.SysMemPitch = 0;
    color_init.SysMemSlicePitch = 0;

    ASSERT_POSIX(ID3D11Device_CreateBuffer(state_desc->device, &color_buf_desc, &color_init, &vertices_buf[1]));

    ID3D11Buffer *vertices_index_buf;

    D3D11_BUFFER_DESC index_buf_desc;
    index_buf_desc.ByteWidth = sizeof(vertices_index);
    index_buf_desc.Usage = D3D11_USAGE_IMMUTABLE;
    index_buf_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    index_buf_desc.CPUAccessFlags = 0;
    index_buf_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA index_init;
    index_init.pSysMem = vertices_index;
    index_init.SysMemPitch = 0;
    index_init.SysMemSlicePitch = 0;

    ASSERT_POSIX(ID3D11Device_CreateBuffer(state_desc->device, &index_buf_desc, &index_init, &vertices_index_buf));

    ID3D11Buffer *constants_buf;

    D3D11_BUFFER_DESC constants_buf_desc;
    constants_buf_desc.ByteWidth = sizeof(render_desc->wvp_matrix);
    constants_buf_desc.Usage = D3D11_USAGE_DYNAMIC;
    constants_buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constants_buf_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constants_buf_desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA constants_init;
    constants_init.pSysMem = render_desc->wvp_matrix;
    constants_init.SysMemPitch = 0;
    constants_init.SysMemSlicePitch = 0;

    ASSERT_POSIX(ID3D11Device_CreateBuffer(state_desc->device, &constants_buf_desc, &constants_init, &constants_buf));

    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11InputLayout *input_layout;

    D3D11_INPUT_ELEMENT_DESC layout_desc[2] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    create_vertex_shader(state_desc->device, L"..\\hlsl\\vertex_cube_shader.cso", &vertex_shader, &input_layout, layout_desc);
    create_pixel_shader(state_desc->device, L"..\\hlsl\\pixel_cube_shader.cso", &pixel_shader);

    uint32_t strides[2] = {sizeof(float) * 3, sizeof(float) * 4};
    uint32_t offsets[2] = {0, 0};

    ID3D11DeviceContext_IASetVertexBuffers(state_desc->device_context, 0, 2, vertices_buf, strides, offsets);
    ID3D11DeviceContext_IASetIndexBuffer(state_desc->device_context, vertices_index_buf, DXGI_FORMAT_R16_UINT, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(state_desc->device_context, 0, 1, &constants_buf);
    ID3D11DeviceContext_VSSetShader(state_desc->device_context, vertex_shader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(state_desc->device_context, pixel_shader, NULL, 0);

    ID3D11DeviceContext_IASetInputLayout(state_desc->device_context, input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(state_desc->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ShowWindow(main_hwnd, nShowCmd);
	UpdateWindow(main_hwnd);
    //RECT clip_cursor;
    //GetClipCursor(&clip_cursor);

    {
    	HOOKPROC hookproc = &wheel_proc;
		HHOOK hook;
		ASSERT(hook = SetWindowsHookExW(WH_MOUSE, hookproc, NULL, GetCurrentThreadId()));
    }

    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    int64_t fr = li.QuadPart;
    float  freq_float  = 1.f / fr;
    double freq_double = 1. / fr;
    //Task_info task_graphics_move_camera = {0, 0};
    //Task_info task_graphics_rotate_camera = {0, 0};
    //Task_info task_graphics_multiply_matrices = {0, 0};
    Task_info task_graphics_draw = {0, 0};

    float (*restrict wvp_matrix)[4] = page_alloc_lines(STATIC_SPACE, 1);

    state_desc->status_flags |= 0x02;

	for (;;) {
        QueryPerformanceCounter(&li);
        int64_t time_start = li.QuadPart;

		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				return (int)msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(~state_desc->status_flags & 0x01) {

            /*task_graphics_move_camera =
            THREADS_SCHEDULE_TASK(
                struct Args {
                    int func;
                    Render_desc* render_desc;
                    float camera_speed;
                } *args = page_alloc(FRAME_SPACE, sizeof(*args), 1);
                *args = (struct Args){GRAPHICS_MOVE_CAMERA, render_desc, state_desc->camera_speed};
                Deps *deps = page_alloc_lines(FRAME_SPACE, 1);
                deps->num = 1;
                deps->array[0] = task_graphics_move_camera;
            );*/
            graphics_move_camera(render_desc, state_desc->camera_speed);

            /*task_graphics_rotate_camera = 
            THREADS_SCHEDULE_TASK(
                struct Args {
                    int func;
                    Render_desc *render_desc;
                    int32_t window_center_x;
                    int32_t window_center_y;
                    float sensitivity;
                } *args = page_alloc(FRAME_SPACE, sizeof(*args), 1);
                *args = (struct Args){GRAPHICS_ROTATE_CAMERA, render_desc, state_desc->window_center.x, state_desc->window_center.y, state_desc->sensitivity};
                Deps *deps = page_alloc_lines(FRAME_SPACE, 1);
                deps->num = 1;
                deps->array[0] = task_graphics_rotate_camera;
            );*/

            graphics_rotate_camera(render_desc, state_desc->window_center.x, state_desc->window_center.y, state_desc->sensitivity);

            /*task_graphics_draw =
            THREADS_SCHEDULE_TASK(
                struct Args {
                    int func;
                    Render_desc *restrict render_desc;
                    float (*restrict wvp_matrix)[4];
                    ID3D11Buffer *constants_buf;
                    ID3D11DeviceContext *device_context; 
                    ID3D11RenderTargetView *state_desc->render_target_view;
                    ID3D11DepthStencilView *state_desc->depth_stencil_view;
                    IDXGISwapChain *swap_chain;
                } *args = page_alloc(FRAME_SPACE, sizeof(*args), 1);
                *args = (struct Args){GRAPHICS_DRAW, render_desc, wvp_matrix, constants_buf, state_desc->device_context, state_desc->render_target_view, state_desc->depth_stencil_view, state_desc->swap_chain};
                Deps *deps = page_alloc_lines(FRAME_SPACE, 1);
                deps->num = 1;
                deps->array[0] = task_graphics_draw;
            );*/

            graphics_draw(render_desc, wvp_matrix, constants_buf, state_desc->device_context, state_desc->render_target_view, state_desc->depth_stencil_view, state_desc->swap_chain);

            /*task_graphics_multiply_matrices =
            THREADS_SCHEDULE_TASK(
                struct Args {
                    int func;
                    float (*restrict wvp_matrix)[4];
                    Render_desc* render_desc;
                    uint16_t scroll;
                } *args = page_alloc(FRAME_SPACE, sizeof(*args), 1);
                *args = (struct Args){GRAPHICS_MULTIPLY_MATRICES, wvp_matrix, render_desc, state_desc->scroll};
                Deps *deps = page_alloc_lines(FRAME_SPACE, 1);
                deps->num = 1;
                deps->array[0] = task_graphics_multiply_matrices;
            );*/

            graphics_multiply_matrices(wvp_matrix, render_desc, state_desc->scroll);

			QueryPerformanceCounter(&li);
			int64_t time_end = li.QuadPart;
            state_desc->camera_speed = 2.5f * (float)((time_end - time_start)) * freq_float;
            #ifdef DEBUG
				//printf("Time elapsed: %lf ms\n", (double)((time_end - time_start) * 1000ll) * freq_double);
			#endif
		}

        //threads_clear();
        page_free_all(FRAME_SPACE);
	}
}
