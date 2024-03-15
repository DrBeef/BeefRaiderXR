#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h>	// for native window JNI
#include <android/input.h>
#include <cstring>
#include <string>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "argtable3.h"
#include "VrInput.h"
#include "VrCvars.h"

#include <game.h>

//#define ENABLE_GL_DEBUG
#define ENABLE_GL_DEBUG_VERBOSE 1

//Let's go to the maximum!
extern int NUM_MULTI_SAMPLES;
extern float SS_MULTIPLIER    ;


/* global arg_xxx structs */
struct arg_dbl *ss;
struct arg_lit *cheats;
struct arg_end *end;

char **argv;
int argc=0;

bool cheatsEnabled = false;
bool isDemo = false;

/*
================================================================================

OpenLara Stuff

================================================================================
*/

void VR_Init();
void osToggleVR(bool enable)
{
    //VR is always enabled
    VR_Init();

    Input::hmd.ready = true;

}

int osStartTime = 0;

int   osGetTimeMS()
{
    return int(Sys_Milliseconds ()) - osStartTime;
}

bool  osJoyReady(int index)
{
    return true;
}

void  osJoyVibrate(int index, float L, float R)
{
    if (R > 0.f)
        TBXR_Vibrate(100, 1, R);

    if (L > 0.f)
        TBXR_Vibrate(100, 2, L);
}

void osBeforeLoadNextLevel()
{
    //Level reset
    Input::hmd.extrarot = 0.f;
}

// sound
#define          SND_FRAMES 1176

Sound::Frame     sndBuf[2][SND_FRAMES];
int              sndBufIndex;

SLObjectItf      sndEngine;
SLObjectItf      sndOutput;
SLObjectItf      sndPlayer;
SLBufferQueueItf sndQueue = NULL;
SLPlayItf        sndPlay  = NULL;

void sndFill(SLBufferQueueItf bq, void *context) {
    if (!sndQueue) return;
    Sound::fill(sndBuf[sndBufIndex ^= 1], SND_FRAMES);
    (*sndQueue)->Enqueue(sndQueue, sndBuf[sndBufIndex], SND_FRAMES * sizeof(Sound::Frame));
}

void sndSetState(bool active) {
    if (!sndPlay) return;
    (*sndPlay)->SetPlayState(sndPlay, active ? SL_PLAYSTATE_PLAYING : SL_PLAYSTATE_PAUSED);
}

