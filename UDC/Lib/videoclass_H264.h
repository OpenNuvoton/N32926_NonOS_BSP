/* Payload Header */
#define UVC_PH_EndOfFrame		0x02
#define UVC_PH_Preview			0x00
#define UVC_PH_Snapshot			0x20

/* Video Class-Specific Request Code */
#define SET_CUR			0x01
#define GET_CUR			0x81
#define GET_MIN			0x82
#define GET_MAX			0x83
#define GET_RES			0x84
#define GET_LEN			0x85
#define GET_INFO		0x86
#define GET_DEF			0x87

/* Control Selector Codes */
/* VideoControl Interface Control Selectors */
#define VC_CONTROL_UNDEFINED				0x0000
#define VC_VIDEO_POWER_MODE_CONTROL			0x0100
#define VC_REQUEST_ERROR_CODE_CONTROL		0x0200
#define Reserved							0x0300

/* Terminal Control Selectors */
#define TE_CONTROL_UNDEFINED				0x00

/* Selector Unit Control Selectors */
#define	SU_CONTROL_UNDEFINED				0x00
#define SU_INPUT_SELECT_CONTROL				0x01

/* Camera Terminal Control Selectors */
#define CT_CONTROL_UNDEFINED				0x00
#define CT_SCANNING_MODE_CONTROL			0x01
#define CT_AE_MODE_CONTROL					0x02
#define CT_AE_PRIORITY_CONTROL				0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL	0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL	0x05
#define CT_FOCUS_ABSOLUTE_CONTROL			0x06
#define CT_FOCUS_RELATIVE_CONTROL			0x07
#define CT_FOCUS_AUTP_CONTROL				0x08
#define CT_IRIS_ABSOLUTE_CONTROL			0x09
#define CT_IRIS_RELATIVE_CONTROL			0x0A
#define CT_ZOOM_ABSOLUTE_CONTROL			0x0B
#define CT_ZOOM_RELATIVE_CONTROL			0x0C
#define CT_PANTILT_ABSOLUTE_CONTROL			0x0D
#define CT_PANTILT_RELATIVE_CONTROL			0x0E
#define CT_ROLL_ABSOLUTE_CONTROL			0x0F
#define CT_ROLL_RELATIVE_CONTROL			0x10
#define CT_PRIVACY_CONTROL					0x11

/* Processing Unit Control Selectors */
#define	PU_CONTROL_UNDEFINED						0x00
#define	PU_BACKLIGHT_COMPENSATION_CONTROL			0x0100
#define	PU_BRIGHTNESS_CONTROL						0x0200
#define	PU_CONTRAST_CONTROL							0x0300
#define	PU_GAIN_CONTROL								0x0400	
#define	PU_POWER_LINE_FREQUENCY_CONTROL				0x0500
#define	PU_HUE_CONTROL								0x0600
#define	PU_SATURATION_CONTROL						0x0700
#define	PU_SHARPNESS_CONTROL						0x0800
#define	PU_GAMMA_CONTROL							0x0900
#define	PU_WHITE_BALANCE_TEMPERATURE_CONTROL		0x0A00
#define	PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL	0x0B00
#define	PU_WHITE_BALANCE_COMPONENT_CONTROL			0x0C00
#define	PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL		0x0D00
#define	PU_DIGITAL_MULTIPLIER_CONTROL				0x0E00
#define	PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL			0x0F00
#define	PU_HUE_AUTO_CONTROL							0x1000
#define	PU_ANALOG_VIDEO_STANDARD_CONTROL			0x1100
#define	PU_ANALOG_LOCK_STATUS_CONTROL				0x1200

// define audio for interface 2
#define PU_MUTE_CONTROL								0x0102
#define PU_VOLUME_CONTROL							0x0202
/* Extension Unit Control Selectors */
#define XU_CONTROL_UNDEFINED						0x00

/* VideoStreaming Interface Control Selectors */
#define	VS_CONTROL_UNDEFINED						0x0000
#define	VS_PROBE_CONTROL							0x0100
#define	VS_COMMIT_CONTROL							0x0200
#define	VS_STILL_PROBE_CONTROL						0x0300
#define	VS_STILL_COMMIT_CONTROL						0x0400
#define	VS_STILL_IMAGE_TRIGGER_CONTROL				0x0500
#define	VS_STREAM_ERROR_CODE_CONTROL				0x0600
#define	VS_GENERATE_KEY_FRAME_CONTROL				0x0700
#define	VS_UPDATE_FRAME_SEGMENT_CONTROL				0x0800
#define	VS_SYNCH_DELAY_CONTROL						0x0900

