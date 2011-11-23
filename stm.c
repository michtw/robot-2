#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "stm.h"

int set_timer_status = 0;
int set_language_status = 0;
int set_character_status = 0;
int set_key_sound_status = 0;
int set_sub_stion_spe_status = -1;
int set_timer2_status = 0;
int set_birthday_status = 0;
int set_status_status = 0;
int set_sensor_status = 0;
int set_time_status = 0;
int set_melody_status = 0;

int language = 0;
int character = 0;
int key_sound = 1;
int feeling = 1;
int feeling2 = 1;

int SpeechTableID = 0;
int Motion = 0;
int Month = 0;
int Day = 0;
int Time = 0;
int Minutes = 0;

int SunTime = 0;
int SunMinutes = 0;
int MonTime = 0;
int MonMinutes = 0;
int TueTime = 0;
int TueMinutes = 0;
int WedTime = 0;
int WedMinutes = 0;
int ThuTime = 0;
int ThuMinutes = 0;
int FriTime = 0;
int FriMinutes = 0;
int SatTime = 0;
int SatMinutes = 0;

int birthday_month = 0;
int birthday_day = 0;

int st_Year = 0;  // SetTime Year
int st_Month = 0;
int st_Day = 0;
int st_Time = 0;
int st_Minutes = 0;

static int moving_status(void)
{
    return 3;
}

static int job_status(void)
{
    return 1;
}

static int error_code(void)
{
    return 0xFF;
}

static void auto_drive(char parm) 
{
    switch (parm) {
            case AD_ACTION_STOP:
		    printf("ACTION_STOP\n");
		    break;
            case AD_AUTO_ACTION:
		    printf("AUTO_ACTION\n");
		    break;
            case AD_SPOT1_ACTION:
		    printf("SPOT1_ACTION\n");
		    break;
            case AD_SPOT2_ACTION:
		    printf("SPOT2_ACTION\n");
		    break;
            case AD_WALL_ACTION:
		    printf("WALL_ACTION\n");
		    break;
            case AD_PCI_ACTION:
		    printf("PCI_ACTION\n");
		    break;
    }
}

static void manual_drive(char parm)
{
    switch (parm) {
            case MD_ACTION_STOP:
		    printf("ACTION_STOP\n");
		    break;
            case MD_FORWARD:
		    printf("FORWARD\n");
		    break;
            case MD_BACK:
		    printf("BACK\n");
		    break;
            case MD_RIGHT_TURN:
		    printf("RIGHT_TURN\n");
		    break;
            case MD_LEFT_TURN:
		    printf("LEFT_TURN\n");
		    break;
    }
}

static void getter(int sock, int cmd)
{
    int len;
    int i, status = 0, idx = 0;
    short sum = 0x00;
 
    unsigned char buf[128];

    buf[idx++] = 0xF1; 
    buf[idx++] = 0xF2; 
    buf[idx++] = 0x08;  // command length
    buf[idx++] = 0x13; 
    buf[idx++] = (cmd >> 8) & 0xff; 
    buf[idx++] = cmd & 0xff; 

    switch (cmd) {
	    case GetMovingStatus:
		    status = moving_status();
		    break;
	    case GetJobStatus:
                    status = job_status();
		    break;
	    case GetErrorCode:
		    status = error_code();
		    break;
	    case TimerOn:
	    case TimerOff:
		    status = set_timer_status;
		    break;
	    case SetLanguage:
		    status = set_language_status;
		    break;
	    case SetCharacter:
		    status = set_character_status;
		    break;
	    case GetLanguage:
		    status = language;
		    break;
	    case GetCharacter:
		    status = character;
		    break;
	    case GetKeySound:
		    status = key_sound;
		    break;
	    case GetFeeling:
		    status = feeling;
		    break;
	    case SetTimer:
		    status = set_timer2_status;
		    break;
	    case SubstitutionSpeech:
		    status = set_sub_stion_spe_status;
		    break;
	    case SetBirthday:
		    status = set_birthday_status;
		    break;
	    case GetFeeling2:
		    status = feeling2;
		    break;
	    case GetStatus:
		    status = set_status_status;
		    break;
	    case GetSensorData:
		    status = set_sensor_status;
		    break;
	    case SetTime:
		    status = set_time_status;
		    break;
	    case PlayMelody:
		    status = set_melody_status;
		    break;
	    default:
		    printf("Unsupported command. (0x%04x)\n", cmd);
		    break;
    }
    
    buf[idx++] = status;

    for (i = 0; i < idx; i++) {
	    sum += buf[i];
    }
    buf[idx++] = sum & 0xff;
#ifdef DEBUG
    printf("Response command: ");
    for (i = 0; i < idx; i++) {
	    printf("0x%02x ", buf[i] & 0xff);
    }
    printf("\n");
#endif
    len = write(sock, buf, idx);
    DBG("Written: %d. \n", len);

    memset(buf, '\0', sizeof(buf));
    len = read(sock, buf, sizeof(buf)); // Read ACK.
    DBG("ACK read: len: %d\n", len);
}

