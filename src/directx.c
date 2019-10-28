#define COBJMACROS
#define COM_NO_WINDOWS_H
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcommon.h>

static inline void create_vertex_shader(ID3D11Device *restrict device, const wchar_t *restrict filename, ID3D11VertexShader **restrict shader, 
	ID3D11InputLayout **restrict input_layout, D3D11_INPUT_ELEMENT_DESC *restrict layout_desc)
{
    HANDLE shader_file;
    ASSERT((shader_file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE);
    LARGE_INTEGER file_size;
    ASSERT(GetFileSizeEx(shader_file, &file_size));
    unsigned char *file_buf = page_alloc(STRING_SPACE, (uint32_t)file_size.QuadPart, 1);
	unsigned long bytes_read;
	ASSERT(ReadFile(shader_file, file_buf, (uint32_t)file_size.QuadPart, &bytes_read, NULL));
	ASSERT_POSIX(ID3D11Device_CreateVertexShader(device, file_buf, bytes_read, NULL, shader));
    ASSERT_POSIX(ID3D11Device_CreateInputLayout(device, layout_desc, 2, file_buf, bytes_read, input_layout));
    ASSERT(CloseHandle(shader_file));
    page_free(STRING_SPACE, (uint64_t)file_size.QuadPart, 1);
}

static inline void create_pixel_shader(ID3D11Device *restrict device, const wchar_t *restrict filename, 
	ID3D11PixelShader **restrict shader)
{
	HANDLE shader_file;
	ASSERT((shader_file = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE);
	LARGE_INTEGER file_size;
	ASSERT(GetFileSizeEx(shader_file, &file_size));
    unsigned char *file_buf = page_alloc(STRING_SPACE, (uint64_t)file_size.QuadPart, 1);
	unsigned long bytes_read;
	ASSERT(ReadFile(shader_file, file_buf, (uint32_t)file_size.QuadPart, &bytes_read, NULL));
	ASSERT_POSIX(ID3D11Device_CreatePixelShader(device, file_buf, bytes_read, NULL, shader));
	ASSERT(CloseHandle(shader_file));
    page_free(STRING_SPACE, (uint64_t)file_size.QuadPart, 1);
}
