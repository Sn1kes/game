typedef struct float3 {
	float a, b, c;
} float3;

typedef struct float4 {
	float a, b, c, d;
} float4;

static void matrix4x4_multiply(float out[restrict 4][4], float mat1[restrict 4][4], float mat2[restrict 4][4])
{
	__m128 b1, b2, b3, b4;

	b1 = _mm_load_ps(mat2[0]);
	b2 = _mm_load_ps(mat2[1]);
	b3 = _mm_load_ps(mat2[2]);
	b4 = _mm_load_ps(mat2[3]);

	for(int i = 0; i < 4; ++i) {
		__m128 a1, a2, a3, a4;

		a1 = _mm_load1_ps(&mat1[i][0]);
		a2 = _mm_load1_ps(&mat1[i][1]);
		a3 = _mm_load1_ps(&mat1[i][2]);
		a4 = _mm_load1_ps(&mat1[i][3]);

		a1 = _mm_mul_ps(a1, b1);
		a2 = _mm_mul_ps(a2, b2);
		a3 = _mm_mul_ps(a3, b3);
		a4 = _mm_mul_ps(a4, b4);

		a1 = _mm_add_ps(a1, a2);
		a3 = _mm_add_ps(a3, a4);
		a1 = _mm_add_ps(a1, a3);
		_mm_store_ps(out[i], a1);
	}
}

static void matrix4x4_transpose(float out[restrict 4][4], float in[restrict 4][4])
{
	__m128 r1, r2, r3, r4;
	__m128 t1, t2, t3, t4;
	r1 = _mm_load_ps(in[0]);
	r2 = _mm_load_ps(in[1]);
	r3 = _mm_load_ps(in[2]);
	r4 = _mm_load_ps(in[3]);

	t1 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(1, 0, 1, 0));
	t2 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(3, 2, 3, 2));
	t3 = _mm_shuffle_ps(r3, r4, _MM_SHUFFLE(1, 0, 1, 0));
	t4 = _mm_shuffle_ps(r3, r4, _MM_SHUFFLE(3, 2, 3, 2));

	r1 = _mm_shuffle_ps(t1, t3, _MM_SHUFFLE(2, 0, 2, 0));
	r2 = _mm_shuffle_ps(t1, t3, _MM_SHUFFLE(3, 1, 3, 1));
	r3 = _mm_shuffle_ps(t2, t4, _MM_SHUFFLE(2, 0, 2, 0));
	r4 = _mm_shuffle_ps(t2, t4, _MM_SHUFFLE(3, 1, 3, 1));

	_mm_store_ps(out[0], r1);
	_mm_store_ps(out[1], r2);
	_mm_store_ps(out[2], r3);
	_mm_store_ps(out[3], r4);
}

static float3 vector3_normal(float3 vec)
{
	ASSERT(vec.a != 0.f || vec.b != 0.f || vec.c != 0.f);
	float norm = 1.f / sqrtf((vec.a * vec.a) + (vec.b * vec.b) + (vec.c * vec.c));

	return (float3){.a = vec.a * norm, .b = vec.b * norm, .c = vec.c * norm};
}

static inline float vector3_dot(float3 vec1, float3 vec2)
{
	return vec1.a * vec2.a + vec1.b * vec2.b + vec1.c * vec2.c;
}

static float3 vector3_add(float3 vec1, float3 vec2)
{
	return (float3){.a = vec1.a + vec2.a, .b = vec1.b + vec2.b, .c = vec1.c + vec2.c};
}

static float3 vector3_sub(float3 vec1, float3 vec2)
{
	return (float3){.a = vec1.a - vec2.a, .b = vec1.b - vec2.b, .c = vec1.c - vec2.c};
}

static float3 vector3_mul_scalar(float3 vec, float scalar)
{
	return (float3){.a = vec.a * scalar, .b = vec.b * scalar, .c = vec.c * scalar};
}

static float3 vector3_cross(float3 vec1, float3 vec2)
{
	return (float3){.a = vec1.b * vec2.c - vec1.c * vec2.b,
					.b = vec1.c * vec2.a - vec1.a * vec2.c,
					.c = vec1.a * vec2.b - vec1.b * vec2.a
	};
}

/*void matrix_view_LH(float out[restrict 4][4], float* restrict camera, float* restrict target, float* restrict up)
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

static void matrix_projection_LH(float out[restrict 4][4], float fovy, float aspect, float zn, float zf)
{
	ASSERT(fovy > 0);
	float yy = tanf((float)M_PI_2 - fovy * 0.5f);
	float xx = yy / aspect;
	float quot = zf / (zf - zn);
	out[0][0] = xx;  	out[0][1] = 0.f; 	out[0][2] = 0.f; 			out[0][3] = 0.f;
	out[1][0] = 0.f; 	out[1][1] = yy;  	out[1][2] = 0.f; 			out[1][3] = 0.f;
	out[2][0] = 0.f; 	out[2][1] = 0.f; 	out[2][2] = quot; 			out[2][3] = 1.f;
	out[3][0] = 0.f;   	out[3][1] = 0.f;   	out[3][2] = -zn * quot;  	out[3][3] = 0.f;
}
