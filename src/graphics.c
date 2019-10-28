typedef struct{
    float3 camera;
    float3 front;
    float3 u;
    float3 v;
    float3 w;
    unsigned char pad[4];
    float world_matrix[4][4];
    float projection_matrix[4][4];
    float view_matrix[4][4];
    float wvp_matrix[4][4];
} Render_desc;

static inline void graphics_multiply_matrices(float wvp_matrix[restrict 4][4], Render_desc *restrict render_desc, uint16_t scroll)
{
    float wv_matrix[4][4] ALIGN(64);

    render_desc->view_matrix[0][0] = render_desc->u.a; render_desc->view_matrix[0][1] = render_desc->v.a; 
    render_desc->view_matrix[0][2] = render_desc->w.a; render_desc->view_matrix[0][3] = 0;
    render_desc->view_matrix[1][0] = render_desc->u.b; render_desc->view_matrix[1][1] = render_desc->v.b; 
    render_desc->view_matrix[1][2] = render_desc->w.b; render_desc->view_matrix[1][3] = 0;
    render_desc->view_matrix[2][0] = render_desc->u.c; render_desc->view_matrix[2][1] = render_desc->v.c; 
    render_desc->view_matrix[2][2] = render_desc->w.c; render_desc->view_matrix[2][3] = 0;
    render_desc->view_matrix[3][0] = -vector3_dot(render_desc->u, render_desc->camera);
    render_desc->view_matrix[3][1] = -vector3_dot(render_desc->v, render_desc->camera);
    render_desc->view_matrix[3][2] = -vector3_dot(render_desc->w, render_desc->camera);
    render_desc->view_matrix[3][3] = 1;

    matrix_projection_LH(render_desc->projection_matrix, (float)M_PI * (scroll * 0.0003f), 1024.f / 768.f, 0.1f, 100.f);
    matrix4x4_multiply(wv_matrix, render_desc->world_matrix, render_desc->view_matrix);
    matrix4x4_multiply(wvp_matrix, wv_matrix, render_desc->projection_matrix);
}

static inline void graphics_move_camera(Render_desc *restrict render_desc, float camera_speed)
{
    int16_t w_key_state = GetAsyncKeyState(0x57);
    int16_t s_key_state = GetAsyncKeyState(0x53);
    int16_t a_key_state = GetAsyncKeyState(0x41);
    int16_t d_key_state = GetAsyncKeyState(0x44);

    {
        if(0x8000 & w_key_state) {
            float3 v1 = vector3_mul_scalar(render_desc->front, camera_speed);
            float3 v2 = vector3_add(render_desc->camera, v1);
            render_desc->camera = v2;
        }
        if(0x8000 & s_key_state) {
            float3 v1 = vector3_mul_scalar(render_desc->front, camera_speed);
            float3 v2 = vector3_sub(render_desc->camera, v1);
            render_desc->camera = v2;
        }
        if(0x8000 & a_key_state) {
            float3 v1;
            v1.a = render_desc->front.c;
            v1.b = 0.f;
            v1.c = -render_desc->front.a;
            float3 v2 = vector3_normal(v1);
            v1 = vector3_mul_scalar(v2, camera_speed);
            v2 = vector3_sub(render_desc->camera, v1);
           render_desc->camera = v2;
        }
        if(0x8000 & d_key_state) {
            float3 v1;
            v1.a = render_desc->front.c;
            v1.b = 0.f;
            v1.c = -render_desc->front.a;
            float3 v2 = vector3_normal(v1);
            v1 = vector3_mul_scalar(v2, camera_speed);
            v2 = vector3_add(render_desc->camera, v1);
            render_desc->camera = v2;
        }
    }

    {
        render_desc->w = vector3_normal(render_desc->front);
        float3 vec;
        vec.a = render_desc->w.c;
        vec.b = 0.f;
        vec.c = -render_desc->w.a;
        render_desc->u = vector3_normal(vec);
        render_desc->v = vector3_cross(render_desc->w, render_desc->u);
    }
}

