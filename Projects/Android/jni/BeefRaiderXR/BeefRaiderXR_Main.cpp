#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include <cstring>
#include <string>

#include "VrCommon.h"

#include <game.h>

#ifdef ANDROID
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h>	// for native window JNI
#include <android/input.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "android/argtable3.h"

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

#endif

bool cheatsEnabled = false;
bool isDemo = false;

ovrInputStateTrackedRemote leftTrackedRemoteState_old;
ovrInputStateTrackedRemote leftTrackedRemoteState_new;
ovrTrackedController leftRemoteTracking_new;
ovrInputStateTrackedRemote rightTrackedRemoteState_old;
ovrInputStateTrackedRemote rightTrackedRemoteState_new;
ovrTrackedController rightRemoteTracking_new;
ovrInputStateGamepad gamepadState_old;
ovrInputStateGamepad gamepadState_new;

vec3_t hmdorientation_snap;

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

#ifdef ANDROID

/*
================
Sys_Milliseconds
================
*/
unsigned long sys_timeBase = 0;
int curtime;
int Sys_Milliseconds ()
{
    struct timeval tp;

    gettimeofday(&tp, NULL);

    if (!sys_timeBase)
    {
        sys_timeBase = tp.tv_sec;
        return tp.tv_usec/1000;
    }

    curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

    static int sys_timeBase = curtime;
    curtime -= sys_timeBase;
    return curtime;
}

int   osGetTimeMS()
{
    return int(Sys_Milliseconds ()) - osStartTime;
}

#else
	
RECT windowSize = { 0, 0, 960, 960 };

int osGetTimeMS() {
#ifdef DEBUG
    LARGE_INTEGER Freq, Count;
    QueryPerformanceFrequency(&Freq);
    QueryPerformanceCounter(&Count);
    return int(Count.QuadPart * 1000L / Freq.QuadPart);
#else
    timeBeginPeriod(0);
    return int(timeGetTime()) - osStartTime;
#endif
}

#endif

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
    Input::hmd.nextrot = 0.f;
    Input::hmd.extrarot = 0.f;
    Input::hmd.extrarot2 = 0.f;
    Input::hmd.extraworldscaler = 1.f;
}

#ifdef ANDROID

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

#else
	

// multi-threading
void* osMutexInit() {
    CRITICAL_SECTION* CS = new CRITICAL_SECTION();
    InitializeCriticalSection(CS);
    return CS;
}

void osMutexFree(void* obj) {
    DeleteCriticalSection((CRITICAL_SECTION*)obj);
    delete (CRITICAL_SECTION*)obj;
}

void osMutexLock(void* obj) {
    EnterCriticalSection((CRITICAL_SECTION*)obj);
}

void osMutexUnlock(void* obj) {
    LeaveCriticalSection((CRITICAL_SECTION*)obj);
}

// common input functions
InputKey keyToInputKey(int code) {
    static const int codes[] = {
        VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_TAB, VK_RETURN, VK_ESCAPE, VK_SHIFT, VK_CONTROL, VK_MENU,
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_ADD, VK_SUBTRACT, VK_MULTIPLY, VK_DIVIDE, VK_DECIMAL,
        VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12,
        VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_4, VK_OEM_6, VK_OEM_2, VK_OEM_5, VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_3, VK_OEM_1, VK_OEM_7, VK_PRIOR, VK_NEXT, VK_HOME, VK_END, VK_DELETE, VK_INSERT, VK_BACK,
    };

    for (int i = 0; i < COUNT(codes); i++) {
        if (codes[i] == code) {
            return (InputKey)(ikLeft + i);
        }
    }
    return ikNone;
}

// sound
#define SND_SIZE 4704*2

bool sndReady;
char* sndData;
HWAVEOUT waveOut;
WAVEFORMATEX waveFmt = { WAVE_FORMAT_PCM, 2, 44100, 44100 * 4, 4, 16, sizeof(waveFmt) };
WAVEHDR waveBuf[2];
HANDLE  sndThread;
HANDLE  sndSema;

void sndFree() {
    if (!sndReady) return;
    sndReady = false;
    ReleaseSemaphore(sndSema, 1, NULL);
    WaitForSingleObject(sndThread, INFINITE);
    CloseHandle(sndThread);
    CloseHandle(sndSema);
    waveOutUnprepareHeader(waveOut, &waveBuf[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(waveOut, &waveBuf[1], sizeof(WAVEHDR));
    waveOutReset(waveOut);
    waveOutClose(waveOut);
    delete[] sndData;
}

DWORD WINAPI sndPrep(void* arg) {
    int idx = 0;
    while (1) {
        WaitForSingleObject(sndSema, INFINITE);
        if (!sndReady) break;

        WAVEHDR* hdr = waveBuf + idx;
        waveOutUnprepareHeader(waveOut, hdr, sizeof(WAVEHDR));
        Sound::fill((Sound::Frame*)hdr->lpData, SND_SIZE / 4);
        waveOutPrepareHeader(waveOut, hdr, sizeof(WAVEHDR));
        waveOutWrite(waveOut, hdr, sizeof(WAVEHDR));

        idx ^= 1;
    }
    return 0;
}

void sndFill(HWAVEOUT waveOut, LPWAVEHDR waveBufPrev) {
    if (!sndReady) return;
    ReleaseSemaphore(sndSema, 1, NULL);
}

void sndInit(HWND hwnd) {
    if (waveOutOpen(&waveOut, WAVE_MAPPER, &waveFmt, (INT_PTR)hwnd, 0, CALLBACK_WINDOW) == MMSYSERR_NOERROR) {
        sndReady = true;
        sndData = new char[SND_SIZE * 2];
        memset(sndData, 0, SND_SIZE * 2);
        memset(&waveBuf, 0, sizeof(waveBuf));
        for (int i = 0; i < 2; i++) {
            WAVEHDR* hdr = waveBuf + i;
            hdr->dwBufferLength = SND_SIZE;
            hdr->lpData = sndData + SND_SIZE * i;
            waveOutPrepareHeader(waveOut, hdr, sizeof(WAVEHDR));
            waveOutWrite(waveOut, hdr, sizeof(WAVEHDR));
        }
        sndSema = CreateSemaphore(NULL, 0, 2, NULL);
        sndThread = CreateThread(NULL, 0, sndPrep, NULL, 0, NULL);
    }
    else {
        sndReady = false;
        sndData = NULL;
    }
}


HWND hWnd;


#ifdef _GAPI_SW
HDC    hDC;

void ContextCreate() {
    hDC = GetDC(hWnd);
}

void ContextDelete() {
    ReleaseDC(hWnd, hDC);
    delete[] GAPI::swColor;
}

void ContextResize() {
    delete[] GAPI::swColor;
    GAPI::swColor = new GAPI::ColorSW[Core::width * Core::height];

    GAPI::resize();
}

void ContextSwap() {
    const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), Core::width, -Core::height, 1, sizeof(GAPI::ColorSW) * 8, BI_RGB, 0, 0, 0, 0, 0 };
    SetDIBitsToDevice(hDC, 0, 0, Core::width, Core::height, 0, 0, 0, Core::height, GAPI::swColor, &bmi, 0);
}
#elif _GAPI_GL
HDC   hDC;
HGLRC hRC;