/* Request Error Code */
#define EC_No_Error			0x00
#define EC_Not_Ready		0x01
#define EC_Wrong_State		0x02
#define EC_Power			0x03
#define EC_Out_Of_Range		0x04
#define EC_Invalid_Uint		0x05
#define EC_Invalid_Control	0x06
#define EC_Invalid_Request	0x07

/* auduo definition */
// Control selector for USB microphone
#define MUTE_CONTROL             0x0100
#define VOLUME_CONTROL           0x0200
#define REC_FEATURE_UNITID		 0x07
#define	SAMPLING_FREQ_CONTROL			0x0100

#define UAC_STOP_AUDIO_RECORD		0
#define UAC_START_AUDIO_RECORD		1
#define UAC_PROCESSING_AUDIO_RECORD	2
#define UAC_BUSY_AUDIO_RECORD		3
__packed  typedef struct _CaptureFilter{
	INT16 volatile Brightness;
	INT16 volatile Contrast;
	INT16 volatile Hue;
	UINT8 volatile POWER_LINE_FREQUENCY;
	INT16 volatile Saturation;
	INT16 volatile Sharpness;
	INT16 volatile Gamma;
	INT16 volatile Backlight;
}CaptureFilter;
// audio attribute
__packed  typedef struct _FeatureFilter{
	INT32 volatile Mute;
	INT32 volatile Volume;
}FeatureFilter;


/* Device Descriptor */
__packed typedef struct _DEVICEDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT16  bcdUSB;
    UINT8   bDeviceClass;
    UINT8   bDeviceSubClass;
    UINT8   bDeviceProtocol;
    UINT8   bMaxPacketSize0;
    UINT16  idVendor;
    UINT16  idProduct;
    UINT16  bcdDevice;
    UINT8   iManufacturer;
    UINT8   iProduct;
    UINT8   iSerialNumber;
    UINT8   bNumConfigurations;
} DEVICEDESCRIPTOR;

/* Configuration Descriptor */
__packed typedef struct _CONFIGURATIONDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT16  wTotalLength;
    UINT8   bNumInterfaces;
    UINT8   bConfigurationValue;
    UINT8   iConfiguration;
    UINT8   bmAttributes;
    UINT8   bMaxPower;
} CONFIGDESCRIPTOR;

/* Interface Association Descriptor*/
__packed typedef struct _IADDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bFirstInterface;
    UINT8   bInterfaceCount;
    UINT8   bFunctionClass;
    UINT8   bFunctionSubClass;
    UINT8   bFunctionProtocol;
    UINT8   iFunction;
} IADDESCRIPTOR;

/* Interface Descriptor */
__packed typedef struct _INTERFACEDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bInterfceNumber;
    UINT8   bAlternateSetting;
    UINT8   bNumEndpoints;
    UINT8   bInterfaceClass;
    UINT8   bInterfaceSubClass;
    UINT8   bInterfaceProtocol;
    UINT8   iInterface;    
} INTERFACEDESCRIPTOR;

/* Class-specific Interface Descriptor */
__packed typedef struct _CSINTERFACEDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT16  bcdUVC;
    UINT16  wTotalLength;
    UINT32  dwClockFrequency;
    UINT8   bInCollection;
    UINT8   baInterfaceNr;
} CSINTERFACEDESCRIPTOR;

/* Input Terminal Descriptor */
__packed typedef struct _INPUTTERMINALDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bTerminalID;
    UINT16  wTerminalType;
    UINT8   bAssocTerminal;
    UINT8   iTerminal;
    UINT16  wObjectiveFocalLengthMin;
    UINT16  wObjectiveFocalLengthMax;
    UINT16  wOcularFocalLength;
    UINT8   bControlSize;        
    UINT8	bmControls0;    
    UINT8	bmControls1; 
    UINT8	bmControls2;         
} INPUTTERMINALDESCRIPTOR;

