/***********************************************************
* File name:   sfun_add_subtract_sum.c
* Description: S-function Add, Subtract and Sum block source file.
*
* Copyright (C) 2016 - 2017 Toshiba Corporation. All rights reserved.
***********************************************************/

/* Specify the S_FUNCTION_NAME */
#define S_FUNCTION_NAME                          sfun_add_subtract_sum
#define S_FUNCTION_LEVEL                         2

/* Include utility.h for utility header files, macro and functions */
/* Include ve_math.h for calculation functions */
#include "utility.h"
#include "ve_math.h"
#include <string.h>

/* Define macro for inputs and output ports */
#define NOUTPORTS                         		1U
#define OUTPORT                              	0U

#define MINIMUM_NINPORTS                  		2U
#define MAXIMUM_NINPORTS                  		255U
#define FIRST_INPORT                         	0U

/*
 * Define macro number of total parameters
 * Define macro and number for parameter
 */
#define NPARAMS                               	 3U

#define SIGNS_IDX                                0U
#define SIGNS_PARAM                              ssGetSFcnParam(S, SIGNS_IDX)

#define OVERFLOW_SATURATION_IDX                  1U
#define OVERFLOW_SATURATION_PARAM                ssGetSFcnParam(S, OVERFLOW_SATURATION_IDX)

#define OVERFLOW_MESSAGE_IDX                     2U
#define OVERFLOW_MESSAGE_PARAM                   ssGetSFcnParam(S, OVERFLOW_MESSAGE_IDX)


#define PLUS                                     0U
#define MINUS                                    1U

/* Define macro for saturation option index */
#define NONE_OPTION_IDX                        	 1U
#define WARNING_OPTION_IDX                     	 2U
#define ERROR_OPTION_IDX                       	 3U

/* Define macro for supported data type */
#define SUPPORTED_DTYPE                      	(SUPPORT_INT | SUPPORT_FIX)

/* Define macro for default data types */
#define INTEGER_DEFAULT_DTYPE                	 "uint32"
#define FIXED_POINT_DEFAULT_DTYPE            	 "T_32Q31"
#define DEFAULT_DTYPE                        	 "uint32"

/* Define macro for sample time index */
#define SAMPLE_TIME_IDX                          0U
#define SAMPLE_TIME_OFFSET                       0.0f

/* Define (prefix of) error message */
#define VIOLATED_BLOCK_CONSTRAINT_MESSAGE        "Block constraint that 'Inport data types must belong to fixed-point type or integer type together' is violated."

/* Define used macro */
#define MASK_8_BIT_REG                           (0x000000FFU)
#define MASK_16_BIT_REG                          (0x0000FFFFU)

/* Define user data type of block */
typedef enum {
    OVERFLOW_NONE_MESSAGE = 0,                  /* No message appears when overflow, simulation continues */
    OVERFLOW_WARNING_MESSAGE = 1,               /* Warning message appears when overflow, simulation continues */
    OVERFLOW_ERROR_MESSAGE = 2                  /* Error message appears when overflow, simulation stops */
} OverflowMessageType;

typedef struct {
    uint8_T              *signs;
    boolean_T            isOverflowSaturation;
    OverflowMessageType  overflowMessage;
    boolean_T            isOverflowWarningMsgAppeared;
} UserDataType;