static void startSubStitutionSpeech(int sock, unsigned char table)
{
    int i;
    unsigned char  buf[256];
    int len;
    unsigned char cmd[] = {0xF1, 0xF2, 0x08, 0x12, 0xA4, 0x02, table, 0xFF};

    cmd[7] = (cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5] + cmd[6]) & 0xff;  // checksum

    printf("Speed table: %d\n", table);

    len = write(sock, cmd, sizeof(cmd));  // Send command to STM
    printf("startSubStitutionSpeech command write: %d\n", len);
    memset(buf, '\0', sizeof(buf));
    len = read(sock, buf, sizeof(buf));
    printf("startSubStitutionSpeech ACK read: %d\n", len);
    for (i = 0; i < len; i++) {
	    printf("0x%02x ", buf[i]);
    }
    printf("\n");
}

static void set_timer(int state)
{
    if (state) { // timer on
	    printf("Timer on.\n");
	    set_timer_status = 1;
    } else {
	    printf("Timer off.\n");
	    set_timer_status = 1;
    }	    
}

static int set_language(int lang)
{
    int rtn = -1;
    switch (lang) {
	    case 0:
	    case 1:
	    case 2:
	    case 3:
		    printf("Set language: [%d] OK.", lang);
		    language = lang;
		    rtn = 0;
		    break;
            default:
		    printf("Unsupported language\n");
		    break;
    }
    return rtn;
}

static int set_character(int chater)
{
    int rtn = -1;
    switch (chater) {
	    case 0:
	    case 1:
	    case 2:
		    printf("Set character: [%d] OK.", chater);
		    character = chater;
		    rtn = 0;
		    break;
            default:
		    printf("Unsupported character\n");
		    break;
    }
    return rtn;
}

static int set_key_sound(int sound)
{
    int rtn = -1;
    switch (sound) {
	    case 0:
	    case 1:
		    printf("Set key sound: [%d] OK.", sound);
		    key_sound = sound;
		    rtn = 0;
		    break;
            default:
		    printf("Unsupported key sound\n");
		    break;
    }
    return rtn;
}

static int set_status(int StatusTableNO)
{
    int rtn = -1;

    switch (StatusTableNO) {
        case 0 ... 41:
		rtn = 0;
		break;
    }
    return rtn;
}

static int set_sensor_data(int StatusTableNO)
{
    int rtn = -1;

    switch (StatusTableNO) {
        case 0 ... 30:
		rtn = 0;
		break;
    }
    return rtn;
}

static int play_melody(int MelodyTableNO)
{
    int rtn = -1;

    switch (MelodyTableNO) {
        case 0 ... 3:
		rtn = 0;
		break;
    }
    return rtn;
}