/* Output Terminal Descriptor */
__packed typedef struct _OUTPUTTERMINALDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bTerminalID;
    UINT16  wTerminalType;
    UINT8   bAssocTerminal;
    UINT8   bSourceID;
    UINT8   iTerminal;
} OUTPUTTERMINALDESCRIPTOR;


/* Processing Unit Descriptor */
__packed typedef struct _PUDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bUnitID;
    UINT8   bSourceID;
    UINT16  wMaxMultiplier;
    UINT8   bControlSize;
    UINT8   bmControls0;
    UINT8   bmControls1; 
    UINT8   iProcessing;    
} PUDESCRIPTOR;
/* Extension Unit Descriptor */
__packed typedef struct _EXUDESCRIPTOR2 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bUnitID;
    UINT32  dwGuidExtensionCode0;
    UINT32  dwGuidExtensionCode1;
    UINT32  dwGuidExtensionCode2;
    UINT32  dwGuidExtensionCode3;  
    UINT8	bNumControls;
    UINT8	bNrInPins;  
    UINT8	baSourceID;  
    UINT8   bControlSize;
    UINT8   bmControls0;
    UINT8   bmControls1; 
    UINT8   iExtension;    
} EXUDESCRIPTOR2;
/* Extension Unit Descriptor */
__packed typedef struct _EXUDESCRIPTOR3 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bUnitID;
    UINT32  dwGuidExtensionCode0;
    UINT32  dwGuidExtensionCode1;
    UINT32  dwGuidExtensionCode2;
    UINT32  dwGuidExtensionCode3;            
    UINT8	bNumControls;
    UINT8	bNrInPins;  
    UINT8	baSourceID;  
    UINT8   bControlSize;
    UINT8   bmControls0;
    UINT8   bmControls1; 
    UINT8   bmControls2;     
    UINT8   iExtension;    
} EXUDESCRIPTOR3;

/* Endpoint Descriptor */
__packed typedef struct _ENDPOINTDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bEndpointAddress;
    UINT8   bmAttributes;
    UINT16  wMaxPacketSize;
    UINT8   bInterval;
} ENDPOINTDESCRIPTOR;

/* Class-specific Interrupt Endpoint Descriptor */
__packed typedef struct _CSINTERRUPTEPDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT16  wMaxPacketSize;
} CSINTERRUPTEPDESCRIPTOR;

/* Class-specific Header Descriptor */
__packed typedef struct _CSHEADERDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bNumFormats;
    UINT16  wTotalLength;
    UINT8   bEndpointAddress;
    UINT8   bmInfo;
    UINT8   bTerminalLink;
    UINT8   bStillCaptureMethod;
    UINT8   bTriggerSupport;
    UINT8   bTriggerUsage;
    UINT8   bControlSize;                
    UINT8   bmaControls0;  
	UINT8   bmaControls1;      
	UINT8   bmaControls2;  	
} CSHEADERDESCRIPTOR;

/* Class-specific Header Descriptor */
__packed typedef struct _CSHEADERDESCRIPTOR_BOTH {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bNumFormats;
    UINT16  wTotalLength;
    UINT8   bEndpointAddress;
    UINT8   bmInfo;
    UINT8   bTerminalLink;
    UINT8   bStillCaptureMethod;
    UINT8   bTriggerSupport;
    UINT8   bTriggerUsage;
    UINT8   bControlSize;                
    UINT8   bmaControls0;       
    UINT8   bmaControls1;   
} CSHEADERDESCRIPTOR_BOTH;

/* Class-specific Format Descriptor */
__packed typedef struct _CSFORMATDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFormatIndex;
    UINT8   bNumFrameDescriptors;
    UINT8   bmFlags;
    UINT8   bDefaultFrameIndex;
    UINT8   bAspectRatioX;
    UINT8   bAspectRatioY;
    UINT8   bmInterlaceFlags;
    UINT8   bCopyProtect;
} CSFORMATDESCRIPTOR_MJPEG;


/* Class-specific Frame Descriptor */
__packed typedef struct _CSFRAMEDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;
    UINT32  dwMinFrameInterval_2; 
    UINT32  dwMinFrameInterval_3; 
    UINT32  dwMinFrameInterval_4; 
    UINT32  dwMinFrameInterval_5; 
    UINT32  dwMinFrameInterval_6;      
} CSFRAMEDESCRIPTOR_MJPEG;