void ContextCreate() {
    hDC = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cColorBits = 32;
    pfd.cRedBits = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits = 8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int format = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, format, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
}

void ContextDelete() {
    wglMakeCurrent(0, 0);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
}

void ContextSwap() {
    SwapBuffers(hDC);
}
#elif _GAPI_D3D9
LPDIRECT3D9           D3D;
LPDIRECT3DDEVICE9     device;
D3DPRESENT_PARAMETERS d3dpp;

void ContextCreate() {
    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferCount = 1;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    if (!(D3D = Direct3DCreate9(D3D_SDK_VERSION))) {
        LOG("! cant't initialize DirectX\n");
        return;
    }

    if (!SUCCEEDED(D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)))
        LOG("! can't create device\n");
}

void ContextDelete() {
    GAPI::deinit();
    if (device) device->Release();
    if (D3D)    D3D->Release();
}

void ContextResize() {
    if (Core::width <= 0 || Core::height <= 0)
        return;
    d3dpp.BackBufferWidth = Core::width;
    d3dpp.BackBufferHeight = Core::height;
    GAPI::resetDevice();
}

void ContextSwap() {
    if (device->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST)
        GAPI::resetDevice();
}
#elif _GAPI_D3D11
ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
IDXGISwapChain* swapChain;

void ContextCreate() {
    DXGI_SWAP_CHAIN_DESC desc = { 0 };
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hWnd;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Windowed = TRUE;
    desc.OutputWindow = hWnd;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT ret;

    ret = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &desc, &swapChain, &device, NULL, &deviceContext);
    ASSERT(ret == S_OK);

    GAPI::defRTV = NULL;
    GAPI::defDSV = NULL;
}

void ContextDelete() {
    GAPI::deinit();
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(deviceContext);
    SAFE_RELEASE(device);
}

void ContextResize() {
    if (Core::width <= 0 || Core::height <= 0)
        return;

    GAPI::resetDevice();

    HRESULT ret = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    ASSERT(ret == S_OK);
}

void ContextSwap() {
    HRESULT ret = swapChain->Present(Core::settings.detail.vsync ? 1 : 0, 0);
}
#endif

#ifdef _NAPI_SOCKET
char command[256];

void parseCommand(char* cmd) {
    NAPI::Peer peer;
    int pos = 0;
    for (int i = 0; i < strlen(cmd); i++)
        if (cmd[i] == ':') {
            cmd[i] = 0;
            pos = i + 1;
            break;
        }
    peer.ip = inet_addr(cmd);
    peer.port = htons(atoi(&cmd[pos]));
    cmd[pos - 1] = ':';

    LOG("join %s:%d\n", inet_ntoa(*(in_addr*)&peer.ip), ntohs(peer.port));
    Network::joinGame(peer);
}
#endif

