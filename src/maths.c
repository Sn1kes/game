typedef struct float3 {
	union {
		float arr[3];
		struct {
			float a, b, c;
		};
	};
} float3;

typedef struct float4 {
	union {
		float arr[4];
		struct {
			float a, b, c, d;
		};
		struct {
			float3 f123;
			float f4;
		};
	};
} float4;

typedef struct hva3 {
	__m128 a;
	__m128 b;
	__m128 c;
} hva3;

typedef struct hva4 {
	__m128 a;
	__m128 b;
	__m128 c;
	__m128 d;
} hva4;

__vectorcall static void matrix4x4_mul(float out[static restrict 4][4], const float mat1[static restrict 4][4], 
	const float mat2[static restrict 4][4])
{
	const __m128 a11 = _mm_set1_ps(mat1[0][0]);
	const __m128 a21 = _mm_set1_ps(mat1[0][1]);
	const __m128 a31 = _mm_set1_ps(mat1[0][2]);
	const __m128 a41 = _mm_set1_ps(mat1[0][3]);

	const __m128 a12 = _mm_set1_ps(mat1[1][0]);
	const __m128 a22 = _mm_set1_ps(mat1[1][1]);
	const __m128 a32 = _mm_set1_ps(mat1[1][2]);
	const __m128 a42 = _mm_set1_ps(mat1[1][3]);

	const __m128 a13 = _mm_set1_ps(mat1[2][0]);
	const __m128 a23 = _mm_set1_ps(mat1[2][1]);
	const __m128 a33 = _mm_set1_ps(mat1[2][2]);
	const __m128 a43 = _mm_set1_ps(mat1[2][3]);

	const __m128 a14 = _mm_set1_ps(mat1[3][0]);
	const __m128 a24 = _mm_set1_ps(mat1[3][1]);
	const __m128 a34 = _mm_set1_ps(mat1[3][2]);
	const __m128 a44 = _mm_set1_ps(mat1[3][3]);

	const __m128 b1 = _mm_load_ps(mat2[0]);
	const __m128 b2 = _mm_load_ps(mat2[1]);
	const __m128 b3 = _mm_load_ps(mat2[2]);
	const __m128 b4 = _mm_load_ps(mat2[3]);

	const __m128 c11 = _mm_mul_ps(a11, b1);
	const __m128 c21 = _mm_mul_ps(a21, b2);
	const __m128 c31 = _mm_mul_ps(a31, b3);
	const __m128 c41 = _mm_mul_ps(a41, b4);
	const __m128 d11 = _mm_add_ps(c11, c21);
	const __m128 d21 = _mm_add_ps(c31, c41);
	const __m128 df1 = _mm_add_ps(d11, d21);

	const __m128 c12 = _mm_mul_ps(a12, b1);
	const __m128 c22 = _mm_mul_ps(a22, b2);
	const __m128 c32 = _mm_mul_ps(a32, b3);
	const __m128 c42 = _mm_mul_ps(a42, b4);
	const __m128 d12 = _mm_add_ps(c12, c22);
	const __m128 d22 = _mm_add_ps(c32, c42);
	const __m128 df2 = _mm_add_ps(d12, d22);

	const __m128 c13 = _mm_mul_ps(a13, b1);
	const __m128 c23 = _mm_mul_ps(a23, b2);
	const __m128 c33 = _mm_mul_ps(a33, b3);
	const __m128 c43 = _mm_mul_ps(a43, b4);
	const __m128 d13 = _mm_add_ps(c13, c23);
	const __m128 d23 = _mm_add_ps(c33, c43);
	const __m128 df3 = _mm_add_ps(d13, d23);

	const __m128 c14 = _mm_mul_ps(a14, b1);
	const __m128 c24 = _mm_mul_ps(a24, b2);
	const __m128 c34 = _mm_mul_ps(a34, b3);
	const __m128 c44 = _mm_mul_ps(a44, b4);
	const __m128 d14 = _mm_add_ps(c14, c24);
	const __m128 d24 = _mm_add_ps(c34, c44);
	const __m128 df4 = _mm_add_ps(d14, d24);
	
	_mm_store_ps(out[0], df1);
	_mm_store_ps(out[1], df2);
	_mm_store_ps(out[2], df3);
	_mm_store_ps(out[3], df4);
}

__vectorcall static __m128 matrix4x4_vector4_mul(const hva4 mat, const __m128 vec)
{

}

__vectorcall static hva4 matrix4x4_transpose(const hva4 in)
{
	const __m128 t1 = _mm_unpacklo_ps(in.x, in.z);
	const __m128 t2 = _mm_unpackhi_ps(in.x, in.z);
	const __m128 t3 = _mm_unpacklo_ps(in.y, in.w);
	const __m128 t4 = _mm_unpackhi_ps(in.y, in.w);

	const __m128 o1 = _mm_unpacklo_ps(t1, t3);
	const __m128 o2 = _mm_unpackhi_ps(t1, t3);
	const __m128 o3 = _mm_unpacklo_ps(t2, t4);
	const __m128 o4 = _mm_unpackhi_ps(t2, t4);
	
	const hva4 out = {
		.a = o1,
		.b = o2,
		.c = o3,
		.d = o4
	};

	return out;
}