/* Class-specific Still Image Frame Descriptor */
__packed typedef struct _CSSTILLFRAMEDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bEndpointAddress;
    UINT8   bNumImageSizePatterns;
    UINT16  wWidth_1;
    UINT16  wHeight_1;
    UINT16  wWidth_2;
    UINT16  wHeight_2;
    UINT16  wWidth_3;
    UINT16  wHeight_3;
    UINT8   bNumCompressionPtn;
} CSSTILLFRAMEDESCRIPTOR;

/* Uncompressed Video Format Descriptor */
__packed typedef struct _UNCOMPRESSFORMATDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bNumFrameDescriptors;
    UINT8   gudiFormat[16];
    UINT8   bBitsPerPixel;
    UINT8   bDefaultFrameIndex;
    UINT8   bAspectRatioX;
    UINT8   bAspectRatioY;
    UINT8   bmInterlaceFlags;
    UINT8   bCopyProtect;
} UNCOMPRESSFORMATDESCRIPTOR;

/* Uncompressed Video Frame Descriptor */
__packed typedef struct _FRAMEDESCRIPTOR_1 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufferSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;   
} FRAMEDESCRIPTOR_1;

/* Uncompressed Video Frame Descriptor */
__packed typedef struct _FRAMEDESCRIPTOR_2 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufferSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;
    UINT32  dwMinFrameInterval_2;   
} FRAMEDESCRIPTOR_2;
__packed typedef struct _FRAMEDESCRIPTOR_3 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufferSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;
    UINT32  dwMinFrameInterval_2; 
    UINT32  dwMinFrameInterval_3; 
} FRAMEDESCRIPTOR_3;

__packed typedef struct _FRAMEDESCRIPTOR_4 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufferSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;
    UINT32  dwMinFrameInterval_2; 
    UINT32  dwMinFrameInterval_3; 
    UINT32  dwMinFrameInterval_4;   
} FRAMEDESCRIPTOR_4;

__packed typedef struct _FRAMEDESCRIPTOR_6 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufferSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;
    UINT32  dwMinFrameInterval_2; 
    UINT32  dwMinFrameInterval_3; 
    UINT32  dwMinFrameInterval_4; 
    UINT32  dwMinFrameInterval_5; 
    UINT32  dwMinFrameInterval_6;      
} FRAMEDESCRIPTOR_6;

__packed typedef struct _FRAMEDESCRIPTOR_7 {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bFrameIndex;
    UINT8   bmCapabilities;
    UINT16  wWidth;
    UINT16  wHeight;
    UINT32  dwMinBitRate;
    UINT32  dwMaxBitRate;
    UINT32  dwMaxVideoFrameBufferSize;
    UINT32  dwDefaultFrameInterval;
    UINT8   bFrameIntervalType;
    UINT32  dwMinFrameInterval_1;
    UINT32  dwMinFrameInterval_2; 
    UINT32  dwMinFrameInterval_3; 
    UINT32  dwMinFrameInterval_4; 
    UINT32  dwMinFrameInterval_5; 
    UINT32  dwMinFrameInterval_6;      
    UINT32  dwMinFrameInterval_7;    
} FRAMEDESCRIPTOR_7;



/* Color Matching Descriptor */
__packed typedef struct _COLORMATCHDESCRIPTOR {
    UINT8   bLength;
    UINT8   bDescriptorType;
    UINT8   bDescriptorSubType;
    UINT8   bColorPrimaries;
    UINT8   bTransferCharacteristics;
    UINT8   bMatrixCoefficients;
} COLORMATCHDESCRIPTOR;


__packed typedef struct _VIDEOSTREAMCMDDATA {
    UINT16  bmHint;
    UINT8   bFormatIndex;
    UINT8   bFrameIndex;    
    UINT32  dwFrameInterval;        
    UINT16  dwKeyFrameRate;    
    UINT16  wPFrameRate;    
    UINT16  wCompQuality;     
    UINT16  wCompWindowSize;    
    UINT16  wDelay;    
    UINT32  dwMaxVideoFrameSize; 
    UINT32  dwMaxPayloadTransferSize;           
} VIDEOSTREAMCMDDATA;

