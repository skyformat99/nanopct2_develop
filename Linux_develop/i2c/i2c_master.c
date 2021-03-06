/******************************************************************************

                  版权所有 (C), 2001-2011, RobotAI.club: zlg

 ******************************************************************************
  文 件 名   : i2c_master.c
  版 本 号   : 初稿
  作    者   : zlg
  生成日期   : 2016年5月24日
  最近修改   :
  功能描述   : i2c_master文件
  函数列表   :
              i2c_read_proc
              i2c_write_proc
              main
              xfm20512_enter_wakeup
              xfm20512_exit_wakeup
              xfm20512_get_degree
              xfm20512_get_version
  修改历史   :
  1.日    期   : 2016年5月24日
    作    者   : zlg
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#define xfm20512_ADDR           (0x47)
#define I2C_BUF_LEN             (256)
#define DELAY_MS(ms)            usleep((ms) * 1000)

/*****************************************************************************
 函 数 名  : i2c_write_proc
 功能描述  : I2C写处理
 输入参数  : int fd              
             unsigned char addr  
             unsigned char reg   
             unsigned char *val  
             unsigned char len   
 输出参数  : 无
 返 回 值  : static
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年5月24日
    作    者   : zlg
    修改内容   : 新生成函数

*****************************************************************************/
static int i2c_write_proc
(
    int fd, 
    unsigned char addr, 
    unsigned char reg, 
    unsigned char *val, 
    unsigned char len
)
{
    unsigned char buf[I2C_BUF_LEN];
    struct i2c_rdwr_ioctl_data cmd;
    struct i2c_msg msg[1];

    memset(buf, 0, sizeof(buf));    // zero memset buffer
    buf[0] = reg;                   // The first byte indicates which register we'll write
    memcpy(&buf[1], val, len);      // The last bytes indicates the values to write
    
    /* Construct the i2c_msg struct */
    msg[0].addr  = addr;                // Device address
    msg[0].flags = 0;               // Write option
    msg[0].len   = len + 1;         // Include register byte
    msg[0].buf   = buf;             // Data buffer

    /* Construct the i2c_rdwr_ioctl_data struct */
    cmd.msgs  = msg;                 // Message
    cmd.nmsgs = sizeof(msg) / sizeof(struct i2c_msg);

    /* Transfer the i2c command packet to the kernel and verify it worked */
    if (ioctl(fd, I2C_RDWR, &cmd) < 0) 
    {
        printf("Unable to send data!\n");
        return 1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : i2c_read_proc
 功能描述  : I2C读处理
 输入参数  : int fd              
             unsigned char addr  
             unsigned char reg   
             unsigned char *val  
             unsigned char len   
 输出参数  : 无
 返 回 值  : static
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年5月24日
    作    者   : zlg
    修改内容   : 新生成函数

*****************************************************************************/
static int i2c_read_proc
(
    int fd, 
    unsigned char addr, 
    unsigned char reg, 
    unsigned char *val, 
    unsigned char len
) 
{
    struct i2c_rdwr_ioctl_data cmd;
    struct i2c_msg msg[2];

    msg[0].addr  = addr;
    msg[0].flags = 0;
    msg[0].len   = 1;
    msg[0].buf   = &reg;
    
    msg[1].addr  = addr;
    msg[1].flags = I2C_M_RD /* | I2C_M_NOSTART*/;
    msg[1].len   = len;
    msg[1].buf   = val;
    
    /* Construct the i2c_rdwr_ioctl_data struct */
    cmd.msgs  = msg;
    cmd.nmsgs = sizeof(msg) / sizeof(struct i2c_msg);

    /* Send the request to the kernel and get the result back */
    if (ioctl(fd, I2C_RDWR, &cmd) < 0) 
    {
        printf("Unable to send data!\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : xfm20512_get_version
 功能描述  : 获取版本信息
 输入参数  : int fd                 
             unsigned int *version  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年5月24日
    作    者   : zlg
    修改内容   : 新生成函数

*****************************************************************************/
int xfm20512_get_version(int fd, unsigned int *version)
{
    unsigned int data = 0x00000F00;

    /* 1. Send version query command */
    if (i2c_write_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, 4))
        return -1;
    DELAY_MS(1);    // Delay 1ms at least

    /* 2. Query command status */
    do {
        if (i2c_read_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, 4))
            return -1;
        DELAY_MS(1);    // Delay 1ms at least
    } while (0x00020001 != data);
    
    if (i2c_read_proc(fd, xfm20512_ADDR, 0x01, (unsigned char *)version, 8))
      return -1;
    
    return 0;
}

/*****************************************************************************
 函 数 名  : xfm20512_get_degree
 功能描述  : 获取角度信息
 输入参数  : int fd                
             unsigned int *degree  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年5月24日
    作    者   : zlg
    修改内容   : 新生成函数

*****************************************************************************/
int xfm20512_get_degree(int fd, unsigned int *degree)
{
    unsigned int data = 0x00001000;

    /* 1. Send degree query command */
    if (i2c_write_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, 4))
        return -1;
    DELAY_MS(1);    // Delay 1ms at least

    /* 2. Query command status */
    do {
        if (i2c_read_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, 4))
            return -1;
        DELAY_MS(1);    // Delay 1ms at least
    } while (0x00010001 != data);
        
    /* 3. Read wakeup degree */
    if (i2c_read_proc(fd, xfm20512_ADDR, 0x01, (unsigned char *)degree, 4))
        return -1;
    
    return 0;
}

int xfm20512_enter_wakeup(int fd)
{
    unsigned int data = 0x00001100;
    
    /* 1. Send enter wakeup command */
    if (i2c_write_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, sizeof(unsigned int)))
        return -1;
    DELAY_MS(1);    // Delay 1ms at least

    /* 2. Query command status */
    do {
        if (i2c_read_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, sizeof(unsigned int)))
            return -1;
        DELAY_MS(1);    // Delay 1ms at least
    } while (0x00000001 != data);
    
    return 0;
}