void sndInit() {
    slCreateEngine(&sndEngine, 0, NULL, 0, NULL, NULL);
    (*sndEngine)->Realize(sndEngine, SL_BOOLEAN_FALSE);

    SLEngineItf engine;

    (*sndEngine)->GetInterface(sndEngine, SL_IID_ENGINE, &engine);
    (*engine)->CreateOutputMix(engine, &sndOutput, 0, NULL, NULL);
    (*sndOutput)->Realize(sndOutput, SL_BOOLEAN_FALSE);

    SLDataFormat_PCM bufFormat;
    bufFormat.formatType    = SL_DATAFORMAT_PCM;
    bufFormat.numChannels   = 2;
    bufFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
    bufFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    bufFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    bufFormat.channelMask   = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT ;
    bufFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;

    SLDataLocator_AndroidSimpleBufferQueue bufLocator;
    bufLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    bufLocator.numBuffers  = 2;

    SLDataLocator_OutputMix snkLocator;
    snkLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    snkLocator.outputMix   = sndOutput;

    SLDataSource audioSrc;
    audioSrc.pLocator = &bufLocator;
    audioSrc.pFormat  = &bufFormat;

    SLDataSink audioSnk;
    audioSnk.pLocator = &snkLocator;
    audioSnk.pFormat  = NULL;

    SLInterfaceID audioInt[] = { SL_IID_BUFFERQUEUE, SL_IID_PLAY  };
    SLboolean     audioReq[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    (*engine)->CreateAudioPlayer(engine, &sndPlayer, &audioSrc, &audioSnk, 2, audioInt, audioReq);
    (*sndPlayer)->Realize(sndPlayer, SL_BOOLEAN_FALSE);
    (*sndPlayer)->GetInterface(sndPlayer, SL_IID_BUFFERQUEUE, &sndQueue);
    (*sndPlayer)->GetInterface(sndPlayer, SL_IID_PLAY, &sndPlay);
    (*sndQueue)->RegisterCallback(sndQueue, sndFill, NULL);

    sndBufIndex = 1;
    sndFill(sndQueue, NULL);
    sndFill(sndQueue, NULL);
}

void sndFree() {
    if (sndPlayer) (*sndPlayer)->Destroy(sndPlayer);
    if (sndOutput) (*sndOutput)->Destroy(sndOutput);
    if (sndEngine) (*sndEngine)->Destroy(sndEngine);
    sndPlayer = sndOutput = sndEngine = NULL;
    sndQueue = NULL;
    sndPlay  = NULL;
}

/*
================================================================================

BeefRaiderXR Stuff

================================================================================
*/

bool VR_UseScreenLayer()
{
	vr.using_screen_layer = inventory->video;

	return vr.using_screen_layer;
}

float VR_GetScreenLayerDistance()
{
	return (2.0f + vr_screen_dist->value);
}

static void UnEscapeQuotes( char *arg )
{
	char *last = NULL;
	while( *arg ) {
		if( *arg == '"' && *last == '\\' ) {
			char *c_curr = arg;
			char *c_last = last;
			while( *c_curr ) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;
	argc = last_argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		while ( isspace(*bufp) ) {
			++bufp;
		}
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			while ( *bufp && ( *bufp != '"' || *lastp == '\\' ) ) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
		if( argv && last_argc != argc ) {
			UnEscapeQuotes( argv[last_argc] );
		}
		last_argc = argc;
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}


void VR_SetHMDOrientation(float pitch, float yaw, float roll)
{
	//Orientation
	VectorSet(vr.hmdorientation, pitch, yaw, roll);
	VectorSubtract(vr.hmdorientation_last, vr.hmdorientation, vr.hmdorientation_delta);

	//Keep this for our records
	VectorCopy(vr.hmdorientation, vr.hmdorientation_last);

	if (!vr.third_person && !vr.remote_npc && !vr.remote_turret && !vr.cgzoommode)
	{
		VectorCopy(vr.hmdorientation, vr.hmdorientation_first);
	}

	if (!vr.remote_turret && !vr.cgzoommode)
	{
		VectorCopy(vr.weaponangles[ANGLES_ADJUSTED], vr.weaponangles_first[ANGLES_ADJUSTED]);
	}

	// View yaw delta
	float clientview_yaw = vr.clientviewangles[YAW] - vr.hmdorientation[YAW];
	vr.clientview_yaw_delta = vr.clientview_yaw_last - clientview_yaw;
	vr.clientview_yaw_last = clientview_yaw;

	// Max-height is set only once on start, or after re-calibration
	// (ignore too low value which is sometimes provided on start)
	if (!vr.maxHeight || vr.maxHeight < 1.0) {
		vr.maxHeight = vr.hmdposition[1];
	}

	//GB Instantiate initial velocity
	if(!vr.tempWeaponVelocity)
	{
		vr.tempWeaponVelocity = 400.0f;
	}

	vr.curHeight = vr.hmdposition[1];
}

void VR_SetHMDPosition(float x, float y, float z )
{
	static bool s_useScreen = false;

	VectorSet(vr.hmdposition, x, y, z);

	//Can be set elsewhere
	vr.take_snap |= s_useScreen != VR_UseScreenLayer();
	if (vr.take_snap)
    {
		s_useScreen = VR_UseScreenLayer();

		//Record player position on transition
		VectorSet(vr.hmdposition_snap, x, y, z);
		VectorCopy(vr.hmdorientation, vr.hmdorientation_snap);
		if (vr.cin_camera)
		{
			//Reset snap turn too if in a cinematic
			Input::hmd.extrarot = 0;
		}
		vr.take_snap = false;
    }

	VectorSubtract(vr.hmdposition, vr.hmdposition_snap, vr.hmdposition_offset);

	//Position
	VectorSubtract(vr.hmdposition_last, vr.hmdposition, vr.hmdposition_delta);

	//Keep this for our records
	VectorCopy(vr.hmdposition, vr.hmdposition_last);
}

static bool forceUpdatePose = false;

void VR_Init()
{
	//Initialise all our variables
	remote_movementSideways = 0.0f;
	remote_movementForward = 0.0f;
	remote_movementUp = 0.0f;
	positional_movementSideways = 0.0f;
	positional_movementForward = 0.0f;
	Input::hmd.extrarot = 0.0f;
	vr.immersive_cinematics = true;
	vr.move_speed = 1; // Default to full speed now

	//init randomiser
	srand(time(NULL));

	//Create Cvars
	vr_turn_mode = Cvar_Get( "vr_turn_mode", "0", CVAR_ARCHIVE); // 0 = snap, 1 = smooth
	vr_turn_angle = Cvar_Get( "vr_turn_angle", "45", CVAR_ARCHIVE);
	vr_positional_factor = Cvar_Get( "vr_positional_factor", "12", CVAR_ARCHIVE);
    vr_walkdirection = Cvar_Get( "vr_walkdirection", "1", CVAR_ARCHIVE);
	vr_weapon_pitchadjust = Cvar_Get( "vr_weapon_pitchadjust", "-45.0", CVAR_ARCHIVE);
    vr_virtual_stock = Cvar_Get( "vr_virtual_stock", "0", CVAR_ARCHIVE);

    //Defaults
	vr_control_scheme = Cvar_Get( "vr_control_scheme", "0", CVAR_ARCHIVE);
	vr_switch_sticks = Cvar_Get( "vr_switch_sticks", "0", CVAR_ARCHIVE);

	vr_immersive_cinematics = Cvar_Get("vr_immersive_cinematics", "1", CVAR_ARCHIVE);
	vr_screen_dist = Cvar_Get( "vr_screen_dist", "3.5", CVAR_ARCHIVE);
	vr_weapon_velocity_trigger = Cvar_Get( "vr_weapon_velocity_trigger", "2.0", CVAR_ARCHIVE);
	vr_scope_engage_distance = Cvar_Get( "vr_scope_engage_distance", "0.25", CVAR_ARCHIVE);
	vr_force_velocity_trigger = Cvar_Get( "vr_force_velocity_trigger", "2.09", CVAR_ARCHIVE);
	vr_force_distance_trigger = Cvar_Get( "vr_force_distance_trigger", "0.15", CVAR_ARCHIVE);
    vr_two_handed_weapons = Cvar_Get ("vr_two_handed_weapons", "1", CVAR_ARCHIVE);
	vr_force_motion_controlled = Cvar_Get ("vr_force_motion_controlled", "1", CVAR_ARCHIVE);
	vr_force_motion_push = Cvar_Get ("vr_force_motion_push", "3", CVAR_ARCHIVE);
	vr_force_motion_pull = Cvar_Get ("vr_force_motion_pull", "4", CVAR_ARCHIVE);
	vr_motion_enable_saber = Cvar_Get ("vr_motion_enable_saber", "0", CVAR_ARCHIVE);
	vr_always_run = Cvar_Get ("vr_always_run", "1", CVAR_ARCHIVE);
	vr_crouch_toggle = Cvar_Get ("vr_crouch_toggle", "0", CVAR_ARCHIVE);
	vr_irl_crouch_enabled = Cvar_Get ("vr_irl_crouch_enabled", "0", CVAR_ARCHIVE);
	vr_irl_crouch_to_stand_ratio = Cvar_Get ("vr_irl_crouch_to_stand_ratio", "0.65", CVAR_ARCHIVE);
	vr_saber_block_debounce_time = Cvar_Get ("vr_saber_block_debounce_time", "200", CVAR_ARCHIVE);
	vr_haptic_intensity = Cvar_Get ("vr_haptic_intensity", "1.0", CVAR_ARCHIVE);
	vr_comfort_vignette = Cvar_Get ("vr_comfort_vignette", "0.0", CVAR_ARCHIVE);
	vr_saber_3rdperson_mode = Cvar_Get ("vr_saber_3rdperson_mode", "1", CVAR_ARCHIVE);
	vr_vehicle_use_hmd_direction = Cvar_Get ("vr_vehicle_use_hmd_direction", "0", CVAR_ARCHIVE);
	vr_vehicle_use_3rd_person = Cvar_Get ("vr_vehicle_use_3rd_person", "0", CVAR_ARCHIVE);
	vr_vehicle_use_controller_for_speed = Cvar_Get ("vr_vehicle_use_controller_for_speed", "1", CVAR_ARCHIVE);
	vr_gesture_triggered_use = Cvar_Get ("vr_gesture_triggered_use", "1", CVAR_ARCHIVE);
	vr_use_gesture_boundary = Cvar_Get ("vr_use_gesture_boundary", "0.35", CVAR_ARCHIVE);
	vr_align_weapons = Cvar_Get ("vr_align_weapons", "0", CVAR_ARCHIVE);
	vr_refresh = Cvar_Get ("vr_refresh", "72", CVAR_ARCHIVE);
	vr_super_sampling = Cvar_Get ("vr_super_sampling", "1.0", CVAR_ARCHIVE);

    vr.menu_right_handed = vr_control_scheme->integer == 0;

    Cvar_Get("openXRHMD", gAppState.OpenXRHMD, CVAR_ARCHIVE);


    int eyeW, eyeH;
    TBXR_GetScreenRes(&eyeW, &eyeH);

    Core::width  = eyeW;
    Core::height = eyeH;

    Core::eyeTex[0] = new Texture(eyeW, eyeH, 1, TexFormat::FMT_RGBA, OPT_TARGET);
    Core::eyeTex[1] = new Texture(eyeW, eyeH, 1, TexFormat::FMT_RGBA, OPT_TARGET);

    forceUpdatePose = true;
}



bool VR_GetVRProjection(int eye, float zNear, float zFar, float zZoomX, float zZoomY, float* projection)
{
    XrFovf fov = gAppState.Views[eye].fov;
    fov.angleLeft = atanf((tanf(fov.angleLeft) / zZoomX));
    fov.angleRight = atanf((tanf(fov.angleRight) / zZoomX));
    fov.angleUp = atanf((tanf(fov.angleUp) / zZoomY));
    fov.angleDown = atanf((tanf(fov.angleDown) / zZoomY));

    XrMatrix4x4f_CreateProjectionFov(
            (XrMatrix4x4f*)projection, GRAPHICS_OPENGL,
            fov, zNear, zFar);

    return true;
}
/*
mat4 convToMat4(const XrMatrix4x4f &m) {
    return mat4(m.m[0], m.m[4], m.m[8], m.m[12],
                m.m[1], m.m[5], m.m[9], m.m[13],
                m.m[2], m.m[6], m.m[10], m.m[14],
                m.m[3], m.m[7], m.m[11], m.m[15]);
}
*/
mat4 convToMat4(const XrMatrix4x4f &m) {
    return mat4(m.m[0], m.m[1], m.m[2], m.m[3],
                m.m[4], m.m[5], m.m[6], m.m[7],
                m.m[8], m.m[9], m.m[10], m.m[11],
                m.m[12], m.m[13], m.m[14], m.m[15]);
}

void VR_prepareEyeBuffer(int eye )
{
    //Bit of a fiddle really
    ovrFramebuffer* frameBuffer = &(gAppState.Renderer.FrameBuffer[eye]);
    Core::eyeTex[eye]->ID = frameBuffer->ColorSwapChainImage[frameBuffer->TextureSwapChainIndex].image;
}

void * AppThreadFunction(void * parm ) {
	gAppThread = (ovrAppThread *) parm;

	java.Vm = gAppThread->JavaVm;
	java.Vm->AttachCurrentThread(&java.Env, NULL);
	java.ActivityObject = gAppThread->ActivityObject;

	jclass cls = java.Env->GetObjectClass(java.ActivityObject);

	// Note that AttachCurrentThread will reset the thread name.
	prctl(PR_SET_NAME, (long) "AppThreadFunction", 0, 0, 0);

	//Set device defaults
	if (SS_MULTIPLIER == 0.0f)
	{
		SS_MULTIPLIER = 1.2f;
	}
	else if (SS_MULTIPLIER > 1.5f)
	{
		SS_MULTIPLIER = 1.5f;
	}

	gAppState.MainThreadTid = gettid();

    sndInit();

	TBXR_InitialiseOpenXR();

	TBXR_EnterVR();
	TBXR_InitRenderer();
	TBXR_InitActions();

    TBXR_WaitForSessionActive();

    std::string levelName;
    if (isDemo)
    {
        chdir("/sdcard/BeefRaiderXR/DATA");
        levelName = "LEVEL2.PHD";
    }
    else
    {
        chdir("/sdcard/BeefRaiderXR");
    }

    osStartTime = Core::getTime();

    sndSetState(true);

    if (argc > 1 && !isDemo)
    {
        for (int arg = 1; arg < argc; ++arg)
        {
            if (!strstr(argv[arg], "--"))
            {
                levelName = argv[arg];
                break;
            }
        }
    }

	//start
    Game::init(levelName.length() > 0 ? levelName.c_str() : NULL);

    while (!Core::isQuit) {
        {
            TBXR_FrameSetup();
            if (Game::update()) {
                Game::render();

                Core::waitVBlank();

                TBXR_submitFrame();
            }
        }
    };


    Game::deinit();

	return NULL;
}

int VR_SetRefreshRate(int refreshRate)
{
	if (strstr(gAppState.OpenXRHMD, "meta") != NULL)
	{
		OXR(gAppState.pfnRequestDisplayRefreshRate(gAppState.Session, (float) refreshRate));
		return refreshRate;
	}

	return 0;
}

//All the stuff we want to do each frame specifically for this game
void VR_FrameSetup()
{
    static float refresh = 0;

    mat4 pL, pR;
    VR_GetVRProjection(0, 8.0f, 45.0f * 1024.0f, 1.f, 1.f, (float *) &pL.e00);
    VR_GetVRProjection(1, 8.0f, 45.0f * 1024.0f, 1.f, 1.f, (float *) &pR.e00);

    vec3 vrPosition(gAppState.xfStageFromHead.position.x,
                    gAppState.xfStageFromHead.position.y,
                    gAppState.xfStageFromHead.position.z);
    quat vrOrientation(gAppState.xfStageFromHead.orientation.x,
                       gAppState.xfStageFromHead.orientation.y,
                       gAppState.xfStageFromHead.orientation.z,
                       gAppState.xfStageFromHead.orientation.w);

    mat4 snapTurnMat;
    snapTurnMat.identity();
    snapTurnMat.rotateY(DEG2RAD * Input::hmd.extrarot);

    mat4 head = snapTurnMat * mat4(vrOrientation, vec3(0));

    if (refresh != vr_refresh->value)
    {
        refresh = vr_refresh->value;
        VR_SetRefreshRate(vr_refresh->value);
    }

    if (leftTrackedRemoteState_new.Buttons & xrButton_LThumb)
    {
        forceUpdatePose = true;
    }

    if (Input::hmd.zero.x == INF || forceUpdatePose) {
        Input::hmd.zero = vrPosition;
        Input::hmd.head = head;
        Input::hmd.head.setPos(vrPosition);

        forceUpdatePose = false;
    }

    vrPosition = vrPosition.rotateY(-DEG2RAD * Input::hmd.extrarot);

    Input::hmd.body = head; // direction body is facing
    vec3 zero = Input::hmd.zero;
    zero = zero.rotateY(-DEG2RAD * Input::hmd.extrarot);
    Input::hmd.head.setPos(vrPosition - zero);

    //Left eye
    mat4 vL = head;
    vL.setPos((Input::hmd.head.getPos() + (head.right().xyz() * (-0.065f / 2.f))) * ONE_METER);

    //Right eye
    mat4 vR = head;
    vR.setPos((Input::hmd.head.getPos() + (head.right().xyz() * (0.065f / 2.f))) * ONE_METER);

    Input::hmd.setView(pL, pR, vL, vR);
}

extern "C" {
void jni_haptic_event(const char *event, int position, int flags, int intensity, float angle, float yHeight);
void jni_haptic_updateevent(const char *event, int intensity, float angle);
void jni_haptic_stopevent(const char *event);
void jni_haptic_endframe();
void jni_haptic_enable();
void jni_haptic_disable();
};

void VR_ExternalHapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight )
{
	jni_haptic_event(event, position, flags, intensity, angle, yHeight);
}

void VR_HapticUpdateEvent(const char* event, int intensity, float angle )
{
	jni_haptic_updateevent(event, intensity, angle);
}

void VR_HapticEndFrame()
{
	jni_haptic_endframe();
}

void VR_HapticStopEvent(const char* event)
{
	jni_haptic_stopevent(event);
}

void VR_HapticEnable()
{
	static bool firstTime = true;
	if (firstTime) {
		jni_haptic_enable();
		firstTime = false;
		jni_haptic_event("fire_pistol", 0, 0, 100, 0, 0);
	}
}

void VR_HapticDisable()
{
	jni_haptic_disable();
}

/*
 *  event - name of event
 *  position - for the use of external haptics providers to indicate which bit of haptic hardware should be triggered
 *  flags - a way for the code to specify which controller to produce haptics on, if 0 then weaponFireChannel is calculated in this function
 *  intensity - 0-100
 *  angle - yaw angle (again for external haptics devices) to place the feedback correctly
 *  yHeight - for external haptics devices to place the feedback correctly
 */
void VR_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight )
{
	if (vr_haptic_intensity->value == 0.0f)
	{
		return;
	}

	//Pass on to any external services
	VR_ExternalHapticEvent(event, position, flags, intensity, angle, yHeight);

	float fIntensity = intensity / 100.0f;

	//Controller Haptic Support
	int weaponFireChannel = vr.weapon_stabilised ? 3 : (vr_control_scheme->integer ? 2 : 1);

	if (flags != 0)
	{
		weaponFireChannel = flags;
	}
	if (strcmp(event, "pickup_shield") == 0 ||
		strcmp(event, "pickup_weapon") == 0 ||
		strstr(event, "pickup_item") != NULL)
	{
		TBXR_Vibrate(100, 3, 1.0);
	}
	else if (strcmp(event, "weapon_switch") == 0)
	{
		TBXR_Vibrate(250, vr_control_scheme->integer ? 2 : 1, 0.8);
	}
	else if (strcmp(event, "shotgun") == 0 || strcmp(event, "fireball") == 0)
	{
		TBXR_Vibrate(400, 3, fIntensity);
	}
	else if (strcmp(event, "bullet") == 0)
	{
		TBXR_Vibrate(150, 3, fIntensity);
	}
	else if (strcmp(event, "chainsaw_fire") == 0) // Saber
	{
		//Special handling for dual sabers / melee
		if (vr.dualsabers)
		{
			if (position == 4 ||
					position == 0) // both hands
			{
				weaponFireChannel = 3;
			}
			else if (position == 1) // left hand
			{
				weaponFireChannel = 2;
			}
			else if (position == 2) // right hand
			{
				weaponFireChannel = 1;
			}
			else
			{
				//no longer need to trigger haptic
				return;
			}
		}

		TBXR_Vibrate(300, weaponFireChannel, fIntensity);
	}
	else if (strcmp(event, "RTCWQuest:fire_tesla") == 0) // Weapon power build up
	{
		TBXR_Vibrate(500, weaponFireChannel, fIntensity);
	}
	else if (strcmp(event, "machinegun_fire") == 0 || strcmp(event, "plasmagun_fire") == 0)
	{
		TBXR_Vibrate(90, weaponFireChannel, fIntensity);
	}
	else if (strcmp(event, "shotgun_fire") == 0)
	{
		TBXR_Vibrate(250, weaponFireChannel, fIntensity);
	}
	else if (strcmp(event, "rocket_fire") == 0 ||
			 strcmp(event, "RTCWQuest:fire_sniper") == 0 ||
			 strcmp(event, "bfg_fire") == 0 ||
			 strcmp(event, "handgrenade_fire") == 0 )
	{
		TBXR_Vibrate(400, weaponFireChannel, fIntensity);
	}
	else if (strcmp(event, "selector_icon") == 0 ||
			 strcmp(event, "use_button") == 0 )
	{
		//Quick blip
		TBXR_Vibrate(50, flags, fIntensity);
	}
}

void VR_HandleControllerInput() {
	TBXR_UpdateControllers();

    bool usingSnapTurn = Core::settings.detail.turnmode == 0;

    if (!inventory->isActive())
    {
        static int increaseSnap = true;
        {
            if (usingSnapTurn)
            {
                if (rightTrackedRemoteState_new.Joystick.x > 0.7f)
                {
                    if (increaseSnap)
                    {
                        Input::hmd.extrarot -= vr_turn_angle->value;
                        increaseSnap = false;
                        if (Input::hmd.extrarot < -180.0f)
                        {
                            Input::hmd.extrarot += 360.f;
                        }
                    }
                }
                else if (rightTrackedRemoteState_new.Joystick.x < 0.3f)
                {
                    increaseSnap = true;
                }
            }

            static int decreaseSnap = true;
            if (usingSnapTurn)
            {
                if (rightTrackedRemoteState_new.Joystick.x < -0.7f)
                {
                    if (decreaseSnap)
                    {
                        Input::hmd.extrarot += vr_turn_angle->value;
                        decreaseSnap = false;

                        if (Input::hmd.extrarot > 180.0f)
                        {
                            Input::hmd.extrarot -= 360.f;
                        }
                    }
                }
                else if (rightTrackedRemoteState_new.Joystick.x > -0.3f)
                {
                    decreaseSnap = true;
                }
            }

            if (!usingSnapTurn && fabs(rightTrackedRemoteState_new.Joystick.x) > 0.1f) //smooth turn
            {
                Input::hmd.extrarot -= (Core::settings.detail.turnmode *
                                rightTrackedRemoteState_new.Joystick.x);
                if (Input::hmd.extrarot > 180.0f)
                {
                    Input::hmd.extrarot -= 360.f;
                }
            }
        }
    }

    int joyRight = 0;
    int joyLeft = 1;

    bool walkingEnabled = leftTrackedRemoteState_new.GripTrigger > 0.4f;

    Lara *lara = nullptr;
    int laraState = -1;
    if (!inventory->isActive() &&
        inventory->game->getLara())
    {
        lara = (Lara*)inventory->game->getLara();
        laraState = lara->state;

        if (lara->camera->firstPerson)
        {
            static bool reversed = false;
            if (lara->animation.index == Lara::ANIM_STAND_ROLL_BEGIN)
            {
                if (!reversed &&
                    lara->animation.frameIndex > lara->animation.framesCount * 0.8f)
                {
                    Input::hmd.extrarot += 180.f;
                    reversed = true;
                }
            }
            else
            {
                reversed = false;
            }
        }
    }

    bool actionPressed = false;
    if (!lara || lara->emptyHands())
    {
        //with empty hands left or right trigger is action
        actionPressed = (leftTrackedRemoteState_new.IndexTrigger +
                         rightTrackedRemoteState_new.IndexTrigger) > 0.4f;
    }

    if (laraState == Lara::STATE_SWIM ||
        laraState == Lara::STATE_TREAD ||
        laraState == Lara::STATE_GLIDE)
    {
        Input::setJoyPos(joyRight, jkL, vec2(0, -leftTrackedRemoteState_new.Joystick.y));
        Input::hmd.head = Input::hmd.body;
    }
    // Once we're standing still or we've entered the walking or running state we then move in the direction the user
    // is pressing the thumbstick like a modern game
    else if (!actionPressed &&
            (laraState == Lara::STATE_STOP ||
              laraState == Lara::STATE_RUN ||
              laraState == Lara::STATE_WALK ||
              laraState == Lara::STATE_FORWARD_JUMP))
    {
        vec2 joy(leftTrackedRemoteState_new.Joystick.x, leftTrackedRemoteState_new.Joystick.y);
        //deadzone
        if (joy.length() > 0.2f)
        {
            mat4 addMat;
            addMat.identity();
            float additionalDirAngle =
                    atan2(joy.x,
                          joy.y);
            addMat.rotateY(-additionalDirAngle);
            Input::hmd.head = addMat * Input::hmd.body;
        }
        else if (laraState == Lara::STATE_RUN ||
                 laraState == Lara::STATE_WALK)
        {
            lara->animation.setAnim(Lara::ANIM_STAND);
            lara->state = Lara::STATE_STOP;
        }

        Input::setJoyPos(joyRight, jkL, vec2(0, -joy.length()));
    }
    // If the user simply pressed the thumbstick in a particular direction that isn't forward
    // after already executing another move (like jump), then
    // we'll execute the move for that direction so treat it as a D pad
    else
    {
        //now adjust movement direction based on thumbstick direction
        vec2 joy(leftTrackedRemoteState_new.Joystick.x, leftTrackedRemoteState_new.Joystick.y);

        //deadzone
        if (joy.length() > 0.2f)
        {
            //Calculate which quandrant the thumbstick is pushed (UP/RIGHT/DOWN/LEFT) like a D pad
            float angle = joy.quadrant() * 90.f;
            joy.y = cosf(DEG2RAD * angle);
            joy.x = sinf(DEG2RAD * angle);

            Input::hmd.head = Input::hmd.body;
        }

        Input::setJoyPos(joyRight, jkL, vec2(joy.x, -joy.y));
    }

    if (laraState == Lara::STATE_STOP)
    {
        Input::hmd.head = Input::hmd.body;
    }

    /*
    static bool toggled = false;
    if (toggled)
    {
        Input::setJoyDown(joyRight, jkSelect, false);
        toggled = false;
    }
    else if ((!inventory->isActive() && rightTrackedRemoteState_new.GripTrigger > 0.4f &&
             rightTrackedRemoteState_old.GripTrigger <= 0.4f) ||
            (inventory->isActive() && rightTrackedRemoteState_new.GripTrigger <= 0.4f &&
             rightTrackedRemoteState_old.GripTrigger > 0.4f))
    {
        Input::setJoyDown(joyRight, jkSelect, true);
        toggled = true;
    }
    */


    vec3 vrLeftControllerPosition(leftRemoteTracking_new.GripPose.position.x,
                                  leftRemoteTracking_new.GripPose.position.y,
                                  leftRemoteTracking_new.GripPose.position.z);

    quat vrLeftControllerOrientation(leftRemoteTracking_new.GripPose.orientation.x,
                                     leftRemoteTracking_new.GripPose.orientation.y,
                                     leftRemoteTracking_new.GripPose.orientation.z,
                                     leftRemoteTracking_new.GripPose.orientation.w);

    vec3 vrRightControllerPosition(rightRemoteTracking_new.GripPose.position.x,
                                   rightRemoteTracking_new.GripPose.position.y,
                                   rightRemoteTracking_new.GripPose.position.z);

    quat vrRightControllerOrientation(rightRemoteTracking_new.GripPose.orientation.x,
                                      rightRemoteTracking_new.GripPose.orientation.y,
                                      rightRemoteTracking_new.GripPose.orientation.z,
                                      rightRemoteTracking_new.GripPose.orientation.w);

    bool twoHandShotgun = false;
    if (!lara || lara->emptyHands())
    {
        //with empty hands left or right trigger is action
        Input::setJoyDown(joyRight, jkA,  (leftTrackedRemoteState_new.IndexTrigger+rightTrackedRemoteState_new.IndexTrigger) > 0.4f ? 1 : 0);

        //Walk
        Input::setJoyDown(joyRight, jkRB, walkingEnabled ? 1 : 0);
    }
    else
    {
        //The only time joyLeft is used is to indicate the firing of the left hand weapon
        Input::setJoyDown(joyLeft, jkA,  leftTrackedRemoteState_new.IndexTrigger > 0.4f ? 1 : 0);
        Input::setJoyDown(joyRight, jkA,  rightTrackedRemoteState_new.IndexTrigger > 0.4f ? 1 : 0);

        //See if we should be trying to two hand the shotgun
/*        if (((Lara*)inventory->game->getLara())->wpnCurrent == TR::Entity::SHOTGUN)
        {
            vec3 dir = (Core::settings.detail.handedness == 0 ? vrRightControllerOrientation : vrLeftControllerOrientation) * vec3(0, 1, 0);
            if ((Core::settings.detail.handedness == 0 ? leftTrackedRemoteState_new.GripTrigger : rightTrackedRemoteState_new.GripTrigger) > 0.4f)// &&
                dir.dot((vrRightControllerPosition - vrLeftControllerPosition).normal()) > 0.5f)
            {
                twoHandShotgun = true;
            }
        }
        else */
        {
            //Walk
            Input::setJoyDown(joyRight, jkRB, walkingEnabled ? 1 : 0);
        }
    }

    //Jump
    Input::setJoyDown(joyRight, jkX, rightTrackedRemoteState_new.Buttons & xrButton_A);

    //Holster/Unholster weapons
    Input::setJoyDown(joyRight, jkY, rightTrackedRemoteState_new.Buttons & xrButton_B);

    //Roll - Reverse Direction - Right thumbstick click
    Input::setJoyDown(joyRight, jkB, rightTrackedRemoteState_new.Buttons & xrButton_RThumb);

    //Menu / Options
    Input::setJoyDown(joyRight, jkSelect, leftTrackedRemoteState_new.Buttons & xrButton_Enter);

    //Inventory controls
    if (inventory->isActive())
    {
        Input::setJoyPos(joyRight, jkL, vec2((leftTrackedRemoteState_new.Joystick.x + rightTrackedRemoteState_new.Joystick.x), -(leftTrackedRemoteState_new.Joystick.y + rightTrackedRemoteState_new.Joystick.y)));
        Input::setJoyDown(joyRight, jkA, (rightTrackedRemoteState_new.Buttons & xrButton_A) | (leftTrackedRemoteState_new.Buttons & xrButton_X));
        Input::setJoyDown(joyRight, jkSelect, (rightTrackedRemoteState_new.Buttons & xrButton_B) | (leftTrackedRemoteState_new.Buttons & xrButton_Y) | (leftTrackedRemoteState_new.Buttons & xrButton_Enter));
    }

    static bool allowSaveLoad = false;
    if (!allowSaveLoad)
    {
        allowSaveLoad = !((bool)(leftTrackedRemoteState_new.Buttons & (xrButton_X|xrButton_Y)));
    }
    else
    {
        if (leftTrackedRemoteState_new.Buttons & xrButton_X)
        {
            Game::quickSave();
            //inventory->game->invShow(0, Inventory::PAGE_SAVEGAME, 0);
            allowSaveLoad = false;
        }
        else if (leftTrackedRemoteState_new.Buttons & xrButton_Y)
        {
            Game::quickLoad();
            //inventory->game->invShow(0, Inventory::PAGE_OPTION, TR::Entity::INV_PASSPORT);
            allowSaveLoad = false;
        }
    }

    if (cheatsEnabled)
    {
        //Toggle speed up the game if right thumbrest is touched
        static bool fast = false;
        Input::setDown(ikT, fast);
        if ((rightTrackedRemoteState_new.Touches & xrButton_ThumbRest) && !(rightTrackedRemoteState_old.Touches & xrButton_ThumbRest))
        {
            fast = !fast;
        }

        if (leftTrackedRemoteState_new.Touches & xrButton_ThumbRest)
        {
            vec2 rightJoy(rightTrackedRemoteState_new.Joystick.x, rightTrackedRemoteState_new.Joystick.y);
            int quadrant = rightJoy.quadrant();
            static bool allowToggleCheat = false;
            if (!allowToggleCheat)
            {
                if (rightJoy.length() < 0.2)
                {
                    allowToggleCheat = true;
                }
            }
            else
            {
                if (rightJoy.length() > 0.2)
                {
                    if (quadrant == 0)
                    {
                        inventory->addWeapons();
                        Game::level->playSound(TR::SND_SCREAM);
                    }
                    else if (quadrant == 1)
                    {
                        Game::level->loadNextLevel();
                    }
                    else if (quadrant == 2)
                    {
                        static bool dozy = true;
                        Lara *lara = (Lara*)Game::level->getLara(0);
                        if (lara) {
                            lara->setDozy(dozy);
                            dozy = !dozy;
                        }
                    }
                    else if (quadrant == 3)
                    {
                    }

                    allowToggleCheat = false;
                }
            }
        }
    }

    float rotation = -vr_weapon_pitchadjust->value;
    if (gAppState.controllersPresent == VIVE_CONTROLLERS)
    {
        rotation += -33.6718750f;
    }

    vrRightControllerPosition = vrRightControllerPosition.rotateY(-DEG2RAD * Input::hmd.extrarot);
    vrLeftControllerPosition = vrLeftControllerPosition.rotateY(-DEG2RAD * Input::hmd.extrarot);

    if (twoHandShotgun)
    {
        Basis basis = Basis(vrRightControllerPosition, vrLeftControllerPosition, vec3(1, 0, 0));
        if (Core::settings.detail.handedness == 0)
        {
            vrRightControllerOrientation = basis.rot.conjugate();
        }
        else
        {
            vrLeftControllerOrientation = basis.rot;
        }
    }

    mat4 snapTurnMat;
    snapTurnMat.identity();
    snapTurnMat.rotateY(DEG2RAD * Input::hmd.extrarot);
    vec3 zero = Input::hmd.zero;
    zero = zero.rotateY(-DEG2RAD * Input::hmd.extrarot);

    mat4 cR = snapTurnMat * mat4(vrRightControllerOrientation, vec3(0));
    cR.setPos((vrRightControllerPosition - zero) * ONE_METER);

    //I don't know what this does but it is necessary for some reason!
    mat4 scaleBasis(
            1,  0,  0, 0,
            0, -1,  0, 0,
            0,  0, -1, 0,
            0,  0,  0, 1);

    cR = scaleBasis * cR * scaleBasis.inverse();
    Input::hmd.controllers[0] = cR;

    mat4 cL = snapTurnMat * mat4(vrLeftControllerOrientation, vec3(0));
    cL.setPos((vrLeftControllerPosition - zero) * ONE_METER);

    cL = scaleBasis * cL * scaleBasis.inverse();
    Input::hmd.controllers[1] = cL;



    //keep old state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}

/*
================================================================================

Activity lifecycle

================================================================================
*/

extern "C" {

jmethodID android_shutdown;
static JavaVM *jVM;
static jobject jniCallbackObj=0;

void jni_shutdown()
{
	ALOGV("Calling: jni_shutdown");
	JNIEnv *env;
	jobject tmp;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}
	return env->CallVoidMethod(jniCallbackObj, android_shutdown);
}

void VR_Shutdown()
{
	jni_shutdown();
}

jmethodID android_haptic_event;
jmethodID android_haptic_updateevent;
jmethodID android_haptic_stopevent;
jmethodID android_haptic_endframe;
jmethodID android_haptic_enable;
jmethodID android_haptic_disable;

void jni_haptic_event(const char* event, int position, int flags, int intensity, float angle, float yHeight)
{
	JNIEnv *env;
	jobject tmp;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}

	jstring StringArg1 = env->NewStringUTF(event);

	return env->CallVoidMethod(jniCallbackObj, android_haptic_event, StringArg1, position, flags, intensity, angle, yHeight);
}

void jni_haptic_updateevent(const char* event, int intensity, float angle)
{
	JNIEnv *env;
	jobject tmp;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}

	jstring StringArg1 = env->NewStringUTF(event);

	return env->CallVoidMethod(jniCallbackObj, android_haptic_updateevent, StringArg1, intensity, angle);
}