__packed typedef struct _VIDEOSTREAMSTILLCMDDATA {
    UINT8   bFormatIndex;
    UINT8   bFrameIndex;    
    UINT8	bCompressionIndex;
    UINT32  dwMaxVideoFrameSize;        
    UINT32	dwMaxPayloadTranferSize;   
} VIDEOSTREAMSTILLCMDDATA;


__packed typedef struct VIDEOCLASS_BOTH {
    CONFIGDESCRIPTOR                VideoClassConfig;    
    IADDESCRIPTOR                   VideoClassIADIF; 
    INTERFACEDESCRIPTOR             VideoClassVCIF;
    CSINTERFACEDESCRIPTOR           VideoClassCSVCIF;
    INPUTTERMINALDESCRIPTOR         VideoClassCameraIT;	 
    PUDESCRIPTOR                    VideoClassPU;
    EXUDESCRIPTOR2					VideoClassEXU0;
    EXUDESCRIPTOR2					VideoClassEXU1;    
    EXUDESCRIPTOR3					VideoClassEXU2;
    EXUDESCRIPTOR2					VideoClassEXU3; 
    EXUDESCRIPTOR2					VideoClassEXU4; 
  	EXUDESCRIPTOR2					VideoClassEXU5;      
  
    OUTPUTTERMINALDESCRIPTOR        VideoClassOutOT;
  	 
  	 
    ENDPOINTDESCRIPTOR              VideoClassEP3;          //Standard Interrupt Endpoint 3
    CSINTERRUPTEPDESCRIPTOR         VideoClassCSEP3;        //Class Specific Interrupt Endpoint
    
    INTERFACEDESCRIPTOR             VideoClassVSIF;    
    CSHEADERDESCRIPTOR		        VideoClassHeader; 
      
    UNCOMPRESSFORMATDESCRIPTOR      VideoClassFormat1;
  
    FRAMEDESCRIPTOR_7  				VideoClassFrame_YUY2_1;
   	FRAMEDESCRIPTOR_7	      		VideoClassFrame_YUY2_2;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_3;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_4;
   	FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_5;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_6;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_7;
   	FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_8;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_9;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_YUY2_10;
 	FRAMEDESCRIPTOR_6  				VideoClassFrame_YUY2_11;
   	FRAMEDESCRIPTOR_6       		VideoClassFrame_YUY2_12;
    FRAMEDESCRIPTOR_4       		VideoClassFrame_YUY2_13;
    FRAMEDESCRIPTOR_4       		VideoClassFrame_YUY2_14;
   	FRAMEDESCRIPTOR_3       		VideoClassFrame_YUY2_15;
    FRAMEDESCRIPTOR_2       		VideoClassFrame_YUY2_16;
    FRAMEDESCRIPTOR_1       		VideoClassFrame_YUY2_17;
   	FRAMEDESCRIPTOR_1       		VideoClassFrame_YUY2_18;
    FRAMEDESCRIPTOR_1       		VideoClassFrame_YUY2_19;            
    

    COLORMATCHDESCRIPTOR            VideoClassColrMatch0;

    char							H264_INTERFACE[0x1C];
    char							VideoClassFrame_H264_1[0x36];
    char							VideoClassFrame_H264_2[0x36];
    char							VideoClassFrame_H264_3[0x36];
    char							VideoClassFrame_H264_4[0x36];
    char							VideoClassFrame_H264_5[0x36];
    char							VideoClassFrame_H264_6[0x36];
    char							VideoClassFrame_H264_7[0x36];
    char							VideoClassFrame_H264_8[0x36];
    char							VideoClassFrame_H264_9[0x36];
    char							VideoClassFrame_H264_10[0x36];
    char							VideoClassFrame_H264_11[0x36];
    char							VideoClassFrame_H264_12[0x36];
    char							VideoClassFrame_H264_13[0x36];
    char							VideoClassFrame_H264_14[0x36];
    char							VideoClassFrame_H264_15[0x36];                                                        
    char							VideoClassFrame_H264_16[0x36];    
    char							VideoClassFrame_H264_17[0x36];    
 
    COLORMATCHDESCRIPTOR            VideoClassColrMatch1;
       
    CSFORMATDESCRIPTOR_MJPEG		VideoClassFormat2;
    FRAMEDESCRIPTOR_7  				VideoClassFrame_MJPEG_1;
   	FRAMEDESCRIPTOR_7	      		VideoClassFrame_MJPEG_2;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_3;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_4;
   	FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_5;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_6;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_7;
   	FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_8;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_9;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_10;
 	FRAMEDESCRIPTOR_7  				VideoClassFrame_MJPEG_11;
   	FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_12;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_13;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_14;
   	FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_15;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_16;
    FRAMEDESCRIPTOR_7       		VideoClassFrame_MJPEG_17;
  
    COLORMATCHDESCRIPTOR            VideoClassColrMatch2;
   
    INTERFACEDESCRIPTOR             VideoClassIF01;
    ENDPOINTDESCRIPTOR              VideoClassEP01;
    INTERFACEDESCRIPTOR             VideoClassIF02;
    ENDPOINTDESCRIPTOR              VideoClassEP02;  
    INTERFACEDESCRIPTOR             VideoClassIF03;
    ENDPOINTDESCRIPTOR              VideoClassEP03;
    INTERFACEDESCRIPTOR             VideoClassIF04;
    ENDPOINTDESCRIPTOR              VideoClassEP04;  
    INTERFACEDESCRIPTOR             VideoClassIF05;
    ENDPOINTDESCRIPTOR              VideoClassEP05;
    INTERFACEDESCRIPTOR             VideoClassIF06;
    ENDPOINTDESCRIPTOR              VideoClassEP06;  
    INTERFACEDESCRIPTOR             VideoClassIF07;
    ENDPOINTDESCRIPTOR              VideoClassEP07;
    INTERFACEDESCRIPTOR             VideoClassIF08;
    ENDPOINTDESCRIPTOR              VideoClassEP08;  
    INTERFACEDESCRIPTOR             VideoClassIF09;
    ENDPOINTDESCRIPTOR              VideoClassEP09;
    INTERFACEDESCRIPTOR             VideoClassIF10;
    ENDPOINTDESCRIPTOR              VideoClassEP10;  
    INTERFACEDESCRIPTOR             VideoClassIF11;
    ENDPOINTDESCRIPTOR              VideoClassEP11;                 
    char                            ch1[0xc1];      
} VIDEOCLASS;  
typedef UINT32 (PFN_UVCD_PUCONTROL_CALLBACK)(UINT32 u32ItemSelect,UINT32 u32Value);
typedef void   (PFN_UAVCD_ISOINT_CALLBACK) (PUINT32 u32Address, PUINT32 u32Len);

