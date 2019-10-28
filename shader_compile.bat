@echo off
fxc /T vs_4_1 /E main /O3 hlsl/vertex_cube_shader.hlsl /Fo hlsl/vertex_cube_shader.cso
fxc /T ps_4_1 /E main /O3 hlsl/pixel_cube_shader.hlsl /Fo hlsl/pixel_cube_shader.cso
pause