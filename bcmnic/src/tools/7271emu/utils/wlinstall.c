/******************************************************************************
 * Broadcom Proprietary and Confidential. (c)2016 Broadcom. All rights reserved.
 *
 * This program is the proprietary software of Broadcom and/or its licensors,
 * and may only be used, duplicated, modified or distributed pursuant to the terms and
 * conditions of a separate, written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized License, Broadcom grants
 * no license (express or implied), right to use, or waiver of any kind with respect to the
 * Software, and Broadcom expressly reserves all rights in and to the Software and all
 * intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU
 * HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY
 * NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1.     This program, including its structure, sequence and organization, constitutes the valuable trade
 * secrets of Broadcom, and you shall use all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of Broadcom integrated circuit products.
 *
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 * AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 * WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT TO
 * THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL IMPLIED WARRANTIES
 * OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE,
 * LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION
 * OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF
 * USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
 * LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR
 * EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO YOUR
 * USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT
 * ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 * LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF
 * ANY LIMITED REMEDY.
 *****************************************************************************/

/******************************************************************************
 *"wlinstall" utlity automatically does the necessary intilization to run generic wifi driver on any board.
 *
 *Usage:
 *wlinstall <name of the driver binary(optional)> <name of the interface(Optional)>
 *Examples:
 *1. #wlinstall --- Just turn on the wifi radio power . After this step user can do insmod to install the driver
 *2. #wlinstall wl.ko --- Turn on the radio power,copy the nvram file,remove existing wl.ko from kernel
 * and reinstall wl.ko. After this just run the wl commands as usual. Utlity expects nvram file in the current
 * directory and to be in the format boardname.txt e.g bcm97271wlan.txt,bcm97271ip.txt
 *3. #wlinstall wl.ko wlan0  --- Turn on the radio power,copy the nvram file,remove existing wl.ko from kernel
 * and reinstall wl.ko, changes the ame of the network interface to wlan0 . After this just run the wl commands
 * as usual. Utlity expects nvram file in the current directory and to be in the format boardname.txt e.g bcm97271wlan.txt,bcm97271ip.txt
 *
 *How to build:
 * arm-linux-gcc -o wlinstall wlinstall.c
 * or aarch64-linux-gcc -o wlinstall wlinstall.c for 64 bit toolchain
 *
 *What does "wlinstall" do:
 *1. Reads the board information and turns on the wifi radio
 *2. Copies the board specific nvram file to standard nvram.txt e.g bcm97271wlan.txt -> nvram.txt
 *3. If the name of the driver binary specified then
 *	 a) Tries to remove the existing driver
 *	 b) install the driver using insmod command
 *4. If the interface name is specifed, is changes the interfcace name
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


#define BOARD_NAME "/proc/device-tree/bolt/board"
#define SYSFS_GPIO "/sys/class/gpio"
#define AON_GPIO_OFFSET 128

static struct
{
    char * board_name;
    int gpio_num;
}board_info[] = {
                        {"BCM97271WLAN",(AON_GPIO_OFFSET+21)},
                        {"BCM97271DV",-1},
                        {"BCM97271IP",(AON_GPIO_OFFSET+15)},
                        {"BCM97271T",(AON_GPIO_OFFSET+15)},
                        {"BCM97271D",(AON_GPIO_OFFSET+21)},
                        {"BCM97271C",(AON_GPIO_OFFSET+15)},
                        {"BCM97271IPC",(AON_GPIO_OFFSET+15)},
                        {NULL,0},
                    };

static int get_board_name(char *board_name, int size)
{
    FILE *fd = NULL;
    char *tmp = NULL;

    fd= fopen(BOARD_NAME, "r" );
    if(fd == NULL)
    {
        printf("%s: Failed to open %s\n",__FUNCTION__,BOARD_NAME);
        return -1;
    }
    memset((void*)board_name,0,size);
    tmp = fgets(board_name,size,fd);
    if(tmp == NULL)
    {
        printf("%s:Failed to read board name\n",__FUNCTION__,BOARD_NAME);
        fclose(fd);
        return -1;
    }
    fclose(fd);
    return 0;
}

static int wlan_radio_on(int gpio)
{
    char buf[256];
    int fd = 0;

    printf("*** Turning on wlan radio using %s gpio pin %d***\n",
           gpio>AON_GPIO_OFFSET?"aon":"",gpio>AON_GPIO_OFFSET?(gpio-AON_GPIO_OFFSET):gpio);
    if(gpio <0 )
    {
        return 0;
    }

    /* remove the gpio pin if already exported */
    snprintf(buf, sizeof(buf), "%s/unexport", SYSFS_GPIO);
    fd = open(buf, O_WRONLY);
    if (fd == 0)
    {
        printf("%s(): Unable to open %s\n", __FUNCTION__, buf);
        goto error;
    }
    snprintf(buf, sizeof(buf),"%d",gpio);
    write(fd, buf, strlen(buf));
    close(fd);

    /* export gpio */
    snprintf(buf, sizeof(buf), "%s/export", SYSFS_GPIO);
    fd = open(buf, O_WRONLY);
    if (fd == 0)
    {
        printf("%s(): Unable to open %s\n", __FUNCTION__, buf);
        goto error;
    }
    snprintf(buf, sizeof(buf),"%d",gpio);
    if (write(fd, buf, strlen(buf)) < 0)
    {
        printf("%s(): Not able to write gpio %d to %s/export\n", __FUNCTION__,gpio,SYSFS_GPIO);
        goto error;
    }
    close(fd);

    /* change the direction of the pin to output */
    snprintf(buf, sizeof(buf), "%s/gpio%d/direction", SYSFS_GPIO, gpio);
    if ((fd = open(buf, O_WRONLY)) < 0)
    {
        printf("%s(): Unable to open %s/gpio%d/direction\n", __FUNCTION__, SYSFS_GPIO, gpio);
        goto error;
    }
    snprintf(buf, sizeof(buf), "%s", "out");
    if (write(fd, buf, strlen(buf)) < 0)
    {
        printf("%s(): Not able to write %s to %s/gpio%d/direction\n", __FUNCTION__,buf,SYSFS_GPIO, gpio);
        goto error;
    }
    close(fd);

    /* turn on the radio */
    snprintf(buf, sizeof(buf), "%s/gpio%d/value", SYSFS_GPIO, gpio);
    if ((fd = open(buf, O_WRONLY)) < 0)
    {
        printf("%s(): Failed to Open %s/gpio%d/value\n", __FUNCTION__,SYSFS_GPIO, gpio);
        goto error;
    }
    snprintf(buf, sizeof(buf),"%d",1);
    if (write(fd, buf, strlen(buf)) < 0)
    {
        printf("%s(): Not able to write %s to %s/gpio%d/value\n", __FUNCTION__,buf,SYSFS_GPIO, gpio);
        goto error;
    }
    close(fd);
    return 0;