void jni_haptic_stopevent(const char* event)
{
	ALOGV("Calling: jni_haptic_stopevent");
	JNIEnv *env;
	jobject tmp;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}

	jstring StringArg1 = env->NewStringUTF(event);

	return env->CallVoidMethod(jniCallbackObj, android_haptic_stopevent, StringArg1);
}

void jni_haptic_endframe()
{
	JNIEnv *env;
	jobject tmp;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}

	return env->CallVoidMethod(jniCallbackObj, android_haptic_endframe);
}

void jni_haptic_enable()
{
	ALOGV("Calling: jni_haptic_enable");
	JNIEnv *env;
	jobject tmp;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}

	return env->CallVoidMethod(jniCallbackObj, android_haptic_enable);
}

void jni_haptic_disable()
{
	ALOGV("Calling: jni_haptic_disable");
	JNIEnv *env;
	if ((jVM->GetEnv((void**) &env, JNI_VERSION_1_4))<0)
	{
		jVM->AttachCurrentThread(&env, NULL);
	}

	return env->CallVoidMethod(jniCallbackObj, android_haptic_disable);
}

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
    jVM = vm;
	if(vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
	{
		ALOGE("Failed JNI_OnLoad");
		return -1;
	}

	return JNI_VERSION_1_4;
}