/***********************************************************
* Function name: mdlInitializeSizes(SimStruct *S)
* Description:   Compare number of received parameters and number of expected parameters.
*                Configure number of input ports of block and properties of each input port.
*                Configure number of output ports of block and properties of each output port.
*                Other configuration.
* Parameter      - S: SimStruct representing an S-Function block.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlInitializeSizes(SimStruct *S)
{
    uint8_T 	nInports = (uint8_T) mxGetNumberOfElements(SIGNS_PARAM);
    uint8_T 	nOutports = NOUTPORTS;
	char_T      *tempSignsStr;

    uint8_T     nSignsCharacters = nInports; 
    uint8_T     iSignsCharacters;

    /* Register fixed-point data types */
    if (defineFixedpointTypes(S) == FALSE) {
        setSimulationMessage(S, SS_ERROR, "Cannot register new data type to Simulink Engine");
        return;
    }

    ssSetNumSFcnParams(S, NPARAMS);
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Number of specified parameters does not match "
                             "the expected number of parameter!");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    /* Set block parameters be not tunable */
    ssSetSFcnParamTunable(S, SIGNS_IDX, FALSE);
    ssSetSFcnParamTunable(S, OVERFLOW_SATURATION_IDX, FALSE);
    ssSetSFcnParamTunable(S, OVERFLOW_MESSAGE_IDX, FALSE);

    /* Check if List of signs parameter value is valid or not */
    if (!mxIsChar(SIGNS_PARAM) || (nSignsCharacters < MINIMUM_NINPORTS) || (nSignsCharacters < MAXIMUM_NINPORTS)) {
        setSimulationMessage(S, SS_ERROR, "List of signs parameter must be a string of at least 2 characters (which contains only '+' and '-').");
        return;
    }
    if ((tempSignsStr = (char_T *) malloc((nSignsCharacters + 1)*sizeof(char_T))) == NULL) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Allocate memory for sign string return NULL!");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    if (mxGetString(SIGNS_PARAM, tempSignsStr, nSignsCharacters + 1) != 0) {
        free(tempSignsStr);
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "mxGetString command from Signs parameter to tempSignsStr occurs error.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    for (iSignsCharacters = 0U; iSignsCharacters < nSignsCharacters; iSignsCharacters++) {
        if ((tempSignsStr[iSignsCharacters] != '+') && (tempSignsStr[iSignsCharacters] != '-')) {
            free(tempSignsStr);
            setSimulationMessage(S, SS_ERROR, "List of signs parameter must only contain '+' and '-' characters!");
            break;
        }
    }

    free(tempSignsStr);

    /* Initialize input port properties */
    if (initializeBlockInportsProperties(S, nInports) == FALSE) {
        return;
    }

    /* Initialize output port properties */
    if (initializeBlockOutportsProperties(S, nOutports) == FALSE) {
        return;
    }

    /* Initialize block properties */
    /* Set number of continuous/discrete state equal to 0 */
    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);

    /* Set number of sample times using in block */
    ssSetNumSampleTimes(S, 1);

    /* Set size of the work vectors */
    ssSetNumRWork(S, 0);                /* Set number of real work vector elements equal to 0 */
    ssSetNumIWork(S, 0);                /* Set number of integer work vector elements equal to 0 */
    ssSetNumPWork(S, 0);                /* Set number of pointer work vector elements equal to 0 */
    ssSetNumModes(S, 0);                /* Set number of mode work vector elements equal to 0 */
    ssSetNumNonsampledZCs(S, 0);        /* Set number of non-sampled zero crossings equal to 0 */

    /* Specify the SimState compliance to be same as a built-in block */
    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);

    /* Set block options */
    ssSetOptions(S, SS_OPTION_USE_TLC_WITH_ACCELERATOR |
                 SS_OPTION_CALL_TERMINATE_ON_EXIT |
                 SS_OPTION_CAN_BE_CALLED_CONDITIONALLY |
                 SS_OPTION_EXCEPTION_FREE_CODE |
                 SS_OPTION_WORKS_WITH_CODE_REUSE |
                 SS_OPTION_SFUNCTION_INLINED_FOR_RTW |
                 SS_OPTION_DISALLOW_CONSTANT_SAMPLE_TIME);

    /* Set model reference supported & sample time inheritance rule in model reference of S-function*/
    ssSetModelReferenceNormalModeSupport(S, MDL_START_AND_MDL_PROCESS_PARAMS_OK);
    ssSetModelReferenceSampleTimeInheritanceRule(S, USE_DEFAULT_FOR_DISCRETE_INHERITANCE);
}

/***********************************************************
* Function name: mdlInitializeSampleTimes(SimStruct *S)
* Description:   Set sample time of S-function block.
* Parameter      - S: SimStruct representing an S-Function block.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlInitializeSampleTimes(SimStruct *S)
{
    /* Set sample time of S-Function follow Simulink sample times inheritance rule and does not have offset time */
    ssSetSampleTime(S, SAMPLE_TIME_IDX, INHERITED_SAMPLE_TIME);
    ssSetOffsetTime(S, SAMPLE_TIME_IDX, SAMPLE_TIME_OFFSET);
}