error:
    if (fd > 0)
    {
        close(fd);
    }
    return -1;
}

static void usage(void)
{
    printf( "Usage: wlinstall <driver name(optional)> <intefacename(optional) <name of nvram file(optional)>\n" );
} /* usage */

int main(int argc, char **argv)
{
    int board_index=0,i=0;
    char buf[256];
    char if_name[256];
    int size=0;

    if(get_board_name(buf, sizeof(buf)))
    {
        return -1;
    }

    board_index=0;
    while(board_info[board_index].board_name)
    {
        if(strcasecmp(board_info[board_index].board_name, buf) == 0)
        {
            printf("*** Board[%s] Found! ***\n",board_info[board_index].board_name);
            if(wlan_radio_on(board_info[board_index].gpio_num))
            {
                printf("Failed to turn on radio for %s board on gpio %d\n",board_info[board_index].board_name,board_info[board_index].gpio_num);
                return -1;
             }
             break;
        }
        board_index++;
    }
    if(board_info[board_index].board_name == NULL)
    {
        printf("*** \nPlease add support for %s ***\n",buf);
        return -1;
    }
    if(argc == 1)
    {
        usage();
        return 0;
    }
    /* copy nvram file */
    memset((void*)if_name,0,sizeof(if_name));
    size = strlen(board_info[board_index].board_name);
    for(i=0;i<size;i++)
        if_name[i] = (char)tolower(board_info[board_index].board_name[i]);

    if(argc == 4)
    {
	snprintf(buf, sizeof(buf),"cp -vf %s nvram.txt",argv[3]);
    }
    else
    {
	snprintf(buf, sizeof(buf),"cp -vf %s.txt nvram.txt",if_name);
    }
    printf("*** Copy nvram: %s ***\n",buf);
    system(buf);

    snprintf(buf, sizeof(buf),"rmmod %s",argv[1]);
    printf("*** Uninstall driver: %s ***\n",buf);
    sleep(1);
    system(buf);

    memset((void*)if_name,0,sizeof(if_name));
    if(argc>2)
    {
        snprintf(if_name, sizeof(if_name),"intf_name=%s",argv[2]);
    }
    snprintf(buf, sizeof(buf),"insmod %s %s",argv[1],if_name);
    printf("*** Installing driver: %s\n",buf);
    sleep(1);
    system(buf);
    return 0;
}