JNIEXPORT jlong JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onCreate( JNIEnv * env, jclass activityClass, jobject activity,
																	   jstring commandLineParams, jboolean demo)
{
	ALOGV( "    GLES3JNILib::onCreate()" );

	/* the global arg_xxx structs are initialised within the argtable */
	void *argtable[] = {
			ss    = arg_dbl0("s", "supersampling", "<double>", "super sampling value (default: Q1: 1.2, Q2: 1.35)"),
            cheats = arg_litn(NULL, "cheats", 0, 1, "Whether cheats are enabled"),
            end   = arg_end(20)
	};

	jboolean iscopy;
	const char *arg = env->GetStringUTFChars(commandLineParams, &iscopy);

	char *cmdLine = NULL;
	if (arg && strlen(arg))
	{
		cmdLine = strdup(arg);
	}

	env->ReleaseStringUTFChars(commandLineParams, arg);

	ALOGV("Command line %s", cmdLine);
	argv = (char**)malloc(sizeof(char*) * 255);
	argc = ParseCommandLine(strdup(cmdLine), argv);

	/* verify the argtable[] entries were allocated sucessfully */
	if (arg_nullcheck(argtable) == 0) {
		/* Parse the command line as defined by argtable[] */
		arg_parse(argc, argv, argtable);

        if (ss->count > 0 && ss->dval[0] > 0.0)
        {
            SS_MULTIPLIER = ss->dval[0];
        }

        if (cheats->count > 0)
        {
            cheatsEnabled = true;
        }
	}

    isDemo = demo;

	ovrAppThread * appThread = (ovrAppThread *) malloc( sizeof( ovrAppThread ) );
	ovrAppThread_Create( appThread, env, activity, activityClass );

	surfaceMessageQueue_Enable(&appThread->MessageQueue, true);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_CREATE, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);

	return (jlong)((size_t)appThread);
}


JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onStart( JNIEnv * env, jobject obj, jlong handle, jobject obj1)
{
	ALOGV( "    GLES3JNILib::onStart()" );


    jniCallbackObj = (jobject)env->NewGlobalRef( obj1);
	jclass callbackClass = env->GetObjectClass( jniCallbackObj);

	android_shutdown = env->GetMethodID(callbackClass,"shutdown","()V");
	android_haptic_event = env->GetMethodID(callbackClass, "haptic_event", "(Ljava/lang/String;IIIFF)V");
	android_haptic_updateevent = env->GetMethodID(callbackClass, "haptic_updateevent", "(Ljava/lang/String;IF)V");
	android_haptic_stopevent = env->GetMethodID(callbackClass, "haptic_stopevent", "(Ljava/lang/String;)V");
	android_haptic_endframe = env->GetMethodID(callbackClass, "haptic_endframe", "()V");
	android_haptic_enable = env->GetMethodID(callbackClass, "haptic_enable", "()V");
	android_haptic_disable = env->GetMethodID(callbackClass, "haptic_disable", "()V");

	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_START, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onResume( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onResume()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_RESUME, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onPause( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onPause()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_PAUSE, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onStop( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onStop()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_STOP, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onDestroy( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onDestroy()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_DESTROY, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
	surfaceMessageQueue_Enable(&appThread->MessageQueue, false);

	ovrAppThread_Destroy( appThread, env );
	free( appThread );
}

/*
================================================================================

Surface lifecycle

================================================================================
*/

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onSurfaceCreated( JNIEnv * env, jobject obj, jlong handle, jobject surface )
{
	ALOGV( "    GLES3JNILib::onSurfaceCreated()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

	ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
	if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
	{
		// An app that is relaunched after pressing the home button gets an initial surface with
		// the wrong orientation even though android:screenOrientation="landscape" is set in the
		// manifest. The choreographer callback will also never be called for this surface because
		// the surface is immediately replaced with a new surface with the correct orientation.
		ALOGE( "        Surface not in landscape mode!" );
	}

	ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
	appThread->NativeWindow = newNativeWindow;
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED);
	surfaceMessage_SetPointerParm(&message, 0, appThread->NativeWindow);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
}

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onSurfaceChanged( JNIEnv * env, jobject obj, jlong handle, jobject surface )
{
	ALOGV( "    GLES3JNILib::onSurfaceChanged()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);

	ANativeWindow * newNativeWindow = ANativeWindow_fromSurface( env, surface );
	if ( ANativeWindow_getWidth( newNativeWindow ) < ANativeWindow_getHeight( newNativeWindow ) )
	{
		// An app that is relaunched after pressing the home button gets an initial surface with
		// the wrong orientation even though android:screenOrientation="landscape" is set in the
		// manifest. The choreographer callback will also never be called for this surface because
		// the surface is immediately replaced with a new surface with the correct orientation.
		ALOGE( "        Surface not in landscape mode!" );
	}

	if ( newNativeWindow != appThread->NativeWindow )
	{
		if ( appThread->NativeWindow != NULL )
		{
			srufaceMessage message;
			surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED);
			surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
			ALOGV( "        ANativeWindow_release( NativeWindow )" );
			ANativeWindow_release( appThread->NativeWindow );
			appThread->NativeWindow = NULL;
		}
		if ( newNativeWindow != NULL )
		{
			ALOGV( "        NativeWindow = ANativeWindow_fromSurface( env, surface )" );
			appThread->NativeWindow = newNativeWindow;
			srufaceMessage message;
			surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_CREATED, MQ_WAIT_PROCESSED);
			surfaceMessage_SetPointerParm(&message, 0, appThread->NativeWindow);
			surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
		}
	}
	else if ( newNativeWindow != NULL )
	{
		ANativeWindow_release( newNativeWindow );
	}
}

JNIEXPORT void JNICALL Java_com_drbeef_beefraiderxr_GLES3JNILib_onSurfaceDestroyed( JNIEnv * env, jobject obj, jlong handle )
{
	ALOGV( "    GLES3JNILib::onSurfaceDestroyed()" );
	ovrAppThread * appThread = (ovrAppThread *)((size_t)handle);
	srufaceMessage message;
	surfaceMessage_Init(&message, MESSAGE_ON_SURFACE_DESTROYED, MQ_WAIT_PROCESSED);
	surfaceMessageQueue_PostMessage(&appThread->MessageQueue, &message);
	ALOGV( "        ANativeWindow_release( NativeWindow )" );
	ANativeWindow_release( appThread->NativeWindow );
	appThread->NativeWindow = NULL;
}

}