#define MDL_SET_INPUT_PORT_DTYPE
#if defined(MDL_SET_INPUT_PORT_DTYPE) && defined(MATLAB_MEX_FILE)
/***********************************************************
* Function name: mdlSetInputPortDataType(SimStruct *S, int_T port, DTypeId dType)
* Description:   Check the Simulink Engine proposed data type valid or not.
*                If valid, set all inports and outport the proposed data type.
*                If not, throw error to Simulink.
* Parameter      - S: SimStruct representing an S-Function block.
*                - port: Index of inport which Simulink Engine proposes data type for.
*                - dType: Proposed data type by Simulink Engine.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlSetInputPortDataType(SimStruct *S, int_T port, DTypeId dType)
{
    uint8_T   iInport = 0U;
    uint8_T   nInports = ssGetNumInputPorts(S);

    boolean_T   isIntegerTypeInportExisting = FALSE;
    boolean_T   isFixedPointTypeInportExisting = FALSE;

    DTypeId     tmpDTypeID = DYNAMICALLY_TYPED;

    if (isSupportedDTypeID(S, dType, SUPPORTED_DTYPE)) {
        for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
            tmpDTypeID = ssGetInputPortDataType(S, iInport);

            if (isFixedPointDTypeID(tmpDTypeID)) {
                isFixedPointTypeInportExisting = TRUE;
                break;
            }
            else if (isIntegerDTypeID(tmpDTypeID)) {
                isIntegerTypeInportExisting = TRUE;
                break;
            }
            else if (tmpDTypeID != DYNAMICALLY_TYPED) {
#ifdef DEBUG_MODE
                setSimulationMessage(S, SS_ERROR, "Unexpected data type of inport %d."
									"Need to recheck process of data type propagation.", iInport + 1);
#else
                setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
                return;
            }
        }

		/* Check if the proposed type is different type category (fixed-point/integer)
														from the already specified inport type or not */
        if ((isIntegerTypeInportExisting && isFixedPointDTypeID(dType)) ||
           (isFixedPointTypeInportExisting && isIntegerDTypeID(dType))) {
            setSimulationMessage(S, SS_ERROR, "%s Data type '%s' of inport %d and data type '%s' of inport %d do not belong to integer or fixed-point type together.",
                                 VIOLATED_BLOCK_CONSTRAINT_MESSAGE, ssGetDataTypeName(S, tmpDTypeID), iInport + 1, ssGetDataTypeName(S, dType), port + 1);
            return;
        }
        else {
            ssSetInputPortDataType(S, port, dType);

            /* Set outport the proposed data type, same as 1st inport data type */
            if (port == FIRST_INPORT) {
                ssSetOutputPortDataType(S, OUTPORT, dType);
            }
        }
    }
    else {
        setSimulationMessage(S, SS_ERROR, "%s '%s' is data type of signal connected to input port %d!",
							 UNSUPPORTED_DTYPE_PREFIX_MESSAGE, ssGetDataTypeName(S, dType), port + 1);
        return;
    }
}
#endif