typedef struct{
	VIDEOSTREAMCMDDATA  		VSCmdCtlData;
	UINT16 u16Dummy;
	VIDEOSTREAMSTILLCMDDATA 	VSStillCmdCtlData;
	CaptureFilter 				CapFilter;	
	FeatureFilter				AudioFeat;
	UINT32	u32FrameIndx;		/* Current Frame index */
	UINT32	u32FormatIndx;		/* Current Format index */
	UINT32	IsoMaxPacketSize[16];
	UINT8	u8ErrCode;			/* UVC Error Code */
	BOOL	bChangeData;		/* Format or Frame index Change glag */	
	PFN_UVCD_PUCONTROL_CALLBACK *PU_CONTROL;  
// audio setting 
	FeatureFilter				FeatFilter;
	PFN_UAVCD_ISOINT_CALLBACK    *Video_IsoIn_ISR;
	PFN_UAVCD_ISOINT_CALLBACK    *Audio_IsoIn_ISR;
}UVC_INFO_T;

typedef struct{
	INT32	PU_BACKLIGHT_COMPENSATION_MIN;
	INT32	PU_BACKLIGHT_COMPENSATION_MAX;
	INT32	PU_BACKLIGHT_COMPENSATION_DEF;
	INT32	PU_BRIGHTNESS_MIN;
	INT32	PU_BRIGHTNESS_MAX;
	INT32	PU_BRIGHTNESS_DEF;
	INT32	PU_CONTRAST_MIN;
	INT32	PU_CONTRAST_MAX;
	INT32	PU_CONTRAST_DEF;
	INT32	PU_HUE_MIN;
	INT32	PU_HUE_MAX;
	INT32	PU_HUE_DEF;
	INT32	PU_SATURATION_MIN;
	INT32	PU_SATURATION_MAX;
	INT32	PU_SATURATION_DEF;
	INT32	PU_SHARPNESS_MIN;
	INT32	PU_SHARPNESS_MAX;
	INT32	PU_SHARPNESS_DEF;
	INT32	PU_GAMMA_MIN;
	INT32	PU_GAMMA_MAX;
	INT32	PU_GAMMA_DEF;
	INT32  PU_POWER_LINE_FREQUENCY_MIN;
	INT32  PU_POWER_LINE_FREQUENCY_MAX;
	INT32  PU_POWER_LINE_FREQUENCY_DEF; 
}UVC_PU_INFO_T;