static void setter(int sock, unsigned char *buf)
{
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
    switch (cmd) {
	    case TimerOn:
		    set_timer(1);
		    break;
	    case TimerOff:
		    set_timer(0);
		    break;
	    case SetLanguage:
		    if (set_language(buf[6]) != -1) {
			    set_language_status = 1;
		    } else {
			    set_language_status = 0;
		    }
		    break;
	    case SetCharacter:
		    if (set_character(buf[6]) != -1) {
			    set_character_status = 1;
		    } else {
			    set_character_status = 0;
		    }
		    break;
	    case SetKeySound:
		    if (set_key_sound(buf[6]) != -1) {
			    set_key_sound_status = 1;
		    } else {
			    set_key_sound_status = 0;
		    }
		    break;
	    case GetStatus:
		    if (set_status(buf[6]) != -1) {
			    set_status_status = 1;
		    } else {
			    set_status_status = 0;
		    }
		    break;
	    case GetSensorData:
		    if (set_sensor_data(buf[6]) != -1) {
			    set_sensor_status = 1;
		    } else {
			    set_sensor_status = 0;
		    }
		    break;
	    case PlayMelody:
		    if (play_melody(buf[6]) != -1) {
			    set_melody_status = 1;
		    } else {
			    set_melody_status = 0;
		    }
		    break;

    }
    getter(sock, cmd);
}

static void substitutionSpeech(int sock, unsigned char *buf)
{
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);

    set_sub_stion_spe_status = 1;

    if ((buf[6] >= 0) && (buf[6] <= 4)) {
        SpeechTableID = buf[6];
    } else {
	set_sub_stion_spe_status = 0;
    }
    if ((buf[7] >= 0) && (buf[7] <= 8)) {
        Motion = buf[7];
    } else {
	set_sub_stion_spe_status = 0;
    }
    if ((buf[8] >= 1) && (buf[8] <= 12)) {
        Month = buf[8];
    } else {
	set_sub_stion_spe_status = 0;
    }
    if ((buf[9] >= 1) && (buf[9] <= 31)) {
        Day = buf[9];
    } else {
	set_sub_stion_spe_status = 0;
    }
    if ((buf[10] >= 0) && (buf[10] <= 23)) {
	Time = buf[10];
    } else {
	set_sub_stion_spe_status = 0;
    }
    if ((buf[11] >= 0) && (buf[11] <= 59)) {
	Minutes = buf[11];
    } else {
	set_sub_stion_spe_status = 0;
    }
    getter(sock, cmd);
}

