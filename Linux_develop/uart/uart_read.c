#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define ROBOT_VISION_DOT_NUM              (640)

#define VISION_DOT_LEFT_START             (0)
#define VISION_DOT_LEFT_END               (ROBOT_VISION_DOT_NUM/3)
#define VISION_DOT_FORWARD_START          (ROBOT_VISION_DOT_NUM/3+1)
#define VISION_DOT_FORWARD_END            (2*ROBOT_VISION_DOT_NUM/3)
#define VISION_DOT_RIGHT_START            (2*ROBOT_VISION_DOT_NUM/3+1)
#define VISION_DOT_RIGHT_END              (ROBOT_VISION_DOT_NUM)

//#define UART_RX_BUFF_SIZE                 (ROBOT_VISION_DOT_NUM*10)
#define UART_RX_BUFF_SIZE                 (4096)


#define VISION_INFO_EFFECTIVE             (1)
#define VISION_INFO_UNEFFECTIVE           (0)

#define VISION_FRAME_START                ("START\n")
#define VISION_FRAME_END                  ("END\n")


int g_uart3_fileid;
char g_VisionInfo[UART_RX_BUFF_SIZE];
unsigned int g_dot_distance[ROBOT_VISION_DOT_NUM];

#define MOVE_FORWARD                      (1)
#define MOVE_BACK                         (2)
#define MOVE_LEFT                         (3)
#define MOVE_RIGHT                        (4)
#define MOVE_STOP                         (5)




int SerialPort_init()
{
	printf("\n********  UART3 TEST!!  ********\n");
	int fd = -1;
	fd = open("/dev/ttyAMA3", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		perror("Open Serial Port Error!\n");
		return -1;
	}

	struct termios options;
	tcgetattr(fd, &options);

	//115200, 8N1
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VTIME]=0;
	options.c_cc[VMIN]=1;
	tcflush(fd, TCIFLUSH);

	tcsetattr(fd, TCSANOW, &options);

	g_uart3_fileid = fd;

	return 0;
}



char* SearchStrFormStr(char* src, int src_len, char* search, int search_len)
{
    int i;
    int ret;

    for (i = 0; i < src_len; i++)
    {
        if (src[i] == search[0])
        {
            ret = memcmp(&src[i], &search[0], search_len);
            if (0 == ret)
            {
                return &src[i];
            }
        }
    }

    if (i == src_len)
    {
        return NULL;
    }
}
   
void SerialPort_read()
{    
    int count;
    int index = 0;
    int size;
    char* pstart = NULL;
    int info_effective = VISION_INFO_UNEFFECTIVE;
    unsigned char rx_buffer[256];
    unsigned int timeout = 0xFFFFFFFF;
    char start[] = "5a5a5a5a5a5a5a5a\n";
    char end[]   = "a5a5a5a5a5a5a5a5\n";
    
    printf("***************************************\n");
    
    memset((void*)g_VisionInfo, '\0', sizeof(g_VisionInfo));

    while ((!info_effective) && (timeout > 0))
    {
        count = 0;
        memset((void*)rx_buffer, '\0', sizeof(rx_buffer));
        
        size = read(g_uart3_fileid, (void*)rx_buffer, sizeof(rx_buffer));
        if (size > 0)
        {
            pstart = SearchStrFormStr(rx_buffer, size, start, strlen(start));
            if (pstart)
                continue;
                        
            memcpy((void*)g_VisionInfo, (void*)rx_buffer, size);
            count += size;
            
            while (count < UART_RX_BUFF_SIZE)
            {
                size = read(g_uart3_fileid, (void*)rx_buffer, sizeof(rx_buffer));
                if (size > 0) 
                {
                    memcpy((void*)g_VisionInfo + count, (void*)rx_buffer, size);
                    count += size;
                }
            }

            info_effective = VISION_INFO_EFFECTIVE;            
        }
        else
        {
            timeout--;
        }
    }
    
    printf("%s", g_VisionInfo);
    
    AnalysisVisionInfo();
    
    return;
}