typedef struct{
	UINT32	MaxVideoFrameSize;
	UINT32 	snapshotMaxVideoFrameSize;	
	UINT32 	FormatIndex;
	UINT32	FrameIndex;
	UINT32 	snapshotFormatIndex;
	UINT32	snapshotFrameIndex;	
	UINT32	StillImage;
	BOOL	bReady;
	BOOL	bReady_Audio;
	BOOL	bH264Enable;
	UINT8	H264A[5];	
	UINT8	H264B[6];
}UVC_STATUS_T;

/* Preview Image Frame Index */
#define UVC_640x480			1
#define UVC_160x90			2
#define UVC_160x120			3		
#define UVC_176x144			4
#define UVC_320x180			5		
#define UVC_320x240			6
#define UVC_352x288			7		
#define UVC_432x240			8
#define UVC_640x360			9
#define UVC_800x448			10
#define UVC_800x600			11
#define UVC_864x480			12
#define UVC_960x720			13
#define UVC_1024x576		14
#define UVC_1280x720		15
#define UVC_1600x896		16
#define UVC_1920x1080		17

/* Still Image Frame Index */
#define UVC_STILL_VGA		1
#define UVC_STILL_QVGA		2
#define UVC_STILL_QQVGA		3	

/* Image Frame Size */
#define UVC_SIZE_VGA		0x00096000	/* 640*480 */
#define UVC_SIZE_QVGA		0x00025800	/* 320*240 */
#define UVC_SIZE_QQVGA		0x00009600	/* 160*120 */

#define UVC_SIZE_640x480		0x00096000
#define UVC_SIZE_160x90			0x00007080
#define UVC_SIZE_160x120		0x00009600		
#define UVC_SIZE_176x144		0x0000C600
#define UVC_SIZE_320x180		0x0001C200		
#define UVC_SIZE_320x240		0x00025800
#define UVC_SIZE_352x288		0x00031800		
#define UVC_SIZE_432x240		0x00032A00
#define UVC_SIZE_640x360		0x00070800
#define UVC_SIZE_800x448		0x000AF000
#define UVC_SIZE_800x600		0x000EA600
#define UVC_SIZE_864x480		0x000CA800
#define UVC_SIZE_960x720		0x00151800
#define UVC_SIZE_1024x576		0x00120000
#define UVC_SIZE_1280x720		0x001C2000
#define UVC_SIZE_1600x896		0x002BC000
#define UVC_SIZE_1920x1080		0x003F4800
/* Preview and Snapshot Format Index */	  	
#define UVC_Format_YUY2		1
#define UVC_Foramt_H264		2
#define UVC_Foramt_MJPEG	3

extern  volatile USBD_STATUS_T usbdStatus;
extern  volatile UVC_STATUS_T uvcStatus;

extern  volatile UVC_PU_INFO_T	uvcPuInfo;

VOID uavcdInit(PFN_UVCD_PUCONTROL_CALLBACK* callback_func, PFN_UAVCD_ISOINT_CALLBACK VideoCallback, PFN_UAVCD_ISOINT_CALLBACK* IsoInt_callback);
BOOL uvcdSendImage(UINT32 u32Addr, UINT32 u32transferSize, BOOL bStillImage);
BOOL uvcdIsReady(VOID);
BOOL uvcdSendAudio(UINT32 u32Addr, UINT32 u32transferSize, BOOL bStillImage);
BOOL uacdIsReady(VOID);

/* Feature Unit Control Selectors for Audio*/
#define FU_MUTE_MIN					0
#define FU_MUTE_MAX					1
#define	FU_VOLUME_MIN				0
#define	FU_VOLUME_MAX				0x1400
#define	FU_VOLUME_RES				1			
#define FU_VOLUME_DEF				2
#define FU_VOLUME_CUR				0x1000
