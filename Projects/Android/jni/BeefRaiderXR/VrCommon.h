#if !defined(vrcommon_h)
#define vrcommon_h

#define PITCH               0       // up / down
#define YAW                 1       // left / right
#define ROLL                2       // fall over


struct cvar_t {
	const char* name;
	int integer;
	float   value;
};

typedef float vec2_t[2];
typedef float vec3_t[3];

#define VectorSubtract( a,b,c )   ( ( c )[0] = ( a )[0] - ( b )[0],( c )[1] = ( a )[1] - ( b )[1],( c )[2] = ( a )[2] - ( b )[2] )
#define VectorAdd( a,b,c )        ( ( c )[0] = ( a )[0] + ( b )[0],( c )[1] = ( a )[1] + ( b )[1],( c )[2] = ( a )[2] + ( b )[2] )
#define VectorClear( a )         ( ( a )[0] = 0,( a )[1] = 0,( a )[2] = 0 )
#define VectorCopy( a,b )         ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2] )
#define VectorCopy4(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define VectorScale( v, s, o )    ( ( o )[0] = ( v )[0] * ( s ),( o )[1] = ( v )[1] * ( s ),( o )[2] = ( v )[2] * ( s ) )
#define VectorMA( v, s, b, o )    ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ) )
#define VectorSet( v, x, y, z )       ( ( v )[0] = ( x ), ( v )[1] = ( y ), ( v )[2] = ( z ) )
float VectorNormalize( vec3_t vec );

int Sys_Milliseconds ();

#define CVAR_ARCHIVE 0
cvar_t *Cvar_Get(const char* name, const char* value, int type);
float Cvar_VariableValue(const char* name);

#ifdef _WIN32
#include "windows/TBXR_Common.h"
#else
#include "android/TBXR_Common.h"
#endif


extern long long global_time;

extern ovrInputStateTrackedRemote leftTrackedRemoteState_old;
extern ovrInputStateTrackedRemote leftTrackedRemoteState_new;
extern ovrTrackedController leftRemoteTracking_new;
extern ovrInputStateTrackedRemote rightTrackedRemoteState_old;
extern ovrInputStateTrackedRemote rightTrackedRemoteState_new;
extern ovrTrackedController rightRemoteTracking_new;

extern vec3_t hmdorientation_snap;

void QuatToYawPitchRoll(XrQuaternionf q, vec3_t rotation, vec3_t out);

#endif //vrcommon_h