int checkLanguage() {
    LANGID id = GetUserDefaultUILanguage() & 0xFF;
    int str = STR_LANG_EN;
    switch (id) {
    case LANG_ENGLISH: str = STR_LANG_EN; break;
    case LANG_FRENCH: str = STR_LANG_FR; break;
    case LANG_GERMAN: str = STR_LANG_DE; break;
    case LANG_SPANISH: str = STR_LANG_ES; break;
    case LANG_ITALIAN: str = STR_LANG_IT; break;
    case LANG_POLISH: str = STR_LANG_PL; break;
    case LANG_PORTUGUESE: str = STR_LANG_PT; break;
    case LANG_RUSSIAN:
    case LANG_UKRAINIAN:
    case LANG_BELARUSIAN: str = STR_LANG_RU; break;
    case LANG_JAPANESE: str = STR_LANG_JA; break;
    case LANG_GREEK: str = STR_LANG_GR; break;
    case LANG_FINNISH: str = STR_LANG_FI; break;
    case LANG_CZECH: str = STR_LANG_CZ; break;
    case LANG_CHINESE: str = STR_LANG_CN; break;
    }
    return str - STR_LANG_EN;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // window
    case WM_ACTIVATE:
        break;
    case WM_SIZE:
        windowSize.right = windowSize.left + LOWORD(lParam);
        windowSize.bottom = windowSize.top + HIWORD(lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        // keyboard
    case WM_CHAR:
    case WM_SYSCHAR:
#ifdef _NAPI_SOCKET
        if (wParam == VK_RETURN) {
            parseCommand(command);
            //command[0] = 0;
        }
        else if ((wParam >= '0' && wParam <= '9') || wParam == ':' || wParam == '.') {
            int len = strlen(command);
            command[len] = wParam;
            command[len + 1] = 0;
        }
        else if (wParam == 8) {
            int len = strlen(command);
            if (len > 0)
                command[len - 1] = 0;
        }
#endif
        break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        if (msg == WM_SYSKEYDOWN && wParam == VK_RETURN) { // Alt + Enter - switch to fullscreen or window
            static WINDOWPLACEMENT pLast;
            DWORD style = GetWindowLong(hWnd, GWL_STYLE);
            if (style & WS_OVERLAPPEDWINDOW) {
                MONITORINFO mInfo = { sizeof(mInfo) };
                if (GetWindowPlacement(hWnd, &pLast) && GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mInfo)) {
                    RECT& r = mInfo.rcMonitor;
                    SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                    MoveWindow(hWnd, r.left, r.top, r.right - r.left, r.bottom - r.top, FALSE);
                }
            }
            else {
                SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
                SetWindowPlacement(hWnd, &pLast);
            }
            break;
        }
        if (msg == WM_SYSKEYDOWN && wParam == VK_F4) { // Alt + F4 - close application
            Core::quit();
            break;
        }
        Input::setDown(keyToInputKey(wParam), msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
        break;
        // sound
    case MM_WOM_DONE:
        sndFill((HWAVEOUT)wParam, (WAVEHDR*)lParam);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

#endif


#ifdef ANDROID


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

#endif

/*
================================================================================

BeefRaiderXR Stuff

================================================================================
*/

bool VR_UseScreenLayer()
{
	return inventory->video;
}

float VR_GetScreenLayerDistance()
{
	return 5.5f;
}


void VR_SetHMDOrientation(float pitch, float yaw, float roll)
{
    static int frame = 0;

    //Can be set elsewhere
    if (frame++ < 100)
    {
        //Record player position on transition
        VectorSet(hmdorientation_snap, pitch, yaw, roll);
    }
}

void VR_SetHMDPosition(float x, float y, float z )
{
}

void VR_Init()
{	
#ifndef ANDROID
    GlInitExtensions();
    TBXR_InitialiseOpenXR();
    TBXR_EnterVR();
    TBXR_InitRenderer();
    TBXR_InitActions();
    TBXR_WaitForSessionActive();
#endif

	//Initialise all our variables
	Input::hmd.nextrot = 0.0f;
	Input::hmd.extrarot = 0.0f;
	Input::hmd.extrarot2 = 0.0f;
	Input::hmd.extraworldscaler = 1.0f;

	//init randomiser
	srand(time(NULL));

#ifdef ANDROID
    Cvar_Get("openXRHMD", gAppState.OpenXRHMD, CVAR_ARCHIVE);
#endif

    int eyeW, eyeH;
    TBXR_GetScreenRes(&eyeW, &eyeH);

    Core::width  = eyeW;
    Core::height = eyeH;

    Core::eyeTex[0] = new Texture(eyeW, eyeH, 1, TexFormat::FMT_RGBA, OPT_TARGET);
    Core::eyeTex[1] = new Texture(eyeW, eyeH, 1, TexFormat::FMT_RGBA, OPT_TARGET);

    Input::hmd.forceUpdatePose = true;
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

void VR_prepareEyeBuffer(int eye )
{
    //Bit of a fiddle really
    ovrFramebuffer* frameBuffer = &(gAppState.Renderer.FrameBuffer[eye]);
    Core::eyeTex[eye]->ID = frameBuffer->ColorSwapChainImage[frameBuffer->TextureSwapChainIndex].image;
}

int VR_SetRefreshRate(int refreshRate)
{
#ifdef ANDROID
	if (strstr(gAppState.OpenXRHMD, "meta") != NULL)
	{
		OXR(gAppState.pfnRequestDisplayRefreshRate(gAppState.Session, (float) refreshRate));
		return refreshRate;
	}
#endif
	return 0;
}

#ifdef ANDROID
extern "C" {
    bool IsMRMode()
    {
        return 	!VR_UseScreenLayer() &&
            (Core::settings.detail.mixedRealityMode == 1) 
            && !inventory->isActive();
    }
}
#endif

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

    //Apply any rotation changes from previous frame
    Input::hmd.extrarot2 += (Input::hmd.nextrot - Input::hmd.extrarot);
    Input::hmd.extrarot = Input::hmd.nextrot;

    mat4 snapTurnMat;
    snapTurnMat.identity();
    snapTurnMat.rotateY(Input::hmd.extrarot);

    mat4 head = snapTurnMat * mat4(vrOrientation, vec3(0));

    ICamera::PointOfView pov = ICamera::POV_1ST_PERSON;
    Lara* lara = nullptr;
    if (inventory->game->getLara())
    {
        lara = (Lara*)inventory->game->getLara();
        pov = lara->camera->getPointOfView();
    }

    //6DOF calculation
    vec3 laraPos;
    int laraStand = Lara::STAND_GROUND;
    float laraAngleY = 0.f;
    if (!inventory->isActive() && lara)
    {
        static vec3 prevPos;        
        laraPos = lara->getPos();
        laraStand = lara->stand;
        laraAngleY = lara->angle.y;

        //Reset here, if we don't then it can break things otherwise
        lara->velocity_6dof = vec2(0.f);

        //6DoF only in 1st person
        if (pov == ICamera::POV_1ST_PERSON)
        {
            //Bit of a hack, but if Lara uses an item (not a med kit), don't re-enable 6dof until she returns to a valid state
            static bool usedItem = false;
            if (lara->usedItem > TR::Entity::NONE)
            {
                usedItem = lara->usedItem != TR::Entity::INV_MEDIKIT_SMALL &&
                    lara->usedItem != TR::Entity::INV_MEDIKIT_BIG;
            }

            //Only apply 6dof in certain scenarios
            if (lara->state == Lara::STATE_WALK ||
                lara->state == Lara::STATE_RUN ||
                lara->state == Lara::STATE_STOP)
            {
                if (!usedItem)
                {
                    lara->velocity_6dof.x = (vrPosition - prevPos).x;
                    lara->velocity_6dof.z = -(vrPosition - prevPos).z;
                    lara->velocity_6dof *= 1000;
                    lara->velocity_6dof = lara->velocity_6dof.rotateY(Input::hmd.extrarot);
                }
            }
            else
            {
                //We've now started the used item animation
                usedItem = false;
            }
        }

        prevPos = vrPosition;
    }

    static bool isTriggerUpdatePose = false;
    if (leftTrackedRemoteState_new.Buttons & xrButton_LThumb)
    {
        if (!isTriggerUpdatePose)
        {
            Input::hmd.mrpos = laraPos;
            Input::hmd.forceUpdatePose = true;
            isTriggerUpdatePose = true;
        }
    }
    else
    {
        isTriggerUpdatePose = false;
    }

    if (Input::hmd.zero.x == INF || Input::hmd.forceUpdatePose)
    {
        Input::hmd.mrorg = Input::hmd.mrpos;
        Input::hmd.zero = vrPosition;
    }

    vec3 zero = Input::hmd.zero;

    Input::hmd.angleY = Controller::getAngleAbs(mat4(vrOrientation, vec3(0)).dir().xyz()).y;
    float angleY = -(Controller::getAngleAbs(Input::hmd.head.dir().xyz()).y);

    static bool prevMRMode = Core::settings.detail.mixedRealityMode;
    if (!prevMRMode && (Core::settings.detail.mixedRealityMode > 0))
    {
        //Switch on MR mode reset
        Input::hmd.extrarot = 0;
        Input::hmd.nextrot = 0;
        Input::hmd.extrarot2 = 0;
    }
    prevMRMode = (bool)Core::settings.detail.mixedRealityMode;

    if (pov == ICamera::POV_1ST_PERSON || Input::hmd.forceUpdatePose)
    {
        if (pov == ICamera::POV_1ST_PERSON || laraStand == Lara::STAND_UNDERWATER)
        {
            Input::hmd.head = head;
            Input::hmd.body.setRot(head.getRot());
        }
        if (!Core::settings.detail.mixedRealityMode)
        {
            if (pov != ICamera::POV_1ST_PERSON)
            {
                if (laraStand == Lara::STAND_UNDERWATER)
                {
                    angleY = -laraAngleY;
                }

                Input::hmd.extrarot = angleY + Input::hmd.angleY;
                Input::hmd.nextrot = angleY + Input::hmd.angleY;
                Input::hmd.extrarot2 = angleY;
            }
            else
            {
                Input::hmd.extrarot2 = angleY;
            }
        }
    }
    else
    {
        mat4 m;
        m.identity();
        m.rotateY(Input::hmd.extrarot2);
        Input::hmd.body.setRot(m.getRot());
    }

    vrPosition = vrPosition.rotateY(-Input::hmd.extrarot);
    zero = zero.rotateY(-Input::hmd.extrarot);

    Input::hmd.head.setPos(vrPosition);
    Input::hmd.body.setPos(vrPosition - zero);
    Input::hmd.body.e03 *= -1.0f; // have to do this to correct 3rd person X position
    Input::hmd.mrpos = Input::hmd.mrorg - (Input::hmd.body.getPos() * (ONE_METER * Input::hmd.extraworldscaler));

    //Left eye
    mat4 vL = head;
    vL.setPos((head.right().xyz() * (-0.065f / 2.f)) * ONE_METER);

    //Right eye
    mat4 vR = head;
    vR.setPos((head.right().xyz() * (0.065f / 2.f)) * ONE_METER);

    Input::hmd.setView(pL, pR, vL, vR);

    //Reset this flag if it was set
    Input::hmd.forceUpdatePose = false;
}


#ifdef ANDROID
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
    std::string brxrDir = (char*)getenv("BRXR_DIR");
    if (isDemo)
    {
        chdir((brxrDir + "/DATA").c_str());
        levelName = "LEVEL2.PHD";
    }
    else
    {
        chdir(brxrDir.c_str());
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

    TBXR_LeaveVR();
    VR_Shutdown();

	return NULL;
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

#else
	
void VR_ExternalHapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight )
{
}

void VR_HapticUpdateEvent(const char* event, int intensity, float angle )
{
}

void VR_HapticEndFrame()
{
}

void VR_HapticStopEvent(const char* event)
{
}

void VR_HapticEnable()
{

}

void VR_HapticDisable()
{
}

#endif

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
	//Pass on to any external services
	VR_ExternalHapticEvent(event, position, flags, intensity, angle, yHeight);

	float fIntensity = intensity / 100.0f;

	//Controller Haptic Support
	int weaponFireChannel = /*vr.weapon_stabilised*/ false ? 3 : (Core::settings.detail.handedness == 1 ? 2 : 1);

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
		TBXR_Vibrate(250, Core::settings.detail.handedness == 1 ? 2 : 1, 0.8f);
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

    if (Core::settings.detail.switchsticks)
    {
        swap(rightTrackedRemoteState_new.Joystick, leftTrackedRemoteState_new.Joystick);
    }

    mat4 snapTurnMat;
    snapTurnMat.identity();
    snapTurnMat.rotateY(Input::hmd.extrarot);
    vec3 zero = Input::hmd.head.getPos();

    Lara *lara = nullptr;
    int laraState = -1;
    int laraStand = -1;
    int laraAnim = -1;
    bool pov1stPerson = false, pov3rdPerson = false, povToyMode = false;
    bool actionPressed = false;
    bool usingSnapTurn = true;
    static bool movingBackwards = false;
    if (inventory->game->getLara())
    {
        lara = (Lara*)inventory->game->getLara();
        laraState = lara->state;
        laraStand = lara->getStand();
        ICamera::PointOfView pov = lara->camera->getPointOfView();
        pov1stPerson = pov == ICamera::POV_1ST_PERSON;
        pov3rdPerson = pov == ICamera::POV_3RD_PERSON_VR_1 || pov == ICamera::POV_3RD_PERSON_VR_2;
        povToyMode = pov == ICamera::POV_3RD_PERSON_VR_TOY_MODE;
        static int spingrab = 0;

        usingSnapTurn = ((Core::settings.detail.turnmode == 0 && (!pov3rdPerson || Core::settings.detail.getCameraModeMode() == Core::CameraMode::MODERN)) ||
            (Core::settings.detail.turnmode == 1 && pov1stPerson));

        //with empty hands left or right trigger is action
        if (lara->emptyHands() || pov != ICamera::POV_1ST_PERSON || Core::settings.detail.autoaim)
        {
            actionPressed = (leftTrackedRemoteState_new.IndexTrigger +
                rightTrackedRemoteState_new.IndexTrigger) > 0.5f;
        }

        //Claculate if Lara is actually moving backwards
        vec3 headDir = Input::hmd.head.dir().xyz();
        headDir.y = 0;
        headDir.x *= -1;
        vec3 velDir = lara->velocity;
        velDir.y = 0;
        bool movingBackwards = headDir.dot(velDir) < 0.f;

        /*
        * Prevent triggering spingrab during certain animations:
        * 
        * Roll Reverse Direction
        * Back Hop
        * Backwards off a ledge
        * 
        */
        {            
            static bool reversed = false;


            if (lara->animation.index == Lara::ANIM_STAND_ROLL_BEGIN ||
                lara->animation.index == Lara::ANIM_HOP_BACK ||
                (movingBackwards && ((pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::CLASSIC) || pov1stPerson)))
            {
                if (!reversed &&
                    lara->animation.frameIndex > lara->animation.framesCount * 0.8f)
                {
                    //Special code to reverse direction
                    if (lara->animation.index == Lara::ANIM_STAND_ROLL_BEGIN)
                    {
                        //Ensure Lara rotates so she can ledge grab
                        if (pov != ICamera::POV_1ST_PERSON)
                        {
                            Input::hmd.head.rotateY(PI);
                        }

                        if (!Core::settings.detail.mixedRealityMode)
                        {
                            Input::hmd.nextrot += PI;
                        }
                    }

                    reversed = true;
                }

                //prevent spinggrab from starting if we go over an edge
                spingrab = 1;
            }
            else
            {
                reversed = false;
            }
        }

        /*
        * Remaster style - spin grab (run off ledge with action pressed and Lara will spin and grab ledge)
        */
        {
            const int grabcount = 26;
            if (lara->animation.index == Lara::ANIM_FALL_FORTH &&
                laraStand == Lara::STAND_AIR &&
                laraState == Lara::STATE_FORWARD_JUMP &&
                spingrab == 0 &&
                actionPressed)
            {
                spingrab = grabcount + 1;
                lara->velocity /= vec3(20.0f);
            }

            if (spingrab > 1)
            {
                if ((pov1stPerson && usingSnapTurn) ||
                    (pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::CLASSIC))
                {
                    if (spingrab == (grabcount / 2))
                    {
                        lara->angle.y -= PI;
                        Input::hmd.nextrot += PI;
                    }
                }
                else
                {
                    float angle = PI / grabcount;
                    if (pov1stPerson)
                    {
                        Input::hmd.nextrot += angle;
                    }

                    lara->angle.y -= angle;
                    Input::hmd.head.rotateY(-angle);
                }

                spingrab--;
            }

            //Reset once Lara isn't in the air
            if (laraStand == Lara::STAND_GROUND &&
                laraState == Lara::STATE_STOP)
            {
                spingrab = 0;
            }
        }
    }

    bool walkingEnabled = leftTrackedRemoteState_new.GripTrigger > 0.4f;
    bool turning = false;

    int joyRight = 0;
    int joyLeft = 1;

    //If swimming allow either joystick to snap/smooth turn you
    XrVector2f joystick = rightTrackedRemoteState_new.Joystick;
    if (laraStand == Lara::STAND_UNDERWATER &&
        !Core::settings.detail.mixedRealityMode && !povToyMode)
    {
        joystick.x += leftTrackedRemoteState_new.Joystick.x;
    }

    // The following scenarios should prevent the camera from being rotated
    // - Lara hanging from ledge or pushing block in 1st Person
    // - Lara hanging from ledge or pushing block  in 3rd person with classic camera mode
    bool lockCameraRotation = (
        (laraState == Lara::STATE_HANG || 
        laraState == Lara::STATE_PULL_BLOCK ||
        laraState == Lara::STATE_PUSH_BLOCK ||
        laraState == Lara::STATE_HANG_LEFT || 
        laraState == Lara::STATE_HANG_RIGHT) &&
        (pov1stPerson || (pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::CLASSIC)));

    if (!lockCameraRotation &&
        !inventory->isActive() && 
        laraState != Lara::STATE_DEATH &&
        !Core::settings.detail.mixedRealityMode)
    {
        vec2 rjoy(joystick.x, joystick.y);
        int sect = rjoy.sector(16);
        bool right = sect >= 3 || sect <= 5;
        bool left = sect >= 11 || sect <= 13;
        if (usingSnapTurn)
        {
            static int snap = true;
            if (rjoy.length() > 0.7f)
            {
                if (snap && (right || left))
                {
                    Input::hmd.nextrot += right ? -PIQ : PIQ;
                    if (Input::hmd.nextrot < -PI)
                    {
                        Input::hmd.nextrot += PI2;
                    }
                    if (Input::hmd.nextrot > PI)
                    {
                        Input::hmd.nextrot -= PI2;
                    }
                    snap = false;
                }
            }
            else if (rjoy.length() < 0.3f)
            {
                snap = true;
            }
        }
        else if (rjoy.length() > 0.5f && (right || left)) //smooth turn
        {
            int speed = (Core::settings.detail.turnmode < 3) ? 1 : 2;
            Input::hmd.nextrot -= ((speed * joystick.x) * DEG2RAD);
            if (Input::hmd.nextrot > PI)
            {
                Input::hmd.nextrot -= PI2;
            }
            if (Input::hmd.nextrot < -PI)
            {
                Input::hmd.nextrot += PI2;
            }

            //We only want Lara to do the in-place rotation animation when in classic camera mode 3rd person (not toy mode)
            if ((pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::CLASSIC) &&
                (!walkingEnabled || fabs(leftTrackedRemoteState_new.Joystick.x) <= 0.01f))
            {
                if (lara->state == Lara::STATE_STOP ||
                    lara->state == Lara::STATE_TURN_LEFT ||
                    lara->state == Lara::STATE_TURN_RIGHT ||
                    lara->state == Lara::STATE_FAST_TURN)
                {
                    Input::setJoyPos(joyRight, jkL, vec2(rightTrackedRemoteState_new.Joystick.x, 0));
                    turning = true;
                    //Disable walking so Lara doesn't side step
                    walkingEnabled = false;
                }
            }
        }
    }

    vec2 joy(leftTrackedRemoteState_new.Joystick.x, leftTrackedRemoteState_new.Joystick.y);
    if (Core::settings.detail.mixedRealityMode)
    {
        joy = vec2(joy.x, -joy.y).rotate(-(Input::hmd.extrarot)
            - Controller::getAngleAbs(Input::hmd.head.dir().xyz()).y
            + Input::hmd.angleY);
    }
    else if (povToyMode || (pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::MODERN))
    {
        joy = vec2(joy.x, -joy.y).rotate(-(Input::hmd.extrarot2) - Controller::getAngleAbs(Input::hmd.head.dir().xyz()).y);
    }
    else
    {
        joy = vec2(joy.x, -joy.y);
    }

    int sector = joy.sector(2);

    //Allow Lara to hop back if holding guns (3rd person only)
    if (!pov1stPerson &&
        (laraState == Lara::STATE_STOP || laraState == Lara::STATE_FAST_BACK) &&
        joy.quadrant() == 0 &&
        lara && !lara->emptyHands())
    {
        Input::setJoyPos(joyRight, jkL, vec2(0, 1));
    }
    //Allow Lara to hop back using X button
    else if ((laraState == Lara::STATE_STOP || laraState == Lara::STATE_FAST_BACK) &&
        leftTrackedRemoteState_new.Buttons & xrButton_X)
    {
        Input::setJoyPos(joyRight, jkL, vec2(0, 1));
    }
    //Trigger walking back or side stepping from a stopped position (3rd person only)
    else if ((laraState == Lara::STATE_STOP ||
            laraState == Lara::STATE_BACK ||
            laraState == Lara::STATE_STEP_LEFT ||
            laraState == Lara::STATE_STEP_RIGHT) &&
        !pov1stPerson &&
        (joy.sector(4) != -1) && (joy.sector(4) != 2) &&
        walkingEnabled && !turning)
    {
        int back = joy.sector(4) == 0 ? 1 : 0;
        int side = joy.sector(4) == 1 ? 1 : joy.sector(4) == 3 ? -1 : 0;
        Input::setJoyPos(joyRight, jkL, vec2(side, back));
    }
    else if (laraStand == Lara::STAND_UNDERWATER)
    {
        //only pitch controlled in 3rd person
        int invert = Core::settings.detail.invertstickswimming ? -1 : 1;
        if (pov1stPerson)
        {
            Input::hmd.head.setRot(Input::hmd.body.getRot());
            Input::setJoyPos(joyRight, jkL, vec2(0, -leftTrackedRemoteState_new.Joystick.y));
        }
        else if (!Core::settings.detail.mixedRealityMode && !povToyMode)
        {
            Input::setJoyPos(joyRight, jkL, vec2(0, invert * leftTrackedRemoteState_new.Joystick.y));
            if (!actionPressed)
            {
                Input::hmd.head.setRot(Input::hmd.body.getRot());
            }
        }
        else
        {
            //Fully joystick controlled for MR mode and toy mode
            Input::setJoyPos(joyRight, jkL, vec2(leftTrackedRemoteState_new.Joystick.x, invert * leftTrackedRemoteState_new.Joystick.y));
        }
    }
    // Once we're standing still or we've entered the walking or running state we then move in the direction the user
    // is pressing the thumbstick like a modern game
    else if ((!actionPressed || !pov1stPerson) &&
            (Game::level && !Game::level->level.isCutsceneLevel()) &&
            !inventory->active &&
            (laraState == Lara::STATE_STOP ||
              laraState == Lara::STATE_RUN ||
              laraState == Lara::STATE_FORWARD_JUMP ||
              laraState == Lara::STATE_WALK ||
              laraState == Lara::STATE_SURF_TREAD ||
              laraState == Lara::STATE_SURF_SWIM))
    {
        //deadzone
        if (joy.length() > 0.8f)
        {
            mat4 addMat;
            addMat.identity();

            if (pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::CLASSIC)
            {
                Input::hmd.head.setRot((Input::hmd.body).getRot());
                if (joy.quadrant() == 2)
                {
                    //Forward - make lara "tilt" by including right thumbstick X value when turning
                    Input::setJoyPos(joyRight, jkL, vec2(usingSnapTurn ? 0 : rightTrackedRemoteState_new.Joystick.x, joy.y));
                } 
                else if (joy.quadrant() % 2 != 0)
                {
                    //Sideways
                    walkingEnabled = fabs(joy.x) > 0.6f;
                    Input::setJoyPos(joyRight, jkL, vec2(joy.x, 0));
                }
                else
                {
                    //Back
                    Input::setJoyPos(joyRight, jkL, vec2(0, joy.y));
                }
            }
            else
            {
                float additionalDirAngle = atan2(leftTrackedRemoteState_new.Joystick.x, leftTrackedRemoteState_new.Joystick.y);
                addMat.rotateY(-additionalDirAngle);

                if (Core::settings.detail.mixedRealityMode)
                {
                    addMat.rotateY(-Input::hmd.angleY);
                }

                Input::hmd.head.setRot((addMat * Input::hmd.body).getRot());

                Input::setJoyPos(joyRight, jkL, vec2(0, -joy.length()));
            }
        }
        else if (laraState == Lara::STATE_RUN ||
            laraState == Lara::STATE_WALK)
        {
            lara->animation.setAnim(Lara::ANIM_STAND);
            lara->state = Lara::STATE_STOP;
            Input::setJoyPos(joyRight, jkL, vec2(0));
        }
        else if (laraState == Lara::STATE_STOP ||
            laraState == Lara::STATE_SURF_TREAD ||
            laraState == Lara::STATE_SURF_SWIM)
        {
            if (!turning)
            {
                Input::setJoyPos(joyRight, jkL, vec2(0));
            }
        }
    }
    // If the user simply pressed the thumbstick in a particular direction that isn't forward
    // after already executing another move (like jump), then
    // we'll execute the move for that direction so treat it as a D pad
    else if (!inventory->isActive())
    {
        //now adjust movement direction based on thumbstick direction
        //deadzone
        vec2 joy(leftTrackedRemoteState_new.Joystick.x, leftTrackedRemoteState_new.Joystick.y);
        if (joy.length() > 0.5f)
        {
            //Calculate which quandrant the thumbstick is pushed (UP/RIGHT/DOWN/LEFT) like a D pad
            float angle = joy.quadrant() * PIH;
            joy.y = cosf(angle);
            joy.x = sinf(angle);

            if (Core::settings.detail.mixedRealityMode)
            {
                Input::setJoyPos(joyRight, jkL, vec2(joy.x, -joy.y).rotate(-Input::hmd.extrarot
                    -Controller::getAngleAbs(Input::hmd.head.dir().xyz()).y
                     +Input::hmd.angleY));
            }
            else if (povToyMode || (pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::MODERN))
            {
                Input::setJoyPos(joyRight, jkL, vec2(joy.x, -joy.y).rotate(-(Input::hmd.extrarot2) -Controller::getAngleAbs(Input::hmd.head.dir().xyz()).y));
            }
            else
            {
                Input::hmd.head.setRot(Input::hmd.body.getRot());
                Input::setJoyPos(joyRight, jkL, vec2(joy.x, -joy.y));
            }
        }
        else
        {
            if (!turning)
            {
                Input::setJoyPos(joyRight, jkL, vec2(0));
            }
        }
    }

    if (pov1stPerson ||
        (pov3rdPerson && Core::settings.detail.getCameraModeMode() == Core::CameraMode::CLASSIC))
    {
        if (laraState == Lara::STATE_STOP || laraState == Lara::STATE_DEATH)
        {
            Input::hmd.head.setRot(Input::hmd.body.getRot());
        }
    }

    //Inventory controls
    static bool AButtonActive = true; // This is used to suppress an A button event when you come out of the inventory so you don't immediately jump
    if (inventory->isActive() || inventory-> video)
    {
        Input::setJoyPos(joyRight, jkL, vec2((leftTrackedRemoteState_new.Joystick.x + rightTrackedRemoteState_new.Joystick.x), -(leftTrackedRemoteState_new.Joystick.y + rightTrackedRemoteState_new.Joystick.y)));
        Input::setJoyDown(joyRight, jkA, AButtonActive && (rightTrackedRemoteState_new.Buttons & xrButton_A));
        Input::setJoyDown(joyRight, jkSelect, (rightTrackedRemoteState_new.Buttons & xrButton_B) | (leftTrackedRemoteState_new.Buttons & xrButton_Y) | (leftTrackedRemoteState_new.Buttons & xrButton_Enter));
        if (rightTrackedRemoteState_new.Buttons & xrButton_A)
        {
            //Button was pressed, so disable A until it is released again
            AButtonActive = false;
        }
    }

    if (!AButtonActive && !(rightTrackedRemoteState_new.Buttons & xrButton_A))
    {
        AButtonActive = true;
    }

    bool twoHandShotgun = false;
    if (lara && !inventory->isActive())
    {
        if (lara->emptyHands() || !pov1stPerson || Core::settings.detail.autoaim)
        {
            //with empty hands left or right trigger is action
            Input::setJoyDown(joyRight, jkA, actionPressed);
        }
        else
        {
            //The only time joyLeft is used is to indicate the firing of the left hand weapon
            bool holster = rightTrackedRemoteState_new.Buttons & xrButton_B;
            Input::setJoyDown(joyLeft, jkA, !holster && (leftTrackedRemoteState_new.IndexTrigger > 0.4f ? 1 : 0));
            Input::setJoyDown(joyRight, jkA, !holster && (rightTrackedRemoteState_new.IndexTrigger > 0.4f ? 1 : 0));
        }

        //Walk
        Input::setDown(ikShift, walkingEnabled & 1);

        //Jump
        Input::setJoyDown(joyRight, jkX, AButtonActive && (rightTrackedRemoteState_new.Buttons & xrButton_A));

        //Holster/Unholster weapons
        Input::setJoyDown(joyRight, jkY, rightTrackedRemoteState_new.Buttons & xrButton_B);

        //Roll - Reverse Direction - Right thumbstick click
        Input::setJoyDown(joyRight, jkB, rightTrackedRemoteState_new.Buttons & xrButton_RThumb);
    }

    //Menu / Options
    static bool allowOptions = false;
    if (!allowOptions)
    {
        allowOptions = !((bool)(leftTrackedRemoteState_new.Buttons & xrButton_Enter));
    }
    else
    {
        if (leftTrackedRemoteState_new.Buttons & xrButton_Enter)
        {
            inventory->toggle(0, Inventory::PAGE_OPTION);
            allowOptions = false;
        }
    }

    const auto INVENTORY_GRIP_THRESHOLD = 0.8f;
    if (!Core::settings.detail.mixedRealityMode &&
        !Game::level->level.isTitle())
    {
        if ((!inventory->isActive() && rightTrackedRemoteState_new.GripTrigger > INVENTORY_GRIP_THRESHOLD &&
            rightTrackedRemoteState_old.GripTrigger <= INVENTORY_GRIP_THRESHOLD) ||
            ((inventory->isActive() || inventory->phaseRing > 0.f) && rightTrackedRemoteState_new.GripTrigger <= INVENTORY_GRIP_THRESHOLD &&
                rightTrackedRemoteState_old.GripTrigger > INVENTORY_GRIP_THRESHOLD))
        {
            inventory->toggle(0, Inventory::PAGE_INVENTORY);
        }
    }

    /*
        World orientation in mixed reality mode
    */
    if (Core::settings.detail.mixedRealityMode)
    {
        // easier just dumping these here as statics than creating member variables
        static vec3 left;
        static vec3 right;
        static vec3 cpos;
        static bool snap = false;
        static float angleY;
        if (!snap)
        {
            if (rightTrackedRemoteState_new.GripTrigger > 0.7f &&
                leftTrackedRemoteState_new.GripTrigger > 0.7f)
            {
                //Record the current controller locations
                right = vrRightControllerPosition;
                left = vrLeftControllerPosition;
                angleY = 0;
                cpos = vec3(0);

                vec3 anchor;
                anchor = (vrLeftControllerPosition + vrRightControllerPosition) / 2.0f;
                anchor = anchor.rotateY((-Input::hmd.extrarot));
                anchor = Input::hmd.head.getPos() - anchor;
                anchor *= ONE_METER * Input::hmd.extraworldscaler;
                anchor.y = 0;
                anchor.x *= -1;
                Input::hmd.mranchor = anchor;

                //Reset the origins
                Input::hmd.forceUpdatePose = true;

                snap = true;
            }
        }
        else
        {
            if (rightTrackedRemoteState_new.GripTrigger < 0.6f ||
                leftTrackedRemoteState_new.GripTrigger < 0.6f)
            {
                vec3 a = Input::hmd.mranchor;
                Input::hmd.mrorg += a;
                a = a.rotateY(angleY);
                Input::hmd.mrorg -= a;
                snap = false;
            }
            else
            {
                //World orienting bit..
                float angle = ((left - right).angleY() - (vrLeftControllerPosition - vrRightControllerPosition).angleY());
                angleY -= angle;
                Input::hmd.nextrot -= angle;

                vec3 poschange;
                poschange = (((left + right) / 2.0f) - ((vrLeftControllerPosition + vrRightControllerPosition) / 2.0f)) * ONE_METER * Input::hmd.extraworldscaler;
                poschange = poschange.rotateY((-Input::hmd.extrarot));
                poschange.x *= -1;
                Input::hmd.mrorg -= poschange;

                vec3 body = Input::hmd.body.getPos() * (ONE_METER * Input::hmd.extraworldscaler);
                vec3 a = Input::hmd.mranchor;
                Input::hmd.mrpos = Input::hmd.mrorg - body + a;
                a = a.rotateY(angleY);
                Input::hmd.mrpos -= a;

                Input::hmd.extraworldscaler += ((left - right).length() - (vrLeftControllerPosition - vrRightControllerPosition).length()) * 3.f * Input::hmd.extraworldscaler;
                Input::hmd.extraworldscaler = clamp(Input::hmd.extraworldscaler, 1.0f, 100.0f);

                //Record the current controller locations
                right = vrRightControllerPosition;
                left = vrLeftControllerPosition;
            }
        }
    }

    static bool allowSaveLoad = false;
    if (!allowSaveLoad)
    {
        allowSaveLoad = !((bool)(leftTrackedRemoteState_new.Buttons & xrButton_Y));
    }
    else
    {
        if (!inventory->isActive())
        {
            if (leftTrackedRemoteState_new.Buttons & xrButton_Y)
            {
                inventory->toggle(0, Inventory::PAGE_OPTION, TR::Entity::INV_PASSPORT);
                allowSaveLoad = false;
            }
        }
    }

    if (lara && !inventory->active && !Core::settings.detail.mixedRealityMode)
    {
        vec2 rightJoy(rightTrackedRemoteState_new.Joystick.x, rightTrackedRemoteState_new.Joystick.y);
        int sector = rightJoy.sector(8);
        static bool allowTogglePerspective = false;
        if (!allowTogglePerspective)
        {
            if (rightJoy.length() < 0.6f)
            {
                allowTogglePerspective = true;
            }
        }
        else
        {
            if (rightJoy.length() > 0.7f)
            {
                if (sector == 4)
                {
                    lara->camera->changeView(true);
                }
                else if (sector == 0)
                {
                    lara->camera->changeView(false);
                }

                allowTogglePerspective = false;
            }
        }
    }

    if (!Core::settings.detail.mixedRealityMode)
    {
        Input::hmd.extraworldscaler = povToyMode ? 12.0f : 1.0f;
    }

    if (cheatsEnabled)
    {
        static int speed = 0;
        Input::setDown(ikT, speed & 1);
        Input::setDown(ikR, speed & 2);

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
                        //cycle through speeds
                        speed++;
                        if (speed == 3) speed = 0;
                    }

                    allowToggleCheat = false;
                }
            }
        }
    }

    float rotation = 45.0f;
    if (gAppState.controllersPresent == VIVE_CONTROLLERS)
    {
        rotation += -33.6718750f;
    }

    vrRightControllerPosition = vrRightControllerPosition.rotateY(-Input::hmd.extrarot);
    vrLeftControllerPosition = vrLeftControllerPosition.rotateY(-Input::hmd.extrarot);

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

    //Calc this here for the next frame
    movingBackwards = Input::hmd.head.getRot().dot(Input::hmd.body.getRot()) < 0.5f;


    //keep old state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}

