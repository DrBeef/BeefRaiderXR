/************************************************************************************

Content		:	Handles common controller input functionality
Created		:	September 2019
Authors		:	Simon Brown

*************************************************************************************/

#include "VrInput.h"

ovrInputStateTrackedRemote leftTrackedRemoteState_old;
ovrInputStateTrackedRemote leftTrackedRemoteState_new;
ovrTrackedController leftRemoteTracking_new;
ovrInputStateTrackedRemote rightTrackedRemoteState_old;
ovrInputStateTrackedRemote rightTrackedRemoteState_new;
ovrTrackedController rightRemoteTracking_new;

float remote_movementSideways;
float remote_movementForward;
float remote_movementUp;
float positional_movementSideways;
float positional_movementForward;
long long global_time;
int ducked;
vr_client_info_t vr;

extern ovrApp gAppState;


//keys.h
void handleTrackedControllerButton(ovrInputStateTrackedRemote * trackedRemoteState, ovrInputStateTrackedRemote * prevTrackedRemoteState, uint32_t button, int key)
{
    if ((trackedRemoteState->Buttons & button) != (prevTrackedRemoteState->Buttons & button))
    {
        //Sys_QueEvent( 0, SE_KEY, key, (trackedRemoteState->Buttons & button) != 0, 0, NULL );
    }
}

float length(float x, float y)
{
    return sqrtf(powf(x, 2.0f) + powf(y, 2.0f));
}

void sendButtonActionSimple(const char* action)
{
    char command[256];
    snprintf( command, sizeof( command ), "%s\n", action );
   // Cbuf_AddText( command );
}

bool between(float min, float val, float max)
{
    return (min < val) && (val < max);
}

void sendButtonAction(const char* action, long buttonDown)
{
    char command[256];
    snprintf( command, sizeof( command ), "%s\n", action );
    if (!buttonDown)
    {
        command[0] = '-';
    }
    //Cbuf_AddText( command );
}


float clamp(float _min, float _val, float _max)
{
    return fmax(fmin(_val, _max), _min);
}