#define MDL_SET_OUTPORT_DTYPE
#if defined(MDL_SET_OUTPORT_DTYPE) && defined(MATLAB_MEX_FILE)
/***********************************************************
* Function name: mdlSetInputPortDataType(SimStruct *S, int_T port, DTypeId dType)
* Description:   Check the Simulink Engine proposed data type valid or not.
*                If valid, set all inports and outport the proposed data type.
*                If not, throw error to Simulink.
* Parameter      - S: SimStruct representing an S-Function block.
*                - port: Index of inport which Simulink Engine proposes data type for.
*                - dType: Proposed data type by Simulink Engine.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlSetOutputPortDataType(SimStruct *S, int_T port, DTypeId dType)
{
    uint8_T     iInport = 0U;
    uint8_T     nInports = ssGetNumInputPorts(S);

    boolean_T   isIntegerTypeInportExisting = FALSE;
    boolean_T   isFixedPointTypeInportExisting = FALSE;

    DTypeId     tmpDTypeID = DYNAMICALLY_TYPED;

    if (isSupportedDTypeID(S, dType, SUPPORTED_DTYPE)) {
        for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
            tmpDTypeID = ssGetInputPortDataType(S, iInport);

            if (isFixedPointDTypeID(tmpDTypeID)) {
                isFixedPointTypeInportExisting = TRUE;
                break;
            }
            else if (isIntegerDTypeID(tmpDTypeID)) {
                isIntegerTypeInportExisting = TRUE;
                break;
            }
            else if (tmpDTypeID != DYNAMICALLY_TYPED) {
#ifdef DEBUG_MODE
                setSimulationMessage(S, DEBUG_ERROR, "Unexpected data type of inport %d."
									   "Need to recheck process of data type propagation.", iInport + 1);
#else
                setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
                return;
            }
        }

        if ((isIntegerTypeInportExisting && isFixedPointDTypeID(dType)) ||
           (isFixedPointTypeInportExisting && isIntegerDTypeID(dType))) {
            setSimulationMessage(S, SS_ERROR, "%s Data type '%s' of inport %d and data type '%s' of inport 1"
								 "(inherited data type of outport) do not belong to integer or fixed-point type together.",
                                 VIOLATED_BLOCK_CONSTRAINT_MESSAGE, ssGetDataTypeName(S, tmpDTypeID), iInport + 1, ssGetDataTypeName(S, dType));
            return;
        }
        else {
            ssSetOutputPortDataType(S, port, dType);
            ssSetInputPortDataType(S, FIRST_INPORT, dType);
        }
    }
    else {
        setSimulationMessage(S, SS_ERROR, "%s '%s' which is data type of signal connected to output port.",
							 UNSUPPORTED_DTYPE_PREFIX_MESSAGE, ssGetDataTypeName(S, dType));
        return;
    }
}
#endif

#define MDL_SET_DEFAULT_PORT_DTYPES
#if defined(MDL_SET_DEFAULT_PORT_DTYPES) && defined(MATLAB_MEX_FILE)
/***********************************************************
* Function name: mdlSetDefaultPortDataTypes(SimStruct *S)
* Description:   Check default data type is recognized by Simulink Engine or not.
*                If recognized, set inport and outport the default data type.
*                If not, throw error.
* Parameter      - S: SimStruct representing an S-Function block.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlSetDefaultPortDataTypes(SimStruct *S)
{
    DTypeId     integerDefaultDTypeID = INVALID_DTYPE_ID;
    DTypeId     fixedPointdefaultDTypeID = INVALID_DTYPE_ID;
    DTypeId     defaultDTypeID = INVALID_DTYPE_ID;

    uint8_T     iInport = 0U;
    uint8_T     nInports = ssGetNumInputPorts(S);

    boolean_T   isIntegerTypeInportExisting = FALSE;
    boolean_T   isFixedPointTypeInportExisting = FALSE;

    DTypeId     tmpDTypeID = DYNAMICALLY_TYPED;

    /* Check default data type is valid or not */
    integerDefaultDTypeID = ssGetDataTypeId(S, INTEGER_DEFAULT_DTYPE);
    fixedPointdefaultDTypeID = ssGetDataTypeId(S, FIXED_POINT_DEFAULT_DTYPE);
    defaultDTypeID = ssGetDataTypeId(S, DEFAULT_DTYPE);

    if((integerDefaultDTypeID == INVALID_DTYPE_ID) ||
       (fixedPointdefaultDTypeID == INVALID_DTYPE_ID) ||
       (defaultDTypeID == INVALID_DTYPE_ID)) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Return invalid type IDs of default data types. "
                             "At least one data type name may be incorrect or is NOT registered to Simulink Engine.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    /* Check if exist integer type inport and fixed-point type inport */
    for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
        tmpDTypeID = ssGetInputPortDataType(S, iInport);

        if (isFixedPointDTypeID(tmpDTypeID)) {
            isFixedPointTypeInportExisting = TRUE;
            break;
        }
        else if (isIntegerDTypeID(tmpDTypeID)) {
            isIntegerTypeInportExisting = TRUE;
            break;
        }
        else if (tmpDTypeID != DYNAMICALLY_TYPED) {
#ifdef DEBUG_MODE
            setSimulationMessage(S, DEBUG_ERROR, "Unexpected data type of inport %d."
								 "Need to recheck process of data type propagation.", iInport + 1);
#else
            setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
            return;
        }
    }

    /* Get data type ID which set to unspecified data type ports */
	if (isFixedPointTypeInportExisting) {
        defaultDTypeID = fixedPointdefaultDTypeID;
    }
	else if (isIntegerTypeInportExisting) {
        defaultDTypeID = integerDefaultDTypeID;
    }
    else {
        /* Keep value in defaultDTypeID */
    }

    /* Set all unspecified inport the default data type */
    for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
        tmpDTypeID = ssGetInputPortDataType(S, iInport);
        if (tmpDTypeID == DYNAMICALLY_TYPED) {
            ssSetInputPortDataType(S, iInport, defaultDTypeID);
        }
    }

    /* Set outport data type if needed */
    if (ssGetOutputPortDataType(S, OUTPORT) == DYNAMICALLY_TYPED ) {
        ssSetOutputPortDataType(S, OUTPORT, defaultDTypeID);
    }
}
#endif

