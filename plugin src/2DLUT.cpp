/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*	Skeleton.cpp	

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to 
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017

*/

#include "2DLUT.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>


#include <stdio.h>
#include <stdlib.h>
using namespace std;

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc

	out_data->out_flags2 = PF_OutFlag2_FLOAT_COLOR_AWARE |
		PF_OutFlag2_SUPPORTS_SMART_RENDER |PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	
	out_data->num_params = SKELETON_NUM_PARAMS;

	return err;
}

PF_FpLong clamp32(PF_FpLong input) {
	if (input < 0) return 0.0;
	if (input > 1.0) return 1.0;
	return input;
	
}

int LUTLength = 100000;

static PF_Err
ApplyLUT32(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel32* inP,
	PF_Pixel32* outP)
{
	PF_Err		err = PF_Err_NONE;

	LUTInfo* giP = reinterpret_cast<LUTInfo*>(refcon);

	int upper, lower;
	if (floor(clamp32(inP->red) / 1.0 * LUTLength) != clamp32(inP->red) / 1.0 * LUTLength) {
		// if our floor doesn't equal the non floored input, we interpolate, else, use the floored value
		lower = floor(clamp32(inP->red) / 1.0 * LUTLength);
		upper = floor(clamp32(inP->red) / 1.0 * LUTLength) + 1;
		outP->red = (giP->LUTArray[lower] + giP->LUTArray[upper]) / 2;
	}
	else {
		outP->red = giP->LUTArray[lower];
	}

	if (floor(clamp32(inP->green) / 1.0 * LUTLength) != clamp32(inP->green) / 1.0 * LUTLength) {
		// if our floor doesn't equal the non floored input, we interpolate, else, use the floored value
		lower = floor(clamp32(inP->green) / 1.0 * LUTLength);
		upper = floor(clamp32(inP->green) / 1.0 * LUTLength) + 1;
		outP->green = (giP->LUTArray[lower] + giP->LUTArray[upper]) / 2;
	}
	else {
		outP->green = giP->LUTArray[lower];
	}

	if (floor(clamp32(inP->blue) / 1.0 * 10000) != clamp32(inP->blue) / 1.0 * LUTLength) {
		// if our floor doesn't equal the non floored input, we interpolate, else, use the floored value
		lower = floor(clamp32(inP->blue) / 1.0 * LUTLength);
		upper = floor(clamp32(inP->blue) / 1.0 * LUTLength) + 1;
		outP->blue = (giP->LUTArray[lower] + giP->LUTArray[upper]) / 2;
	}
	else {
		outP->blue = giP->LUTArray[lower];
	}

	
	outP->alpha = 1.0;
		

	return err;
}

