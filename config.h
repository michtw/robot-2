#ifndef __DISPATCH__
#define __DISPATCH__

#define DSOCKET_PATH "/tmp/.covia_ipc"

#define CGI_SOCKET_FILE    "/tmp/cgi-socket"
#define STM_SOCKET_FILE    "/tmp/stm-socket"
#define DIS_STM_SOCKET_FILE     "/tmp/dispatcher-stm-socket"
#define DIS_SER2_SOCKET_FILE    "/tmp/dispatcher-service2-socket"
#define DIS_SER4_SOCKET_FILE    "/tmp/dispatcher-service4-socket"

#define SER2_DIS_SOCKET_FILE    "/tmp/service2-dispatcher-socket"
#define SER4_DIS_SOCKET_FILE    "/tmp/service4-dispatcher-socket"  // service4 to dispatcher
#define STM_DIS_SOCKET_FILE     "/tmp/stm-dispatcher-socket"  // STM service to dispatcher

#define CGI2STM         0x31
#define CGI2SERVICE2    0x32
#define CGI2SERVICE4    0x34

#define STM2SERVICE2    0x12
#define STM2CGI         0x13
#define STM2SERVICE4    0x14

#define SERVICE22STM        0x21
#define SERVICE22CGI        0x23 
#define SERVICE22SERVICE4   0x24

#define SERVICE42STM        0x41
#define SERVICE42SERVICE2   0x42
#define SERVICE42CGI        0x43 



#define STM2DIS        0x10
#define SERVICE22DIS   0x20
#define SERVICE42DIS   0x40

#define GetSsID   0xA002
#define GetWlan   0xA003
#define SetWlan   0xA004



/* CGI -> STM commands.	*/
#define AutoDrive         0xA301
#define ManualDrive       0xA302
#define GoHomeAndDock     0xA303
#define GetMovingStatus   0xA304
#define GetJobStatus      0xA305
#define GetErrorCode      0xA306

// <Substitution Command>
#define SubstitutionSpeech         0xA401
#define StartSubstitutionSpeech    0xA402  // STM->ARM11

// <Reservation Time>
#define SetTimer     0xA501
#define TimerOn      0xA502
#define TimerOff     0xA503
#define GetTimer     0xA504

// <Character Setting>
#define SetLanguage      0xA601
#define SetExpression    0xA602
#define SetCharacter     0xA603
#define GetLanguage      0xA604
#define GetExpression    0xA605
#define Getcharacter     0xA606

// <Variety Setting>
#define SetKeySound     0xA701
#define SetBirthday     0xA702
#define SetTime         0xA703
#define GetKeySound     0xA704
#define GetBirthday     0xA705
#define GetFeeling      0xA706

#define REGISTER        0x9001
#define ACK             0x9003

#ifdef DEBUG
#define DBG(fmt, args...) \
               printf(fmt, ##args)
#else
#define DBG(fmt, args...) 
#endif

#endif