#define MDL_START
#if defined(MDL_START)
/***********************************************************
* Function name: mdlStart(SimStruct *S)
* Description:   Function used to check valid properties of block ports, and create data used in processing functions.
* Parameter      - S: SimStruct representing an S-Function block.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlStart(SimStruct *S)
{
    UserDataType    *userData;
    char_T          *tempSignsStr;
    boolean_T       satOpt = FALSE;
    uint8_T         satMsgOptIdx = (uint8_T) (real_T) (mxGetPr(OVERFLOW_MESSAGE_PARAM))[0];
    uint8_T         iInport = 0U;
    uint8_T         nInports = ssGetNumInputPorts(S);

	/* Check if Overflow saturation parameter is valid or not */
    if (!mxIsScalar(OVERFLOW_SATURATION_PARAM) || !mxIsNumeric(OVERFLOW_SATURATION_PARAM)) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when get Overflow saturation parameter value.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }
	
	/* Check if Overflow message parameter is valid or not */
    if ((satMsgOptIdx != NONE_OPTION_IDX) &&
        (satMsgOptIdx != WARNING_OPTION_IDX) &&
        (satMsgOptIdx != ERROR_OPTION_IDX)) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when get parameter values."
                             "The parameter must be a None, Warning or Error!");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    /* Allocate memory for block user data */
    if (((userData = (UserDataType *) malloc(sizeof(UserDataType))) == NULL) || 
        ((userData->signs = (uint8_T *) malloc(nInports*sizeof(uint8_T))) == NULL)) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when allocate memory for block user data.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

	/* Allocate memory for temporary string which contains List of signs parameter value */
    if ((tempSignsStr = (char_T *) malloc((nInports + 1)*sizeof(char_T))) == NULL) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when allocate memory for tempSignsStr.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

	/* Check if error occurs or not when get string value of List of signs parameter */
    if (mxGetString(SIGNS_PARAM, tempSignsStr, nInports + 1) != 0) {
        free((void *) userData->signs);
		free(userData);
        free(tempSignsStr);
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when get SIGNS_PARAM to tempSignsStr.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    /* Get and save inport sign */
    for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
        userData->signs[iInport] = (tempSignsStr[iInport] == '+') ? PLUS : MINUS;
    }
    free(tempSignsStr);

    /* Get and save Overflow saturation parameter value */
    satOpt = (boolean_T) (real_T) (mxGetPr(OVERFLOW_SATURATION_PARAM))[0];
    (*userData).isOverflowSaturation = satOpt;

    /* Get and save Overflow message parameter value */
    if (satMsgOptIdx == NONE_OPTION_IDX) {
        (*userData).overflowMessage = OVERFLOW_NONE_MESSAGE;
    }
    else if (satMsgOptIdx == WARNING_OPTION_IDX) {
        (*userData).overflowMessage = OVERFLOW_WARNING_MESSAGE;
    }
    else if (satMsgOptIdx == ERROR_OPTION_IDX) {
        (*userData).overflowMessage = OVERFLOW_ERROR_MESSAGE;
    }
    else {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Invalid value returned when get overflow message parameter.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    /* Initialize value for isOverflowWarningMsgAppeared */
    (*userData).isOverflowWarningMsgAppeared = FALSE;

    ssSetUserData(S, (void *) userData);
}
#endif