#ifdef ANDROID

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

#else
	

#ifdef _DEBUG
int main(int argc, char** argv) {
#else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc = (lpCmdLine && strlen(lpCmdLine)) ? 2 : 1;
    const char* argv[] = { "", lpCmdLine };
#endif
    cacheDir[0] = saveDir[0] = contentDir[0] = 0;

    strcat(cacheDir, getenv("APPDATA"));
    strcat(cacheDir, "\\BeefRaiderXR\\");
    strcpy(saveDir, cacheDir);
    CreateDirectory(cacheDir, NULL);

    //if (false)
    {
        int ox = (GetSystemMetrics(SM_CXSCREEN) - (windowSize.right - windowSize.left)) / 2;
        int oy = (GetSystemMetrics(SM_CYSCREEN) - (windowSize.bottom - windowSize.top)) / 2;
        windowSize.left += ox;
        windowSize.top += oy;
        windowSize.right += ox;
        windowSize.bottom += oy;
    }


    hWnd = CreateWindow("static", "BeefRaiderXR", WS_OVERLAPPEDWINDOW, windowSize.left, windowSize.top, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, 0, 0, 0, 0);
    AdjustWindowRect(&windowSize, WS_OVERLAPPEDWINDOW, false);
    SendMessage(hWnd, WM_SETICON, 1, (LPARAM)LoadIcon(GetModuleHandle(NULL), "MAINICON"));

    ContextCreate();

    Sound::channelsCount = 0;

    osStartTime = Core::getTime();
    sndInit(hWnd);

    Core::defLang = checkLanguage();

    if (argc > 1 && std::string(argv[1]) == "--cheats")
    {
        cheatsEnabled = true;
    }

    Game::init((argc > 1 && !cheatsEnabled) ? argv[1] : NULL);

    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&WndProc);

    if (Core::isQuit) {
        MessageBoxA(hWnd, "Please check the readme file first!", "Game resources not found", MB_ICONHAND);
    }
    else {
        ShowWindow(hWnd, SW_SHOWDEFAULT);
    }

    MSG msg;

    while (!Core::isQuit) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                Core::quit();
        }
        else {
            TBXR_FrameSetup();
            if (Game::update()) {
                Game::render();
                Core::waitVBlank();

                TBXR_submitFrame();

                ContextSwap();
            }
        }
    };

    TBXR_LeaveVR();

    sndFree();
    Game::deinit();

    ContextDelete();

    DestroyWindow(hWnd);

    return 0;
}

#endif