static inline void graphics_rotate_camera(Render_desc *restrict render_desc, int32_t window_center_x, int32_t window_center_y, float sensitivity)
{   
    POINT mouse_coords;
    ASSERT(GetCursorPos(&mouse_coords));
    ASSERT(SetCursorPos(window_center_x, window_center_y));

    float x = window_center_x - mouse_coords.x;
    float y = mouse_coords.y - window_center_y;
    ASSERT(render_desc->w.b > -1.f && render_desc->w.b < 1.f);
    int rnd = fegetround();
    fesetround(FE_TONEAREST);
    int8_t rw_y = (int8_t)lrintf(render_desc->w.b * 100.f);
    fesetround(rnd);
    if((y < 0 && rw_y >= 99) || (y > 0 && rw_y <= -99)) {
        mouse_coords.y = window_center_y;
        y = 0;
    }
    if(mouse_coords.x != window_center_x || mouse_coords.y != window_center_y) {
        float dist = sqrtf(x * x + y * y);
        float angle = (dist * sensitivity);
        float q_sin = sinf(angle);
        float q_cos = sqrtf(1 - q_sin * q_sin);

        float3 v1 = vector3_mul_scalar(render_desc->u, y);
        float3 v2 = vector3_mul_scalar(render_desc->v, -x);
        float3 v3 = vector3_add(v1, v2);
        float3 v_unit = vector3_normal(v3);

        float4 q = {.a = v_unit.a * q_sin, .b = v_unit.b * q_sin, .c = v_unit.c * q_sin, .d = q_cos};
        float4 qc = {.a = -q.a, .b = -q.b, .c = -q.c, .d = q.d};
        float4 interm = {
            .a = (q.b * render_desc->front.c) + (q.d * render_desc->front.a) - (q.c * render_desc->front.b),
            .b = (q.c * render_desc->front.a) + (q.d * render_desc->front.b) - (q.a * render_desc->front.c),
            .c = (q.a * render_desc->front.b) - (q.b * render_desc->front.a) + (q.d * render_desc->front.c),
            .d = -(q.a * render_desc->front.a) - (q.b * render_desc->front.b) - (q.c * render_desc->front.c)
        };
        float3 new_front = {
            .a = (interm.b * qc.c) + (interm.a * qc.d) - (interm.c * qc.b) + (interm.d * qc.a),
            .b = (interm.c * qc.a) - (interm.a * qc.c) + (interm.b * qc.d) + (interm.d * qc.b),
            .c = (interm.a * qc.b) - (interm.b * qc.a) + (interm.c * qc.d) + (interm.d * qc.c)
        };
        render_desc->front = new_front;
    }
}

static inline void graphics_draw(Render_desc *restrict render_desc, float(*restrict wvp_matrix)[4], ID3D11Buffer *constants_buf,
    ID3D11DeviceContext *device_context, ID3D11RenderTargetView *render_target_view, ID3D11DepthStencilView *depth_stencil_view, IDXGISwapChain *swap_chain)
{
    matrix4x4_transpose(render_desc->wvp_matrix, wvp_matrix);
    D3D11_MAPPED_SUBRESOURCE constants_buf_sub;
    memset(&constants_buf_sub, 0, sizeof(constants_buf_sub));
    ID3D11Resource *res;
    ASSERT_POSIX(ID3D11Buffer_QueryInterface(constants_buf, &IID_ID3D11Resource, (void **)&res));
    ASSERT_POSIX(ID3D11DeviceContext_Map(device_context, res, 0, D3D11_MAP_WRITE_DISCARD, 0, &constants_buf_sub));
    memcpy(constants_buf_sub.pData, &render_desc->wvp_matrix, sizeof(render_desc->wvp_matrix));
    ID3D11DeviceContext_Unmap(device_context, res, 0);
    ID3D11Resource_Release(res);

    static const float ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    ID3D11DeviceContext_ClearRenderTargetView(device_context, render_target_view, ClearColor);
    ID3D11DeviceContext_ClearDepthStencilView(device_context, depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 255);
    ID3D11DeviceContext_DrawIndexed(device_context, 14, 0, 0);
    ASSERT_POSIX(IDXGISwapChain_Present(swap_chain, 0, 0));
}