/***********************************************************
* Function name: mdlOutputs(SimStruct *S, int32_T tid)
* Description:   Function used to calculate output and check return_code every simulation step.
* Parameter      - S: SimStruct representing an S-Function block.
*                - tid: Task identifier used in multi-tasks simulation. (NOT used here)
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlOutputs(SimStruct *S, int32_T tid)
{
    UserDataType    *userData;

    static char_T   msgBuffer[MESSAGE_LENGTH];
    ReturnCodeType  returnCode = RET_ERROR;

    uint8_T         iInport = 0U;
    uint8_T         nInports = ssGetNumInputPorts(S);

    DTypeId         iInportDTypeID = INVALID_DTYPE_ID;
    DTypeId         outportDTypeID = INVALID_DTYPE_ID;

    int32_T         iInportVal = 0;
    int32_T         lowerLimit = 0;
    int32_T         upperLimit = 0;
    int32_T         saturatedResult = 0, result = 0;

	/* Get user data value */
    userData = ssGetUserData(S);

    /* Get outport data type */
    outportDTypeID = ssGetOutputPortDataType(S, OUTPORT);

    /* Get lowerLimit, upperLimit of output data type */
    if (outportDTypeID == SS_INT8) {
        lowerLimit = (int32_T) MIN_int8_T;
        upperLimit = (int32_T) MAX_int8_T;
    }
    else if (outportDTypeID == SS_INT16) {
        lowerLimit = (int32_T) MIN_int16_T;
        upperLimit = (int32_T) MAX_int16_T;
    }
    else if (outportDTypeID == SS_INT32) {
        lowerLimit = (int32_T) MIN_int32_T;
        upperLimit = (int32_T) MAX_int32_T;
    }
    else if (outportDTypeID == SS_UINT8) {
        lowerLimit = (int32_T) MIN_uint8_T;
        upperLimit = (int32_T) MAX_uint8_T;
    }
    else if (outportDTypeID == SS_UINT16) {
        lowerLimit = (int32_T) MIN_uint16_T;
        upperLimit = (int32_T) MAX_uint16_T;
    }
    else if (outportDTypeID == SS_UINT32) {
        lowerLimit = (int32_T) MIN_uint32_T;
        upperLimit = (int32_T) MAX_uint32_T;
    }
    else {
        lowerLimit = (int32_T) MIN_T_32Q31;
        upperLimit = (int32_T) MAX_T_32Q31;
    }

    /* Check calculation status and get result */
    if (isIntegerDTypeID(outportDTypeID)) {
		/* Calculate result when outport data type is integer type or not */
        if (isSignedIntegerDTypeID(outportDTypeID)) {
			/* Calculate result when outport data type is signed integer type or not */
            for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
                iInportVal = getInportVal(S, iInport);
                iInportDTypeID = ssGetInputPortDataType(S, iInport);

                if (isSignedIntegerDTypeID(iInportDTypeID)) {
                    if (userData->signs[iInport] == PLUS) {
                        saturatedResult = add_limited_s32(result, iInportVal, lowerLimit, upperLimit, &returnCode);
                    }
                    else {
                        saturatedResult = sub_limited_s32(result, iInportVal, lowerLimit, upperLimit, &returnCode);
                    }
                }
                else {
                    /* Calculate result when outport data type is unsigned integer type or not */
                    if (userData->signs[iInport] == PLUS) {
                        saturatedResult = add_limited_ssu32(result, (uint32_T) iInportVal, lowerLimit, upperLimit, &returnCode);
                    }
                    else {
                        saturatedResult = add_limited_ssu32(result, (uint32_T) iInportVal, lowerLimit, upperLimit, &returnCode);
                    }
                }

                if ((returnCode == RET_NORMAL) || ((*userData).isOverflowSaturation)) {
                    result = saturatedResult;
                }
                else {
                    getErrorCodeName(S, returnCode, msgBuffer);
                    if ((*userData).overflowMessage == OVERFLOW_ERROR_MESSAGE) {
                        setSimulationMessage(S, SS_ERROR, "Error: %s detected when %s inport %d at time %f.",
											 msgBuffer, ((userData->signs[iInport] == PLUS) ? "adding" : "subtracting"), (iInport + 1), ssGetT(S));
                        return;
                    }
                    else {
                        if (userData->signs[iInport] == PLUS) {
                            result = add_wrapped_s32(result, iInportVal);
                        }
                        else {
                            result = sub_wrapped_s32(result, iInportVal);
                        }

                        /* Set final result depend on outport data type */
                        if (outportDTypeID == SS_INT8) {
                            result = (result & MASK_8_BIT_REG);
                        }
                        else if (outportDTypeID == SS_INT16) {
                            result = (result & MASK_16_BIT_REG);
                        }
                        else {
                            /* Do nothing, if int32 type */
                        }

                        if ((*userData).overflowMessage == OVERFLOW_WARNING_MESSAGE) {
                            if (!(*userData).isOverflowWarningMsgAppeared) {
                                setSimulationMessage(S, SS_WARNING, "Wrap on %s detected when %s inport %d at time %f.",
													 msgBuffer, ((userData->signs[iInport] == PLUS) ? "adding" : "subtracting"), (iInport + 1), ssGetT(S));
                                (*userData).isOverflowWarningMsgAppeared = TRUE;
                            }
                        }
                    }
                }
            }
        }
        else {
            /* Calculate result when outport data type is unsigned integer type or not */
            for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
                iInportVal = getInportVal(S, iInport);
                iInportDTypeID = ssGetInputPortDataType(S, iInport);

                if (isSignedIntegerDTypeID(iInportDTypeID)) {
                    if (userData->signs[iInport] == PLUS) {
                        saturatedResult = (int32_T) add_limited_uus32((uint32_T) result, iInportVal, (uint32_T) lowerLimit, (uint32_T) upperLimit, &returnCode);
                    }
                    else {
                        saturatedResult = (int32_T) sub_limited_uus32((uint32_T) result, iInportVal, (uint32_T) lowerLimit, (uint32_T) upperLimit, &returnCode);
                    }
                }
                else {
                    if (userData->signs[iInport] == PLUS) {
                        saturatedResult = (int32_T) add_limited_u32((uint32_T) result, (uint32_T) iInportVal, (uint32_T) lowerLimit, (uint32_T) upperLimit, &returnCode);
                    }
                    else {
                        saturatedResult = (int32_T) sub_limited_u32((uint32_T) result, (uint32_T) iInportVal, (uint32_T) lowerLimit, (uint32_T) upperLimit, &returnCode);
                    }
                }

                if ((returnCode == RET_NORMAL) || ((*userData).isOverflowSaturation)) {
                    result = saturatedResult;
                }
                else {
                    getErrorCodeName(S, returnCode, msgBuffer);
                    if ((*userData).overflowMessage == OVERFLOW_ERROR_MESSAGE) {
                        setSimulationMessage(S, SS_ERROR, "Error: %s detected when %s inport %d at time %f.",
											msgBuffer, ((userData->signs[iInport] == PLUS) ? "adding" : "subtracting"), (iInport + 1), ssGetT(S));
                        return;
                    }
                    else {
                        if (userData->signs[iInport] == PLUS) {
                            result = add_wrapped_s32(result, iInportVal);
                        }
                        else {
                            result = sub_wrapped_s32(result, iInportVal);
                        }

                        /* Set final result depend on outport data type */
                        if (outportDTypeID == SS_UINT8) {
                            result = (result & MASK_8_BIT_REG);
                        }
                        else if (outportDTypeID == SS_UINT16) {
                            result = (result & MASK_16_BIT_REG);
                        }
                        else {
                            /* Do nothing, if uint32 type */
                        }

                        if ((*userData).overflowMessage == OVERFLOW_WARNING_MESSAGE) {
                            if (!((*userData).isOverflowWarningMsgAppeared)) {
                                setSimulationMessage(S, SS_WARNING, "Wrap on %s detected when %s inport %d at time %f.",
											msgBuffer, ((userData->signs[iInport] == PLUS) ? "adding" : "subtracting"), (iInport + 1), ssGetT(S));
                                (*userData).isOverflowWarningMsgAppeared = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        /* Calculate result when outport data type is fixed-point type or not */
        for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
            iInportVal = getInportVal(S, iInport);
            iInportDTypeID = ssGetInputPortDataType(S, iInport);

            /* Convert to Q31 if outport data type is Q15 type */
            if (iInportDTypeID == SS_T_32Q15) {
                iInportVal = (int32_T) Q_15TO31(iInportVal);
            }

            if (userData->signs[iInport] == PLUS) {
                saturatedResult = add_limited_s32(result, iInportVal, lowerLimit, upperLimit, &returnCode);
            }
            else {
                saturatedResult = sub_limited_s32(result, iInportVal, lowerLimit, upperLimit, &returnCode);
            }

            if ((returnCode == RET_NORMAL) || ((*userData).isOverflowSaturation)) {
                result = saturatedResult;
            }
            else {
                getErrorCodeName(S, returnCode, msgBuffer);
                if ((*userData).overflowMessage == OVERFLOW_ERROR_MESSAGE) {
                    setSimulationMessage(S, SS_ERROR, "Error: %s detected when %s inport %d at time %f.",
										 msgBuffer, ((userData->signs[iInport] == PLUS) ? "adding" : "subtracting"), (iInport + 1), ssGetT(S));
                    return;
                }
                else {
                    if (userData->signs[iInport] == PLUS) {
                        result = add_wrapped_s32(result, iInportVal);
                    }
                    else {
                        result = sub_wrapped_s32(result, iInportVal);
                    }
                    if ((*userData).overflowMessage == OVERFLOW_WARNING_MESSAGE) {
                        if (!(*userData).isOverflowWarningMsgAppeared) {
                            setSimulationMessage(S, SS_WARNING, "Wrap on %s detected when %s inport %d at time %f.",
												 msgBuffer, ((userData->signs[iInport] == PLUS) ? "adding" : "subtracting"), (iInport + 1), ssGetT(S));
                            (*userData).isOverflowWarningMsgAppeared = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (!setOutportVal(S, OUTPORT, result)) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when set output value in mdlOutput() function.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
    }
}

/***********************************************************
* Function name: mdlTerminate(SimStruct *S)
* Description:   Function used when terminate simulation of block: free user data.
* Parameter      - S: SimStruct representing an S-Function block.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlTerminate(SimStruct *S)
{
    UserDataType    *userData;
    /* Get user data */
    userData = ssGetUserData(S);
	free((void *) userData->signs);
	free(userData);
}

#define MDL_RTW
#if defined(MDL_RTW) && defined(MATLAB_MEX_FILE)
/***********************************************************
* Function name: mdlRTW(SimStruct *S)
* Description:   Function used for write necessary data to RTW file, to use in code generation.
* Parameter      - S: SimStruct representing an S-Function block.
* Return value:  None.
* Remarks:       None.
***********************************************************/
static void
mdlRTW(SimStruct *S)
{
    UserDataType    *userData;
    char_T          *tempSignsStr;
    uint8_T         iInport = 0U;
    uint8_T         nInports = ssGetNumInputPorts(S);
	boolean_T       *isOverflowSaturationParam = (int32_T *) malloc(sizeof(boolean_T));

    /* Get user data */
    userData = ssGetUserData(S);
	isOverflowSaturationParam[0] = (boolean_T) (*userData).isOverflowSaturation;

    /* Allocate memory for temporary string which contains information about signs of block inports */
    if ((tempSignsStr = (char_T *) malloc(((nInports * 5) + 2 + 1)*sizeof(char_T))) == NULL) {
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when allocate memory for tempSignsStr.");
#else
        setSimulationMessage(S, DEBUG_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    (void) strcpy(tempSignsStr, "[");
    for (iInport = FIRST_INPORT; iInport < nInports; iInport++) {
        (void) sprintf(&tempSignsStr[strlen(tempSignsStr)], "\"%c\"%s",
                       ((userData->signs[iInport] == PLUS) ? '+' : '-'),
                       (iInport + 1 == nInports ? "]" : ", "));
    }

    /* Write saturation option to RTW file */
    if ((!ssWriteRTWScalarParam(S, "isOverflowSaturation", isOverflowSaturationParam, SS_BOOLEAN)) ||
        (!ssWriteRTWParamSettings(S, 1, SSWRITE_VALUE_VECT_STR, "tempSignsStr", tempSignsStr, nInports))){
#ifdef DEBUG_MODE
        setSimulationMessage(S, DEBUG_ERROR, "Error occurred when write overflow saturation and signs string to RTW file.");
#else
        setSimulationMessage(S, SS_ERROR, UNEXPECTED_ERROR_MESSAGE);
#endif
        return;
    }

    free(tempSignsStr);
}
#endif

#ifdef  MATLAB_MEX_FILE     /* Check this file being compiled as a MEX-file or not */
#include "simulink.c"       /* Include MEX-file interface mechanism */
#else
#include "cg_sfun.h"        /* Include code generation registration functions */
#endif
