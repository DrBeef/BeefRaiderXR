/************************************************************************************

Filename	:	VrInputDefault.c
Content		:	Handles default controller input
Created		:	August 2019
Authors		:	Simon Brown

*************************************************************************************/

#include "VrInput.h"
#include "VrCvars.h"

#ifndef _WIN32
#include <android/keycodes.h>
#endif

#define A_ESCAPE -1
#define A_MOUSE1 -2

void HandleInput_Default( ovrInputStateTrackedRemote *pDominantTrackedRemoteNew, ovrInputStateTrackedRemote *pDominantTrackedRemoteOld, ovrTrackedController* pDominantTracking,
                          ovrInputStateTrackedRemote *pOffTrackedRemoteNew, ovrInputStateTrackedRemote *pOffTrackedRemoteOld, ovrTrackedController* pOffTracking,
                          int domButton1, int domButton2, int offButton1, int offButton2 )

{
	//Ensure handedness is set correctly
	vr.right_handed = vr_control_scheme->value < 10 ||
            vr_control_scheme->value == 99; // Always right-handed for weapon calibration

    static bool dominantGripPushed = false;

    //Need this for the touch screen
    ovrTrackedController * pWeapon = pDominantTracking;
    ovrTrackedController * pOff = pOffTracking;

    //All this to allow stick and button switching!
    XrVector2f *pPrimaryJoystick;
    XrVector2f *pSecondaryJoystick;
    uint32_t primaryButtonsNew;
    uint32_t primaryButtonsOld;
    uint32_t secondaryButtonsNew;
    uint32_t secondaryButtonsOld;
    int primaryButton1;
    bool primaryButton2New;
    bool primaryButton2Old;
    int secondaryButton1;
    bool secondaryButton2New;
    bool secondaryButton2Old;
    bool secondaryButton1New;
    bool secondaryButton1Old;
    int primaryThumb;
    int secondaryThumb;
    if (vr_control_scheme->integer == LEFT_HANDED_DEFAULT && vr_switch_sticks->integer)
    {
        primaryThumb = xrButton_RThumb;
        secondaryThumb = xrButton_LThumb;
    }
    else if (vr_control_scheme->integer == LEFT_HANDED_DEFAULT || vr_switch_sticks->integer)
    {
        primaryThumb = xrButton_LThumb;
        secondaryThumb = xrButton_RThumb;
    }
    else
    {
        primaryThumb = xrButton_RThumb;
        secondaryThumb = xrButton_LThumb;
    }

    if (vr_switch_sticks->integer)
    {
        //
        // This will switch the joystick and A/B/X/Y button functions only
        // Move, Strafe, Turn, Jump, Crouch, Notepad, HUD mode, Weapon Switch
        pSecondaryJoystick = &pDominantTrackedRemoteNew->Joystick;
        pPrimaryJoystick = &pOffTrackedRemoteNew->Joystick;
        secondaryButtonsNew = pDominantTrackedRemoteNew->Buttons;
        secondaryButtonsOld = pDominantTrackedRemoteOld->Buttons;
        primaryButtonsNew = pOffTrackedRemoteNew->Buttons;
        primaryButtonsOld = pOffTrackedRemoteOld->Buttons;
        primaryButton1 = offButton1;
        secondaryButton1 = domButton1;
    }
    else
    {
        pPrimaryJoystick = &pDominantTrackedRemoteNew->Joystick;
        pSecondaryJoystick = &pOffTrackedRemoteNew->Joystick;
        primaryButtonsNew = pDominantTrackedRemoteNew->Buttons;
        primaryButtonsOld = pDominantTrackedRemoteOld->Buttons;
        secondaryButtonsNew = pOffTrackedRemoteNew->Buttons;
        secondaryButtonsOld = pOffTrackedRemoteOld->Buttons;
        primaryButton1 = domButton1;
        secondaryButton1 = offButton1;
    }

    //Don't switch B/Y buttons even if switched sticks
    primaryButton2New = domButton2 & pDominantTrackedRemoteNew->Buttons;
    primaryButton2Old = domButton2 & pDominantTrackedRemoteOld->Buttons;
    secondaryButton2New = offButton2 & pOffTrackedRemoteNew->Buttons;
    secondaryButton2Old = offButton2 & pOffTrackedRemoteOld->Buttons;
    secondaryButton1New = offButton1 & pOffTrackedRemoteNew->Buttons;
    secondaryButton1Old = offButton1 & pOffTrackedRemoteOld->Buttons;

    //Allow weapon alignment mode toggle on x
/*    if (vr_align_weapons->value)
    {
        bool offhandX = (pOffTrackedRemoteNew->Buttons & xrButton_X);
        if ((offhandX != (pOffTrackedRemoteOld->Buttons & xrButton_X)) &&
                offhandX)
        Cvar_Set("vr_control_scheme", "99");
    }*/

    static int cinCameraTimestamp = -1;
    if (vr.cin_camera && cinCameraTimestamp == -1) {
        cinCameraTimestamp = Sys_Milliseconds();
    }
    else if (!vr.cin_camera) {
        cinCameraTimestamp = -1;
    }

    //Set controller angles - We need to calculate all those we might need (including adjustments) for the client to then take its pick
    {
        vec3_t rotation = {0};
        QuatToYawPitchRoll(pWeapon->Pose.orientation, rotation, vr.weaponangles[ANGLES_DEFAULT]);
        QuatToYawPitchRoll(pOff->Pose.orientation, rotation, vr.offhandangles[ANGLES_DEFAULT]);

        //If we are in a saberBlockDebounce thing then add on an angle
        //Lerped upon how far from the start of the saber move 
        //Index default -> vr_saber_pitchadjust->value = -2.42187500
        //Vive Default -> 0.312500000

        //Individual Controller offsets (so that they match quest)
        if (gAppState.controllersPresent == INDEX_CONTROLLERS)
        {
            rotation[PITCH] += 10.938125f;
        }
        else if (gAppState.controllersPresent == VIVE_CONTROLLERS)
        {
            rotation[PITCH] += 13.6725f;
        }
        else if (gAppState.controllersPresent == PICO_CONTROLLERS)
        {
            rotation[PITCH] += 12.500625f;        
        }

        QuatToYawPitchRoll(pWeapon->GripPose.orientation, rotation, vr.weaponangles[ANGLES_SABER]);
        QuatToYawPitchRoll(pOff->GripPose.orientation, rotation, vr.offhandangles[ANGLES_SABER]);
        // + (gAppState.controllersPresent == INDEX_CONTROLLERS ? -35
        //VIVE CONTROLLERS -> -33.6718750
        rotation[PITCH] = vr_weapon_pitchadjust->value;
        if (gAppState.controllersPresent == VIVE_CONTROLLERS)
        {
            rotation[PITCH] -= 33.6718750f;
        }
        QuatToYawPitchRoll(pWeapon->Pose.orientation, rotation, vr.weaponangles[ANGLES_ADJUSTED]);
        QuatToYawPitchRoll(pOff->Pose.orientation, rotation, vr.offhandangles[ANGLES_ADJUSTED]);

        for (int anglesIndex = 0; anglesIndex <= ANGLES_SABER; ++anglesIndex)
        {
            VectorSubtract(vr.weaponangles_last[anglesIndex], vr.weaponangles[anglesIndex], vr.weaponangles_delta[anglesIndex]);
            VectorCopy(vr.weaponangles[anglesIndex], vr.weaponangles_last[anglesIndex]);

            VectorSubtract(vr.offhandangles_last[anglesIndex], vr.offhandangles[anglesIndex], vr.offhandangles_delta[anglesIndex]);
            VectorCopy(vr.offhandangles[anglesIndex], vr.offhandangles_last[anglesIndex]);
        }
    }

    //Menu button
	handleTrackedControllerButton(&leftTrackedRemoteState_new, &leftTrackedRemoteState_old, xrButton_Enter, A_ESCAPE);
	handleTrackedControllerButton(&rightTrackedRemoteState_new, &rightTrackedRemoteState_old, xrButton_Enter, A_ESCAPE);

    static float menuYaw = 0;
    if (VR_UseScreenLayer() && !vr.misc_camera)
    {
        if (vr.cin_camera && cinCameraTimestamp + 1000 < Sys_Milliseconds())
        {
            // To skip cinematic use any thumb or trigger (but wait a while
            // to prevent skipping when cinematic is started during action)
            if ((primaryButtonsNew & primaryThumb) != (primaryButtonsOld & primaryThumb)) {
                sendButtonAction("+use", (primaryButtonsNew & primaryThumb));
            }
            if ((secondaryButtonsNew & secondaryThumb) != (secondaryButtonsOld & secondaryThumb)) {
                sendButtonAction("+use", (secondaryButtonsNew & secondaryThumb));
            }
            if ((pDominantTrackedRemoteNew->Buttons & xrButton_Trigger) != (pDominantTrackedRemoteOld->Buttons & xrButton_Trigger)) {
                sendButtonAction("+use", (pDominantTrackedRemoteNew->Buttons & xrButton_Trigger));
                // mark button as already pressed to prevent firing after entering the game
                pDominantTrackedRemoteOld->Buttons |= xrButton_Trigger;
            }
            if ((pOffTrackedRemoteNew->Buttons & xrButton_Trigger) != (pOffTrackedRemoteOld->Buttons & xrButton_Trigger)) {
                sendButtonAction("+use", (pOffTrackedRemoteNew->Buttons & xrButton_Trigger));
                // mark button as already pressed to prevent firing after entering the game
                pOffTrackedRemoteOld->Buttons |= xrButton_Trigger;
            }
        }

        bool controlsLeftHanded = vr_control_scheme->integer >= 10;
        if (controlsLeftHanded == vr.menu_right_handed) {
            handleTrackedControllerButton(pOffTrackedRemoteNew, pOffTrackedRemoteOld, offButton1, A_MOUSE1);
            handleTrackedControllerButton(pOffTrackedRemoteNew, pOffTrackedRemoteOld, xrButton_Trigger, A_MOUSE1);
            handleTrackedControllerButton(pOffTrackedRemoteNew, pOffTrackedRemoteOld, offButton2, A_ESCAPE);
            if ((pDominantTrackedRemoteNew->Buttons & xrButton_Trigger) != (pDominantTrackedRemoteOld->Buttons & xrButton_Trigger) && (pDominantTrackedRemoteNew->Buttons & xrButton_Trigger)) {
                vr.menu_right_handed = !vr.menu_right_handed;
            }
        } else {
            handleTrackedControllerButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, domButton1, A_MOUSE1);
            handleTrackedControllerButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, xrButton_Trigger, A_MOUSE1);
            handleTrackedControllerButton(pDominantTrackedRemoteNew, pDominantTrackedRemoteOld, domButton2, A_ESCAPE);
            if ((pOffTrackedRemoteNew->Buttons & xrButton_Trigger) != (pOffTrackedRemoteOld->Buttons & xrButton_Trigger) && (pOffTrackedRemoteNew->Buttons & xrButton_Trigger)) {
                vr.menu_right_handed = !vr.menu_right_handed;
            }
        }

        //Close the datapad
        if (secondaryButton2New && !secondaryButton2Old) {
                //Sys_QueEvent(0, SE_KEY, A_TAB, true, 0, NULL);
        }

        //Close the menu
        if (secondaryButton1New && !secondaryButton1Old) {
                //Sys_QueEvent(0, SE_KEY, A_ESCAPE, true, 0, NULL);
        }

    }
    else
    {
        menuYaw = vr.hmdorientation[YAW];

        float distance = sqrtf(powf(pOff->Pose.position.x - pWeapon->Pose.position.x, 2) +
                               powf(pOff->Pose.position.y - pWeapon->Pose.position.y, 2) +
                               powf(pOff->Pose.position.z - pWeapon->Pose.position.z, 2));

        float distanceToHMD = sqrtf(powf(vr.hmdposition[0] - pWeapon->Pose.position.x, 2) +
                                    powf(vr.hmdposition[1] - pWeapon->Pose.position.y, 2) +
                                    powf(vr.hmdposition[2] - pWeapon->Pose.position.z, 2));

        float distanceToHMDOff = sqrtf(powf(vr.hmdposition[0] - pOff->Pose.position.x, 2) +
                                    powf(vr.hmdposition[1] - pOff->Pose.position.y, 2) +
                                    powf(vr.hmdposition[2] - pOff->Pose.position.z, 2));


        float controllerYawHeading = 0.0f;


        dominantGripPushed = (pDominantTrackedRemoteNew->Buttons &
                              xrButton_GripTrigger) != 0;

        //Do this early so we can suppress other button actions when item selector is up
        if (dominantGripPushed) {
            if (!vr.weapon_stabilised && vr.item_selector == 0
                && !vr.misc_camera && !vr.cgzoommode) {
                vr.item_selector = 1;
            }
        }
        else if (vr.item_selector == 1)
        {
            sendButtonActionSimple("itemselectorselect");
            vr.item_selector = 0;
        }



#define JOYX_SAMPLE_COUNT   4
        static float joyx[JOYX_SAMPLE_COUNT] = {0};
        for (int j = JOYX_SAMPLE_COUNT - 1; j > 0; --j)
            joyx[j] = joyx[j - 1];
        joyx[0] = pPrimaryJoystick->x;
        float sum = 0.0f;
        for (int j = 0; j < JOYX_SAMPLE_COUNT; ++j)
            sum += joyx[j];
        float primaryJoystickX = sum / 4.0f;


        //Left/right to switch between which selector we are using
        if (vr.item_selector == 1) {
            float x, y;
            if (vr_switch_sticks->integer)
            {
                x = pSecondaryJoystick->x;
                y = pSecondaryJoystick->y;
            }
            else
            {
                x = pPrimaryJoystick->x;
                y = pPrimaryJoystick->y;
            }
            static bool selectorSwitched = false;
            if (between(-0.2f, y, 0.2f) &&
                (x > 0.8f || x < -0.8f)) {

                if (!selectorSwitched) {
                    if (x > 0.8f) {
                        sendButtonActionSimple("itemselectornext");
                        selectorSwitched = true;
                    } else if (x < -0.8f) {
                        sendButtonActionSimple("itemselectorprev");
                        selectorSwitched = true;
                    }
                }
            } else if (between(-0.4f, primaryJoystickX, 0.4f)) {
                selectorSwitched = false;
            }
        }

        //Left/right to switch between which selector we are using
        if (vr.item_selector == 2) {
            float x, y;
            if (vr_switch_sticks->integer)
            {
                x = pPrimaryJoystick->x;
                y = pPrimaryJoystick->y;
            }
            else
            {
                x = pSecondaryJoystick->x;
                y = pSecondaryJoystick->y;
            }
            static bool selectorSwitched = false;
            if (between(-0.2f, y, 0.2f) &&
                (x > 0.8f || x < -0.8f)) {

                if (!selectorSwitched) {
                    if (x > 0.8f) {
                        sendButtonActionSimple("itemselectornext");
                        selectorSwitched = true;
                    } else if (x < -0.8f) {
                        sendButtonActionSimple("itemselectorprev");
                        selectorSwitched = true;
                    }
                }
            } else if (between(-0.4f, x, 0.4f)) {
                selectorSwitched = false;
            }
        }

        if (vr.cin_camera && cinCameraTimestamp + 1000 < Sys_Milliseconds())
        {
            // To skip cinematic use any thumb or trigger (but wait a while
            // to prevent skipping when cinematic is started during action)
            if ((primaryButtonsNew & primaryThumb) != (primaryButtonsOld & primaryThumb)) {
                sendButtonAction("+use", (primaryButtonsNew & primaryThumb));
            }
            if ((secondaryButtonsNew & secondaryThumb) != (secondaryButtonsOld & secondaryThumb)) {
                sendButtonAction("+use", (secondaryButtonsNew & secondaryThumb));
            }
            if ((pDominantTrackedRemoteNew->Buttons & xrButton_Trigger) != (pDominantTrackedRemoteOld->Buttons & xrButton_Trigger)) {
                sendButtonAction("+use", (pDominantTrackedRemoteNew->Buttons & xrButton_Trigger));
                // mark button as already pressed to prevent firing after entering the game
                pDominantTrackedRemoteOld->Buttons |= xrButton_Trigger;
            }
            if ((pOffTrackedRemoteNew->Buttons & xrButton_Trigger) != (pOffTrackedRemoteOld->Buttons & xrButton_Trigger)) {
                sendButtonAction("+use", (pOffTrackedRemoteNew->Buttons & xrButton_Trigger));
                // mark button as already pressed to prevent firing after entering the game
                pOffTrackedRemoteOld->Buttons |= xrButton_Trigger;
            }
        }
        else if (vr.misc_camera && !vr.remote_droid)
        {
            if (between(-0.2f, primaryJoystickX, 0.2f)) {
                sendButtonAction("+use", pPrimaryJoystick->y < -0.8f || pPrimaryJoystick->y > 0.8f);
            }
        }
        else if (vr.cgzoommode)
        {
            if (between(-0.2f, primaryJoystickX, 0.2f)) {
                if (vr.cgzoommode == 2)
                { // We are in disruptor scope
                    if (pPrimaryJoystick->y > 0.8f) {
                        vr.cgzoomdir = -1; // zooming in
                        sendButtonAction("+altattack", true);
                    } else if (pPrimaryJoystick->y < -0.8f) {
                        vr.cgzoomdir = 1; // zooming out
                        sendButtonAction("+altattack", true);
                    } else {
                        sendButtonAction("+altattack", false);
                    }
                }
                else if (vr.cgzoommode == 1)
                { // We are in binoculars scope - zoom in or out
                  sendButtonAction("+attack", pPrimaryJoystick->y > 0.8f);
                  sendButtonAction("+altattack", pPrimaryJoystick->y < -0.8f);
                }
                // No function of thumbstick for nightvision (3) or blaster scope (4)
            }
        }
        else
        {
            int mode = (int)Cvar_VariableValue("cg_thirdPerson");
            if (mode != 0)
            {
                sendButtonActionSimple("cg_thirdPerson 0");
            }
        }

        //Switch movement speed
        if (!vr.cgzoommode && !vr_always_run->integer)
        {
            static bool switched = false;
            if (between(-0.2f, primaryJoystickX, 0.2f) &&
                between(0.8f, pPrimaryJoystick->y, 1.0f)) {
                if (!switched) {
                    vr.move_speed = (++vr.move_speed) % 3;
                    switched = true;
                }
            }
            else {
                switched = false;
            }
        }

        /*
        //Parameter Changer
         static bool changed = false;
        if (between(-0.2f, primaryJoystickX, 0.2f) &&
            between(0.8f, pPrimaryJoystick->y, 1.0f)) {
            if(!changed) {
                vr.tempWeaponVelocity += 25;
                changed = true;
                ALOGV("**TBDC** Projectile speed %f",vr.tempWeaponVelocity);
            }
       } else if (between(-0.2f, primaryJoystickX, 0.2f) &&
                    between(-1.0f, pPrimaryJoystick->y, -0.8f)) {
            if(!changed) {
                vr.tempWeaponVelocity -= 25;
                ALOGV("**TBDC** Projectile speed %f",vr.tempWeaponVelocity);
                changed = true;
            }
        }
        else
        {
            changed = false;
        }*/

        //dominant hand stuff first
        {
            //Record recent weapon position for trajectory based stuff
            for (int i = (NUM_WEAPON_SAMPLES - 1); i != 0; --i) {
                VectorCopy(vr.weaponoffset_history[i - 1], vr.weaponoffset_history[i]);
                vr.weaponoffset_history_timestamp[i] = vr.weaponoffset_history_timestamp[i - 1];
            }
            VectorCopy(vr.weaponoffset, vr.weaponoffset_history[0]);
            vr.weaponoffset_history_timestamp[0] = vr.weaponoffset_timestamp;


            VectorSet(vr.weaponposition, pWeapon->Pose.position.x,
                      pWeapon->Pose.position.y, pWeapon->Pose.position.z);

            ///Weapon location relative to view
            VectorSet(vr.weaponoffset, pWeapon->Pose.position.x,
                      pWeapon->Pose.position.y, pWeapon->Pose.position.z);
            VectorSubtract(vr.weaponoffset, vr.hmdposition, vr.weaponoffset);
            vr.weaponoffset_timestamp = Sys_Milliseconds();


            vec3_t velocity;
            VectorSet(velocity, pWeapon->Velocity.linearVelocity.x,
                      pWeapon->Velocity.linearVelocity.y, pWeapon->Velocity.linearVelocity.z);
            //vr.primaryswingvelocity = VectorLength(velocity);

            VectorSet(velocity, pOff->Velocity.linearVelocity.x,
                      pOff->Velocity.linearVelocity.y, pOff->Velocity.linearVelocity.z);
            //vr.secondaryswingvelocity = VectorLength(velocity);


            //off-hand stuff (done here as I reference it in the save state thing
            {
                for (int i = 4; i > 0; --i)
                {
                    VectorCopy(vr.offhandposition[i-1], vr.offhandposition[i]);
                }
                vr.offhandposition[0][0] = pOff->Pose.position.x;
                vr.offhandposition[0][1] = pOff->Pose.position.y;
                vr.offhandposition[0][2] = pOff->Pose.position.z;

                vr.offhandoffset[0] = pOff->Pose.position.x - vr.hmdposition[0];
                vr.offhandoffset[1] = pOff->Pose.position.y - vr.hmdposition[1];
                vr.offhandoffset[2] = pOff->Pose.position.z - vr.hmdposition[2];

                if (vr_walkdirection->value == 0) {
                    controllerYawHeading = vr.offhandangles[ANGLES_ADJUSTED][YAW] - vr.hmdorientation[YAW];
                } else {
                    controllerYawHeading = 0.0f;
                }
            }

        }

        //Right-hand specific stuff
        {
            //This section corrects for the fact that the controller actually controls direction of movement, but we want to move relative to the direction the
            //player is facing for positional tracking

            //Positional movement speed correction for when we are not hitting target framerate
            static double lastframetime = 0;
            int refresh = TBXR_GetRefresh();
            double newframetime = Sys_Milliseconds();
            float multiplier = (float) ((1000.0 / refresh) / (newframetime - lastframetime));
            lastframetime = newframetime;

            vec2_t v;
            float factor = (refresh / 72.0F) *
                           vr_positional_factor->value; // adjust positional factor based on refresh rate
            rotateAboutOrigin(-vr.hmdposition_delta[0] * factor * multiplier,
                              vr.hmdposition_delta[2] * factor * multiplier,
                              -vr.hmdorientation[YAW], v);
            positional_movementSideways = v[0];
            positional_movementForward = v[1];

              //Jump (A Button)
            if ((primaryButtonsNew & primaryButton1) != (primaryButtonsOld & primaryButton1)) {
                sendButtonAction("+moveup", (primaryButtonsNew & primaryButton1));
            }

            //B Button
            if (primaryButton2New != primaryButton2Old) {
                {
                    sendButtonAction("+altattack", primaryButton2New);
                }
            }

            //Duck - off hand joystick
            if ((secondaryButtonsNew & secondaryThumb) !=
                (secondaryButtonsOld & secondaryThumb)) {

                if (vr_crouch_toggle->integer)
                {
                    if (secondaryButtonsOld & secondaryThumb) {
                        vr.crouched = !vr.crouched;
                        sendButtonAction("+movedown", vr.crouched);
                    }
                }
                else
                {
                    sendButtonAction("+movedown", (secondaryButtonsNew & secondaryThumb));
                }

                // Reset max height for IRL crouch
                vr.maxHeight = 0;
            }

            //Use
            if ((primaryButtonsNew & primaryThumb) !=
                (primaryButtonsOld & primaryThumb)) {

                sendButtonAction("+use", (primaryButtonsNew & primaryThumb));
            }
        }

        {
            //Apply a filter and quadratic scaler so small movements are easier to make
            float dist = 0;
            if (vr.item_selector != (vr_switch_sticks->integer ? 1 : 2))
            {
                dist = length(pSecondaryJoystick->x, pSecondaryJoystick->y);
            }
            float nlf = nonLinearFilter(dist);
            dist = (dist > 1.0f) ? dist : 1.0f;
            float x = nlf * (pSecondaryJoystick->x / dist);
            float y = nlf * (pSecondaryJoystick->y / dist);

            vr.player_moving = (fabs(x) + fabs(y)) > 0.05f;

            //Adjust to be off-hand controller oriented
            vec2_t v;
            rotateAboutOrigin(x, y, controllerYawHeading, v);

            float move_speed_multiplier = 1.0f;
            switch (vr.move_speed)
            {
            case 0:
                move_speed_multiplier = 0.75f;
                break;
            case 1:
                move_speed_multiplier = 1.0f;
                break;
            case 2:
                move_speed_multiplier = 0.5f;
                break;
            }

            if (vr_always_run->integer)
            {
                move_speed_multiplier = 1.0f;
            }

            remote_movementSideways = move_speed_multiplier * v[0];
            remote_movementForward = move_speed_multiplier * v[1];
            

            //X button invokes menu now
            if ((secondaryButtonsNew & secondaryButton1) &&
                !(secondaryButtonsOld & secondaryButton1))
            {
                //Sys_QueEvent(0, SE_KEY, A_ESCAPE, true, 0, NULL);
            }

            //Open the datapad
            if (secondaryButton2New && !secondaryButton2Old) {
                //Sys_QueEvent(0, SE_KEY, A_TAB, true, 0, NULL);
            }

            //Use Force - off hand trigger
            {
                if ((pOffTrackedRemoteNew->Buttons & xrButton_Trigger) !=
                    (pOffTrackedRemoteOld->Buttons & xrButton_Trigger))
                {
                    sendButtonAction("+useforce", (pOffTrackedRemoteNew->Buttons & xrButton_Trigger));
                }
            }

            //Use smooth in 3rd person
            bool usingSnapTurn = vr_turn_mode->integer == 0 ||
                    (!vr.third_person && vr_turn_mode->integer == 1);

            float previousSnap = vr.snapTurn;
            static int increaseSnap = true;
            if (!vr.item_selector) {
                if (usingSnapTurn) {
                    if (primaryJoystickX > 0.7f) {
                        if (increaseSnap) {
                            vr.snapTurn -= vr_turn_angle->value;
                            increaseSnap = false;
                            if (vr.snapTurn < -180.0f) {
                                vr.snapTurn += 360.f;
                            }
                        }
                    } else if (primaryJoystickX < 0.3f) {
                        increaseSnap = true;
                    }
                }

                static int decreaseSnap = true;
                if (usingSnapTurn) {
                    if (primaryJoystickX < -0.7f) {
                        if (decreaseSnap) {
                            vr.snapTurn += vr_turn_angle->value;
                            decreaseSnap = false;

                            if (vr.snapTurn > 180.0f) {
                                vr.snapTurn -= 360.f;
                            }
                        }
                    } else if (primaryJoystickX > -0.3f) {
                        decreaseSnap = true;
                    }
                }

                if (!usingSnapTurn && fabs(primaryJoystickX) > 0.1f) //smooth turn
                {
                    vr.snapTurn -= ((vr_turn_angle->value / 10.0f) *
                                    primaryJoystickX);
                    if (vr.snapTurn > 180.0f) {
                        vr.snapTurn -= 360.f;
                    }
                }
            } else {
                if (fabs(primaryJoystickX) > 0.5f) {
                    increaseSnap = false;
                } else {
                    increaseSnap = true;
                }
            }
        }
    }


    //Save state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}