/*************************************************
 * 视觉扫描信息获取
*************************************************/
static void vision_info_read()
{
    int size;
    int count = 0;
    char rx_buffer[512];
    void* pVisionInfo = (void*)g_VisionInfo;

    while (count < UART_RX_BUFF_SIZE)
    {
        size = read(g_uart3_fileid, (void*)rx_buffer, sizeof(rx_buffer));
        if (size > 0)
        {
            if (size + count <= UART_RX_BUFF_SIZE)
            {
                memcpy((void*)g_VisionInfo + count, (void*)rx_buffer, size);
                count += size;
            }
            else
            {
                memcpy((void*)g_VisionInfo + count, (void*)rx_buffer, UART_RX_BUFF_SIZE - count);
                count = UART_RX_BUFF_SIZE;
            }
        }
    }
    
    return;
}


/*************************************************
 * 视觉扫描信息解析
*************************************************/
static void analysis_vision_info(char* vision_info, const unsigned int size)
{
    unsigned int index;
    unsigned int dot_index;
    char temp[20];
    char* p_head = NULL;
    char* p_tail = NULL;

    /* 搜索并跳过第一个\n */
    p_head = strchr(vision_info, '\n');

    index = (unsigned int)p_head - (unsigned int)vision_info;
    index++;

    memset((void*)g_dot_distance, 0, sizeof(g_dot_distance));

    while (index < size-1)
    {
        /* 抓取像素点 */
        p_head = vision_info + index;
        p_tail = strchr(p_head, ' ');

        memset((void*)temp, '\0', sizeof(temp));
        memcpy((void*)temp, p_head, (unsigned int)p_tail - (unsigned int)p_head);

        dot_index = atoi(temp);

        index += (unsigned int)p_tail - (unsigned int)p_head;

        /* 抓取像素点对应距离 */
        p_head = vision_info + index;
        p_tail = strchr(p_head, '\n');

        memset((void*)temp, '\0', sizeof(temp));
        memcpy((void*)temp, p_head, (unsigned int)p_tail - (unsigned int)p_head);

        g_dot_distance[dot_index] = atoi(temp);

        index += (unsigned int)p_tail - (unsigned int)p_head;
        index += 1;        
    }
    
    return;
}

/*************************************************
 * 视觉壁障算法
*************************************************/
int obstacle_avoidance_algorithm()
{
    int i;
    int dot_index = 0;
    int max = g_dot_distance[0];

    for (i = 1; i < sizeof(g_dot_distance)/sizeof(g_dot_distance[0]; i++))
    {
        if (g_dot_distance[i] > max)
        {
            max = g_dot_distance[i];
            dot_index = i;
        }
    }

    if (0 == max)
    {
        return MOVE_STOP;
    } 
    else if ((dot_index >= VISION_DOT_LEFT_START) && (dot_index <= VISION_DOT_LEFT_END))
    {
        return MOVE_LEFT;
    }
    else if ((dot_index >= VISION_DOT_FORWARD_START) && (dot_index <= VISION_DOT_FORWARD_END))
    {
        return MOVE_FORWARD;
    }
    else if ((dot_index >= VISION_DOT_RIGHT_START) && (dot_index <= VISION_DOT_RIGHT_END))
    {
        return MOVE_RIGHT;
    }

    return MOVE_FORWARD;
}

/*************************************************
 * 根据视觉扫描信息计算壁障方向
*************************************************/
int get_move_direction()
{
    vision_info_read();

    analysis_vision_info();

    return obstacle_avoidance_algorithm();
}

int main()
{            
    SerialPort_init();

    struct itimerval value, ovalue;
            
    signal(SIGALRM, SerialPort_read);
    
    value.it_value.tv_sec     = 5;
    value.it_value.tv_usec    = 0;
    value.it_interval.tv_sec  = 5;
    value.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &value, &ovalue);
    
    for (;;);
            
    return 0;     
}