static PF_Err
ActuallyRender(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_LayerDef* output,
	PF_EffectWorld* input,
	PF_ParamDef* params[])
{
	PF_Err				err = PF_Err_NONE;
	PF_Err				err2 = PF_Err_NONE;
	PF_PixelFormat format = PF_PixelFormat_INVALID;
	PF_WorldSuite2* wsP = NULL;
	ofstream myfile;


	// changing the gradient every frame actually creates a sort of film flicker
	//randomPower = RandomFloat(.5, 3.5);
	A_Rect* src_rect;

	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	LUTInfo			biP;
	AEFX_CLR_STRUCT(biP);
	A_long				linesL = 0;
	LUTInfo* g_tableP;


#ifdef AE_OS_WIN
	char* sysDrive = getenv("SystemDrive");
	string lutPathWin = "C:\\Users\\Natel\\Documents\\myLUT2.cube";

	const char* lutPathWinConst = lutPathWin.c_str();
	ifstream lutListFile(lutPathWinConst);
	//#ifdef defined(__GNUC__) && defined(__MACH__)
#else
	string lutPathMac = "macDocumentsFolder/myLUT.cube";

	const char* lutPathMacConst = lutPathMac.c_str();
	ifstream lutFile(lutPathMacConst);
#endif

	int counter = 0;
#ifdef AE_OS_WIN
	const char* filePath = lutPathWinConst;
#else
	const char* filePath = lutPathMacConst;
#endif

	char line[100000];
	FILE* fp;
	string tp;
	fp = fopen(filePath, "r");
	//FILE* fp;
	//fopen_s(&fp, filePath, "r");
	////Checks if file is empty
	while (fgets(line, 100000, fp)) {
		//printf("%s\n", line);
		tp = string(line);


		biP.LUTArray[counter] = std::stof(tp);
		counter++;

		//return 0;
	}


	linesL = output->extent_hint.bottom - output->extent_hint.top;

	PF_NewWorldFlags	flags = PF_NewWorldFlag_CLEAR_PIXELS;
	PF_Boolean			deepB = PF_WORLD_IS_DEEP(output);

	if (deepB) {
		flags |= PF_NewWorldFlag_DEEP_PIXELS;
	}

	ERR(AEFX_AcquireSuite(in_data,
		out_data,
		kPFWorldSuite,
		kPFWorldSuiteVersion2,
		"Couldn't load suite.",
		(void**)&wsP));

	if (!err) {

		ERR(wsP->PF_GetPixelFormat(input, &format));
		// pixel depth switch
		switch (format) {
		case PF_PixelFormat_ARGB128:
		// 32 bpc

			ERR(suites.IterateFloatSuite1()->iterate(in_data,
				0,								// progress base
				output->height,							// progress final
				input,	// src 
				NULL,							// area - null for all pixels
				(void*)&biP,					// refcon - your custom data pointer
				ApplyLUT32,				// pixel function pointer
				output));


			break;
		case PF_PixelFormat_ARGB64:
			// 16 bpc

			break;
		case PF_PixelFormat_ARGB32:

			// 8 bpc

			break;
		}
	}


	ERR2(AEFX_ReleaseSuite(in_data,
		out_data,
		kPFWorldSuite,
		kPFWorldSuiteVersion2,
		"Coludn't release suite."));


	return err;
}


static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	LUTInfo			giP;
	AEFX_CLR_STRUCT(giP);
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top;
	
	if (PF_WORLD_IS_DEEP(output)){
	
	} else {
			
	}

	return err;
}

static PF_Err
PreRender(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_PreRenderExtra* extra)
{
	PF_Err err = PF_Err_NONE;
	PF_ParamDef channel_param;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;

	//AEFX_CLR_STRUCT(channel_param);


	// Mix in the background color of the comp, as a demonstration of GuidMixInPtr()
	// When the background color changes, the effect will need to be rerendered.
	// Note: This doesn't handle the collapsed comp case
	// Your effect can use a similar approach to trigger a rerender based on changes beyond just its effect parameters.

	req.channel_mask = PF_ChannelMask_RED | PF_ChannelMask_GREEN | PF_ChannelMask_BLUE;

	req.preserve_rgb_of_zero_alpha = FALSE;	//	Hey, we dont care.N


		ERR(extra->cb->checkout_layer(in_data->effect_ref,
			SKELETON_INPUT,
			SKELETON_INPUT,
			&req,
			in_data->current_time,
			in_data->time_step,
			in_data->time_scale,
			&in_result));
	

	UnionLRect(&in_result.result_rect, &extra->output->result_rect);
	UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);

	//	Notice something missing, namely the PF_CHECKIN_PARAM to balance
	//	the old-fashioned PF_CHECKOUT_PARAM, above? 

	//	For SmartFX, AE automagically checks in any params checked out 
	//	during PF_Cmd_SMART_PRE_RENDER, new or old-fashioned.

	return err;
}

static PF_Err
SmartRender(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_SmartRenderExtra* extra)

{

	PF_Err			err = PF_Err_NONE,
		err2 = PF_Err_NONE;

	PF_EffectWorld* input_worldP = NULL;
	PF_EffectWorld* output_worldP = NULL;

	PF_ParamDef params[SKELETON_NUM_PARAMS];
	PF_ParamDef* paramsP[SKELETON_NUM_PARAMS];

	AEFX_CLR_STRUCT(params);

	for (int i = 0; i < SKELETON_NUM_PARAMS; i++) {
		paramsP[i] = &params[i];
	}

	ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, SKELETON_INPUT, &input_worldP)));
	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	ERR(ActuallyRender(in_data,
		out_data,
		output_worldP,
		input_worldP,
		paramsP));


	return err;

}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"2D LUT", // Name
		"NT 2DLUT", // Match Name
		"NT Productions", // Category
		AE_RESERVED_INFO); // Reserved Info

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;

			case PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
				break;

			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
				break;
			
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

