/****************************************************************************
 * examples/rgbled/rgbled.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_RGBLED_DEVNAME
#  define CONFIG_EXAMPLES_RGBLED_DEVNAME "/dev/rgbled0"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static char* fifo_name = "/fifo";
static int rgb_ffd = 0;
static char rx_char = 20;
/****************************************************************************
 * rgbled_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int rgbled_main(int argc, char *argv[])
#endif
{
  int red = 255;
  int green = 0;
  int blue = 0;
  int sred = -1;
  int sgreen = 1;
  int sblue = 0;
  int fd;
  char buffer[8];

  fd = open(CONFIG_EXAMPLES_RGBLED_DEVNAME, O_WRONLY);
  rgb_ffd = open(fifo_name, O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    {
      printf("Error opening %s!\n", CONFIG_EXAMPLES_RGBLED_DEVNAME);
      return -1;
    }

    printf("RGB: received char: %i\n", rx_char);
  while(1)
  {
    if( read(rgb_ffd, &rx_char, 1) > 0)
    {
      printf("Led driver received char: %i\n", rx_char);
    }
    (void)write(fd, &green, 1);
    usleep(500 * rx_char);
    (void)write(fd, &sgreen, 1);
    usleep(500 * rx_char);
  }

  return 0;
}