int xfm20512_exit_wakeup(int fd, unsigned int beam)
{
    unsigned int data = 0x00001200 | ((beam & 0x3) << 16);
    
    /* 1. Send exit wakeup command */
    if (i2c_write_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, sizeof(unsigned int)))
        return -1;
    DELAY_MS(1);    // Delay 1ms at least

    /* 2. Query command status */
    do {
        if (i2c_read_proc(fd, xfm20512_ADDR, 0x00, (unsigned char *)&data, sizeof(unsigned int)))
            return -1;
        DELAY_MS(1);    // Delay 1ms at least
    } while (0x00030001 != data);
    
    return 0;
}

/*****************************************************************************
 函 数 名  : main
 功能描述  : 主处理
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年5月24日
    作    者   : zlg
    修改内容   : 新生成函数

*****************************************************************************/
int main()
{
    printf("\n********  i2c master test!!  ********\n");
    
    int fd = -1;
    int ret;
    int write_count = 0;

    /* 打开master侧的I2C-0接口 */
    fd = open("/dev/i2c-0", O_RDWR);
    if (fd == -1) 
    {
        perror("Open i2c-0 Port Error!\n");
        return -1;
    }

    /* 注册从机 */
    if (ioctl(fd, I2C_SLAVE, xfm20512_ADDR) < 0)
    {
        perror("ioctl error\n");
        return -1;
    }

    /* 查询模块版本信息 */
    unsigned int version;
    
    if (!xfm20512_get_version(fd, &version))
    {
        printf("version = %d\n", version);
    }
    
    /* 获取角度信息 */
    unsigned int degree;
    
    if (!xfm20512_get_degree(fd, &degree))
    {
        printf("degree = %d\n", degree);
    }
    
    close(fd);

    printf("\n********  i2c master test finish !!  ********\n");
    return 0;
}
