enum {
	MATRIX4x4_MULTIPLY,
	MATRIX4x4_TRANSPOSE,
	MATRIX_PROJECTION_LH,
	ID3D11DEVICECONTEXT_DRAWINDEXED,
	ID3D11DEVICECONTEXT_CLEARRENDERTARGETVIEW,
	ID3D11DEVICECONTEXT_CLEARDEPTHSTENCILVIEW,
	IDXGISWAPCHAIN_PRESENT,
	ID3D11DEVICECONTEXT_MAP,
	ID3D11DEVICECONTEXT_UNMAP,
	GRAPHICS_ROTATE_CAMERA,
	GRAPHICS_MULTIPLY_MATRICES,
	GRAPHICS_DRAW,
	GRAPHICS_MOVE_CAMERA
};

static inline void f1(void *in)
{
	struct {
		int func;
		float (*restrict out)[4];
		float (*restrict mat1)[4];
		float (*restrict mat2)[4];
	} *args = in;
	matrix4x4_multiply(args->out, args->mat1, args->mat2);
}

static inline void f2(void *in)
{
	struct {
		int func;
		float (*restrict out)[4];
		float (*restrict in)[4];
	} *args = in;
	matrix4x4_transpose(args->out, args->in);
}

static inline void f3(void *in)
{
	struct {
		int func;
		float (*restrict out)[4];
		float fovy;
		float aspect;
		float zn;
		float zf;
	} *args = in;
	matrix_projection_LH(args->out, args->fovy, args->aspect, args->zn, args->zf);
}

static inline void f4(void *in)
{
	struct {
        int func;
        ID3D11DeviceContext *DeviceContext;
        UINT IndexCount;
        UINT StartIndexLocation;
        INT  BaseVertexLocation;
    } *args = in;
	ID3D11DeviceContext_DrawIndexed(args->DeviceContext, args->IndexCount, args->StartIndexLocation, args->BaseVertexLocation);
}

static inline void f5(void *in)
{
	struct {
        int func;
        ID3D11DeviceContext *DeviceContext;
        ID3D11RenderTargetView *pRenderTargetView;
 	 	const FLOAT         ColorRGBA[4];
    } *args = in;
	ID3D11DeviceContext_ClearRenderTargetView(args->DeviceContext, args->pRenderTargetView, args->ColorRGBA);
}

static inline void f6(void *in)
{
	struct {
        int func;
        ID3D11DeviceContext    *DeviceContext;
        ID3D11DepthStencilView *pDepthStencilView;
		UINT                    ClearFlags;
		FLOAT                   Depth;
		UINT8                   Stencil;
    } *args = in;
	ID3D11DeviceContext_ClearDepthStencilView(args->DeviceContext, args->pDepthStencilView, args->ClearFlags, args->Depth, args->Stencil);
}

static inline void f7(void *in)
{
	struct {
        int func;
        IDXGISwapChain *SwapChain;
        UINT SyncInterval;
		UINT Flags;
    } *args = in;
	ASSERT_POSIX(IDXGISwapChain_Present(args->SwapChain, args->SyncInterval, args->Flags));
}

static inline void f8(void *in)
{
	struct {
        int func;
        ID3D11DeviceContext      *DeviceContext;
		ID3D11Resource           *pResource;
		UINT                     Subresource;
		D3D11_MAP                MapType;
		UINT                     MapFlags;
		D3D11_MAPPED_SUBRESOURCE *pMappedResource;
    } *args = in;
	ASSERT_POSIX(ID3D11DeviceContext_Map(args->DeviceContext, args->pResource, args->Subresource, args->MapType, args->MapFlags, args->pMappedResource));
}

static inline void f9(void *in)
{
	struct {
        int func;
        ID3D11DeviceContext *DeviceContext;
        ID3D11Resource      *pResource;
		UINT                Subresource;
    } *args = in;
	ID3D11DeviceContext_Unmap(args->DeviceContext, args->pResource, args->Subresource);
}

static inline void f10(void *in)
{
	struct {
        int func;
        Render_desc *render_desc;
        int32_t window_center_x;
        int32_t window_center_y;
        float sensitivity;
    } *args = in;
	graphics_rotate_camera(args->render_desc, args->window_center_x, args->window_center_y, args->sensitivity);
}

static inline void f11(void *in)
{
	struct {
        int func;
        float (*restrict wvp_matrix)[4];
        Render_desc* render_desc;
        uint16_t scroll;
    } *args = in;
	graphics_multiply_matrices(args->wvp_matrix, args->render_desc, args->scroll);
}

static inline void f12(void *in)
{
	struct {
        int func;
        Render_desc *restrict render_desc;
        float (*restrict wvp_matrix)[4];
        ID3D11Buffer *constants_buf;
        ID3D11DeviceContext *device_context; 
    	ID3D11RenderTargetView *render_target_view;
    	ID3D11DepthStencilView *depth_stencil_view;
    	IDXGISwapChain *swap_chain;
    } *args = in;
	graphics_draw(args->render_desc, args->wvp_matrix, args->constants_buf, args->device_context, args->render_target_view, args->depth_stencil_view, args->swap_chain);
}

static inline void f13(void *in)
{
	struct {
		int func;
		Render_desc *render_desc;
		float camera_speed;
	} *args = in;
	graphics_move_camera(args->render_desc, args->camera_speed);
}
