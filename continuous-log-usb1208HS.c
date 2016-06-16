/*
 *  Copyright (c) 2014 Warren J. Jasper <wjasper@tx.ncsu.edu>
 *  Copyright (c) 2016 assignee tbd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>

#include "pmd.h"
#include "usb-1208HS.h"

#define FALSE 0
#define TRUE 1

static int wMaxPacketSize;  // will be the same for all devices of this type so
                            // no need to be reentrant. 

#define BLOCKSIZE 512

int main (int argc, char **argv)
{
  double frequency;

  float table_AIN[NMODE][NGAINS_1208HS][2];
  float table_AO[NCHAN_AO_1208HS][2];

  int usb1208HS_2AO = FALSE;
  int usb1208HS_4AO = FALSE;
  libusb_device_handle *udev = NULL;
  int i, j;
  //int transferred;
  uint8_t gain, mode, channel;
  float volts;
  int ch;
  int ret;

  int fd;
  
  uint16_t sdataIn[BLOCKSIZE];    // holds 13 bit unsigned analog input data
  uint8_t range[NCHAN_1208HS];

  udev = NULL;

  ret = libusb_init(NULL);
  if (ret < 0) {
    perror("usb_device_find_USB_MCC: Failed to initialize libusb");
    exit(1);
  }

  if ((udev = usb_device_find_USB_MCC(USB1208HS_PID, NULL))) {
    printf("Success, found a USB 1208HS!\n");
  } else if ((udev = usb_device_find_USB_MCC(USB1208HS_2AO_PID, NULL))) {
    printf("Success, found a USB 1208HS-2AO!\n");
    usb1208HS_2AO = TRUE;
  } else if ((udev = usb_device_find_USB_MCC(USB1208HS_4AO_PID, NULL))) {
    printf("Success, found a USB 1208HS-4AO!\n");
    usb1208HS_4AO = TRUE;
  } else {
    printf("Failure, did not find a USB 1208HS, 1208HS-2AO or 1208HS-4AO!\n");
    return 0;
  }
  (void) usb1208HS_2AO;
  (void) usb1208HS_4AO;

  usbInit_1208HS(udev);
  usbBuildGainTable_USB1208HS(udev, table_AIN);
  for (i = 0; i < NMODE; i++ ) {
    for (j = 0; j < NGAINS_1208HS; j++) {
      printf("Mode = %d   Gain: %d    Slope = %f    Offset = %f\n", i, j, table_AIN[i][j][0], table_AIN[i][j][1]);
    }
  }

  if (usb1208HS_4AO) {
    usbBuildGainTable_USB1208HS_4AO(udev, table_AO);
    printf("\n");
    for (i = 0; i < NCHAN_AO_1208HS; i++) {
      printf("VDAC%d:    Slope = %f    Offset = %f\n", i, table_AO[i][0], table_AO[i][1]);
    }
  }

  wMaxPacketSize = usb_get_max_packet_size(udev, 0);

  fd = open("data.raw", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    perror("Error opening file.");
    return -1;
  }

	printf("Testing USB-1208HS Analog Input Scan.\n");
	usbAInScanStop_USB1208HS(udev);
  channel = 0;
	printf("Input channel %d",channel);
  ch = '4';
  char * gain_range = "";
	switch(ch) {
	  case '1': gain = BP_10V; gain_range = "+/-10V"; break;
    case '2': gain = BP_5V; gain_range = "+/-5V"; break;
	  case '3': gain = BP_2_5V; gain_range = "+/-2.5V"; break;
	  case '4': gain = UP_10V; gain_range = "0-10V SE"; break;
	  default:  gain = BP_10V; gain_range = "+/-10V"; break;
	}
	printf("Gain Range for channel %d: %s",channel,gain_range);
	mode = SINGLE_ENDED;
	for (i = 0; i < NCHAN_1208HS; i++) {
	  range[i] = gain;
	}
	usbAInConfig_USB1208HS(udev, mode, range);
  frequency = 1000;
  printf("Sampling frequency: %1.02fHz",frequency);

	usbAInScanStart_USB1208HS(udev, 0, 0, frequency, (0x1<<channel), 0xff, 0);
  usleep(1000);
  while(1) {
    ret = usbAInScanRead_USB1208HS(udev, BLOCKSIZE, 1, sdataIn);
    if (ret < 0) continue;

    write(fd,sdataIn,BLOCKSIZE);
    /*sdataIn[i] = rint(sdataIn[i]*table_AIN[mode][gain][0] + table_AIN[mode][gain][1]);
    volts = volts_USB1208HS(mode, gain, sdataIn[i]);*/
  }
	usbAInScanStop_USB1208HS(udev);
  close(fd);

  return 0;
}

