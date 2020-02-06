all: ../hlsl/vertex_shader.cso ../hlsl/pixel_shader.cso main.dump
../hlsl/vertex_shader.cso: ../src/hlsl/vertex_shader.hlsl
	../fxc.exe /T vs_4_1 /E main /O3 /Zi ../src/hlsl/vertex_shader.hlsl /Fc ../hlsl/vertex_shader.asm /Fo ../hlsl/vertex_shader.cso
../hlsl/pixel_shader.cso: ../src/hlsl/pixel_shader.hlsl
	../fxc.exe /T ps_4_1 /E main /O3 /Zi ../src/hlsl/pixel_shader.hlsl /Fc ../hlsl/pixel_shader.asm /Fo ../hlsl/pixel_shader.cso
main.dump: main.exe
	dumpbin /DISASM $^ /OUT:$@