static void set_timer2(int sock, unsigned char *buf)
{
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
    unsigned char v;	

    set_timer2_status = 1;

    v = buf[6];
    if ((v >= 0) && (v <= 23)) {
	SunTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[7];
    if ((v >= 0) && (v <= 59)) {
	SunMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    v = buf[8];
    if ((v >= 0) && (v <= 23)) {
        MonTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[9];
    if ((v >= 0) && (v <= 59)) {
	MonMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    v = buf[10];
    if ((v >= 0) && (v <= 23)) {
	TueTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[11];
    if ((v >= 0) && (v <= 59)) {
	TueMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    v = buf[12];
    if ((v >= 0) && (v <= 23)) {
	WedTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[13];
    if ((v >= 0) && (v <= 59)) {
	WedMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    v = buf[14];	
    if ((v >= 0) && (v <= 23)) {
	ThuTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[15];	
    if ((v >= 0) && (v <= 59)) {
	ThuMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    v = buf[16];
    if ((v >= 0) && (v <= 23)) {
	FriTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[17];
    if ((v >= 0) && (v <= 59)) {
	FriMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    v = buf[18];
    if ((v >= 0) && (v <= 23)) {
	SatTime = v;
    } else {
	set_timer2_status = 0;
    }
    v = buf[19];
    if ((v >= 0) && (v <= 59)) {
	SatMinutes = v;
    } else {
	set_timer2_status = 0;
    }

    getter(sock, cmd);
}

static void get_birthday(int sock)
{
    int len;
    int i, idx = 0;
    short sum = 0x00;
 
    unsigned char buf[128];

    buf[idx++] = 0xF1; 
    buf[idx++] = 0xF2; 
    buf[idx++] = 9;  // command length
    buf[idx++] = 0x13; 
    buf[idx++] = 0xA7; 
    buf[idx++] = 0x05; 

    buf[idx++] = birthday_month;
    buf[idx++] = birthday_day;

    for (i = 0; i < idx; i++) {
	    sum += buf[i];
    }
    buf[idx++] = sum & 0xff;
#ifdef DEBUG
    printf("Response command: ");
    for (i = 0; i < idx; i++) {
	    printf("0x%02x ", buf[i] & 0xff);
    }
    printf("\n");
#endif
    len = write(sock, buf, idx);
    DBG("Written: %d. \n", len);

    memset(buf, '\0', sizeof(buf));
    len = read(sock, buf, sizeof(buf)); // Read ACK.
    DBG("ACK read: len: %d\n", len);
}

static void get_timer(int sock)
{
    int len;
    int i, idx = 0;
    short sum = 0x00;
 
    unsigned char buf[128];

    buf[idx++] = 0xF1; 
    buf[idx++] = 0xF2; 
    buf[idx++] = 21;  // command length
    buf[idx++] = 0x13; 
    buf[idx++] = 0xA5; 
    buf[idx++] = 0x04; 

    buf[idx++] = SunTime;
    buf[idx++] = SunMinutes;
    buf[idx++] = MonTime;
    buf[idx++] = MonMinutes;
    buf[idx++] = TueTime;
    buf[idx++] = TueMinutes;
    buf[idx++] = WedTime;
    buf[idx++] = WedMinutes;
    buf[idx++] = ThuTime;
    buf[idx++] = ThuMinutes;
    buf[idx++] = FriTime;
    buf[idx++] = FriMinutes;
    buf[idx++] = SatTime;
    buf[idx++] = SatMinutes;

    for (i = 0; i < idx; i++) {
	    sum += buf[i];
    }
    buf[idx++] = sum & 0xff;
#ifdef DEBUG
    printf("Response command: ");
    for (i = 0; i < idx; i++) {
	    printf("0x%02x ", buf[i] & 0xff);
    }
    printf("\n");
#endif
    len = write(sock, buf, idx);
    DBG("Written: %d. \n", len);

    memset(buf, '\0', sizeof(buf));
    len = read(sock, buf, sizeof(buf)); // Read ACK.
    DBG("ACK read: len: %d\n", len);
}

static void set_birthday(int sock, unsigned char *buf)
{
    int v;
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
    set_birthday_status = 1;

    v = buf[6];
    if ((v >= 1) && (v <= 12)) {
        birthday_month = v;	
    } else {
	set_birthday_status = 0;
    }

    v = buf[7];
    if ((v >= 1) && (v <= 31)) {
        birthday_day = v;	
    } else {
	set_birthday_status = 0;
    }

    getter(sock, cmd);
}

static void set_time(int sock, unsigned char *buf)
{
    int v;
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
    set_time_status = 1;

    v = buf[6];
    if ((v >= 1) && (v <= 12)) {
        st_Year = v;	
    } else {
	set_time_status = 0;
    }

    v = buf[7];
    if ((v >= 1) && (v <= 12)) {
        st_Month = v;	
    } else {
	set_time_status = 0;
    }

    v = buf[8];
    if ((v >= 1) && (v <= 31)) {
        st_Day = v;	
    } else {
	set_time_status = 0;
    }

    v = buf[9];
    if ((v >= 0) && (v <= 23)) {
        st_Time = v;	
    } else {
	set_time_status = 0;
    }

    v = buf[10];
    if ((v >= 0) && (v <= 59)) {
        st_Minutes = v;	
    } else {
	set_time_status = 0;
    }

    getter(sock, cmd);
}

void handle_cmd(int sock, unsigned char *buf)
{
    int cmd = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
    int parm = buf[6];

    switch (cmd) {
	    case AutoDrive:
		    auto_drive(parm);
		    break;
	    case ManualDrive:
		    manual_drive(parm);
		    break;
	    case GoHomeAndDock:
		    printf("GoHomeAndDock NOT supported.\n");
		    break;
	    case GetMovingStatus:
	    case GetJobStatus:
	    case GetErrorCode:
	    case GetFeeling:
	    case GetKeySound:
            case GetFeeling2:
		    getter(sock, cmd);
		    break;
	    case StartSubstitutionSpeech:
		    startSubStitutionSpeech(sock, buf[6]);
                    break;
	    case SubstitutionSpeech:
		    substitutionSpeech(sock, buf);
		    break;
	    case SetTimer:
		    set_timer2(sock, buf);
		    break;
	    case GetTimer:
		    get_timer(sock);
		    break;
	    case TimerOn:
	    case TimerOff:
		    setter(sock, buf);
		    break;
	    case SetLanguage:
	    case SetCharacter:
		    setter(sock, buf);
		    break;
	    case GetLanguage:
	    case GetCharacter:
		    getter(sock, cmd);
		    break;
	    case SetKeySound:
		    setter(sock, buf);
		    break;
	    case SetBirthday:
		    set_birthday(sock, buf);
	    case GetBirthday:
		    get_birthday(sock);
		    break;
	    case GetStatus: // Is this SetStatus
	    case GetSensorData:
	    case PlayMelody:
		    setter(sock, buf);
		    break;
	    case SetTime:
                    set_time(sock, buf);
		    break;
	    default:
		    printf("[STM] ERROR: Command not support.\n");
		    break;
    }
}

void *conn(void *ptr)
{
    int sock;
    struct sockaddr_un address;
    size_t addr_length;
    int amount;
    fd_set ready_r;        
    struct timeval to;
    int i;

    unsigned char buf[256];

    const unsigned char register_cmd[] = {0xF1, 0xF2, 0x07, 0x10, 0x90, 0x01, 0x8B};
    const unsigned char ack_cmd[] = {0xF1, 0xF2, 0x07, 0x13, 0x90, 0x03, 0x90};

    if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
	    perror("socket");
	    exit(1);
    }

    address.sun_family = AF_UNIX;    /* Unix domain socket */
    strcpy(address.sun_path, DSOCKET_PATH);

    /* The total length of the address includes the sun_family element */
    addr_length = sizeof(address.sun_family) + strlen(address.sun_path);

    if (connect(sock, (struct sockaddr *) &address, addr_length)) {
	    printf("Cannot register service. (%s)\n", strerror(errno));
	    exit(1);
    }

    amount = write(sock, register_cmd, sizeof(register_cmd));  // Register of service.
    amount = read(sock, buf, sizeof(buf));  // Read ACK from dispatcher.
    if (amount <= 0) {
	    printf("Cannot register service.\n");
	    exit(1);
    }

    do {
	FD_ZERO(&ready_r);
	FD_SET(sock, &ready_r);
	to.tv_sec = 3;
	if (select(sock + 1, &ready_r, 0, 0, &to) < 0) {
		perror("select");
	}
	if (FD_ISSET(sock, &ready_r)) {
		memset(buf, '\0', sizeof(buf));
		amount = read(sock, buf, sizeof(buf)); // Dispatcher request.
		if (amount < 0) {
			perror("read");
			continue;
		}
		printf("[STM] Dispatcher request(%d): ", amount);
		for (i = 0; i < amount; i++) {
			printf("0x%02x ", buf[i] & 0xff);
		}
		printf("\n");

		amount = write(sock, ack_cmd, sizeof(ack_cmd));
		//close(sock);
		printf("ACK write amount: %d\n", amount);

		printf("[STM] ---- done\n");

		usleep(100000);

		handle_cmd(sock, buf);
	} 
    } while (1);

    printf("Close socket.\n");
    close(sock);

    return 0;
}

int main(int argc, char *argv[]) 
{
    int iret_conn;
    pthread_t thread_conn;

    iret_conn = pthread_create(&thread_conn, NULL, conn, (void *)NULL);

    pthread_join(thread_conn, NULL); 

    printf("Thread conn returns: %d\n", iret_conn);
    return 0;
}