__vectorcall static float3 vector3_normal(const float3 vec)
{
	ASSERT(fabsf(vec.a + vec.b + vec.c) > 0.f);
	const float norm = 1.f / sqrtf((vec.a * vec.a) + (vec.b * vec.b) + (vec.c * vec.c));
	return (float3){.a = vec.a * norm, .b = vec.b * norm, .c = vec.c * norm};
}

__vectorcall static float vector3_dot(const float3 vec1, const float3 vec2)
{
	return vec1.a * vec2.a + vec1.b * vec2.b + vec1.c * vec2.c;
}

__vectorcall static float vector4_dot(const float4 vec1, const float4 vec2)
{
	return vec1.a * vec2.a + vec1.b * vec2.b + vec1.c * vec2.c + vec1.d * vec2.d;
}

__vectorcall static __m128 vector3_dot_4n(__m128 *out, hva3 *const restrict vec1, 
	hva3 *const restrict vec2, const uint32_t n)
{
	for(int i = 0; i < n; ++i)
	const __m128 o1 = _mm_mul_ps(vec1.a, vec2.a);
	const __m128 o2 = _mm_mul_ps(vec1.b, vec2.b);
	const __m128 o3 = _mm_mul_ps(vec1.c, vec2.c);
	const __m128 o4 = _mm_add_ps(o1, o2);
	const __m128 out = _mm_add_ps(o4, o3);
}

__vectorcall static __m128 vector4_dot_4n(__m128 *out, hva4 *const restrict vec1, 
	hva4 *const restrict vec2, const uint32_t n)
{
	for(int i = 0; i < n; ++i) {
		const hva4 vec1_i = vec1[i];
		const hva4 vec2_i = vec2[i];
		const __m128 o1 = _mm_mul_ps(vec1.a, vec2.a);
		const __m128 o2 = _mm_mul_ps(vec1.b, vec2.b);
		const __m128 o3 = _mm_mul_ps(vec1.c, vec2.c);
		const __m128 o4 = _mm_mul_ps(vec1.d, vec2.d);
		const __m128 o5 = _mm_add_ps(o1, o2);
		const __m128 o6 = _mm_add_ps(o3, o4);
		const __m128 out = _mm_add_ps(o5, o6);
	}
}

__vectorcall static float3 vector3_add(const float3 vec1, const float3 vec2)
{
	return (float3){.a = vec1.a + vec2.a, .b = vec1.b + vec2.b, .c = vec1.c + vec2.c};
}

__vectorcall static float3 vector3_sub(const float3 vec1, const float3 vec2)
{
	return (float3){.a = vec1.a - vec2.a, .b = vec1.b - vec2.b, .c = vec1.c - vec2.c};
}

__vectorcall static float3 vector3_mul_scalar(const float3 vec, const float scalar)
{
	return (float3){.a = vec.a * scalar, .b = vec.b * scalar, .c = vec.c * scalar};
}

__vectorcall static float3 vector3_cross(const float3 vec1, const float3 vec2)
{
	return (float3){.a = vec1.b * vec2.c - vec1.c * vec2.b,
					.b = vec1.c * vec2.a - vec1.a * vec2.c,
					.c = vec1.a * vec2.b - vec1.b * vec2.a
	};
}

/*__vectorcall void matrix_view_LH(float out[restrict 4][4], float* restrict camera, float* restrict target, float* restrict up)
{	
	vector3_sub(line->vec1, target, camera);
	vector3_normal(line->zz, line->vec1);
	vector3_cross(line->vec3, up, line->zz);
	vector3_normal(line->xx, line->vec3);
	vector3_cross(line->yy, line->zz, line->xx);

	out[0][0] = line->xx[0]; 					  out[0][1] = line->yy[0]; 					  out[0][2] = line->zz[0]; 					  out[0][3] = 0;
	out[1][0] = line->xx[1]; 					  out[1][1] = line->yy[1]; 					  out[1][2] = line->zz[1]; 					  out[1][3] = 0;
	out[2][0] = line->xx[2]; 					  out[2][1] = line->yy[2]; 					  out[2][2] = line->zz[2]; 					  out[2][3] = 0;
	out[3][0] = -vector3_dot(line->xx, camera);   out[3][1] = -vector3_dot(line->yy, camera); out[3][2] = -vector3_dot(line->zz, camera); out[3][3] = 1;
}*/

__vectorcall static inline void matrix_projection_LH(float out[static restrict 4][4], const float fovy, 
	const float aspect, const float zn, const float zf)
{
	ASSERT(fovy > 0.f);
	const float yy = tanf((float)M_PI_2 - fovy * 0.5f);
	const float xx = yy / aspect;
	const float quot = zf / (zf - zn);
	out[0][0] = xx;  	out[0][1] = 0.f; 	out[0][2] = 0.f; 			out[0][3] = 0.f;
	out[1][0] = 0.f; 	out[1][1] = yy;  	out[1][2] = 0.f; 			out[1][3] = 0.f;
	out[2][0] = 0.f; 	out[2][1] = 0.f; 	out[2][2] = quot; 			out[2][3] = 1.f;
	out[3][0] = 0.f;   	out[3][1] = 0.f;   	out[3][2] = -zn * quot;  	out[3][3] = 0.f;
}
