/****************************************************************************
 * examples/tcpecho/tcpecho_main.c
 *
 *   Copyright (C) 2013 Max Holtzberg. All rights reserved.
 *   Copyright (C) 2008, 2011-2012, 2015 Gregory Nutt. All rights reserved.
 *
 *   Authors: Max Holtzberg <mh@uvc.de>
 *            Gregory Nutt <gnutt@nuttx.org>
 *
 * This code is based upon the poll example from W. Richard Stevens'
 * UNIX Network Programming Book.
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

#include <debug.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <nuttx/config.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>

#include <nuttx/net/arp.h>
#include "netutils/netlib.h"

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
#  include <arpa/inet.h>
#endif


#define LED_DEV_NAME "/dev/userleds"


/* Here we include the header file for the application(s) we use in
 * our project as defined in the config/<board-name>/defconfig file
 */

/* DHCPC may be used in conjunction with any other feature (or not) */

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
#  include "netutils/dhcpc.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCPECHO_MAXLINE 64
#define TCPECHO_POLLTIMEOUT 10000

/****************************************************************************
 * Private Data
 ****************************************************************************/
int fd = NULL; // File desciptor for led
int glb_dly = 500000;
static int ffd = NULL; 
static char* fifo_name = "/fifo";
static char rec_char;
static char TEST[] = "All work and no play makes Jack a dull boy\r\n\0";
static char TEST_0[] = "All play and no work makes Jack a mere toy\r\n\0";
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int tcpecho_netsetup(void);
static int tcpecho_server(void);

static int open_led_dev(void);


static int open_led_dev(void)
{
  int _fd = open(LED_DEV_NAME, O_WRONLY);
  if(_fd < 0)
  {
    printf("Error opening LED device %s\n\n", LED_DEV_NAME);
  }
  else
  {
    printf("Sucess opening LED device %s\n\n", LED_DEV_NAME);
  }
  return _fd;
}

static void toogle_led(int led_fd)
{
  static internal_delay = 50000;
  static state = 1;
  int off = 0;
  int on = 1;
  printf(".");
 // if(internal_delay > 2)
 // {
 //   --internal_delay;
 // }
 // else
 // {
 //   internal_delay = glb_dly;
    // toogle 
    state ^= 1;
    if(state)
    {
      write(led_fd,&on,1);
    }
    else
    {
      write(led_fd,&off,1);
    }
 // }
}



/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int tcpecho_netsetup()
{
  /* If this task is excecutated as an NSH built-in function, then the
   * network has already been configured by NSH's start-up logic.
   */

#ifndef CONFIG_NSH_BUILTIN_APPS
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_TCPECHO_DHCPC) || defined(CONFIG_EXAMPLES_TCPECHO_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
  struct dhcpc_state ds;
  void *handle;
#endif

/* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_TCPECHO_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = HTONL(CONFIG_EXAMPLES_TCPECHO_IPADDR);
#endif
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */
  addr.s_addr = HTONL(CONFIG_EXAMPLES_TCPECHO_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_TCPECHO_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
  /* Get the MAC address of the NIC */

  netlib_getmacaddr("eth0", mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open(&mac, IFHWADDRLEN);

  /* Get an IP address.  Note:  there is no logic here for renewing the address in this
   * example.  The address should be renewed in ds.lease_time/2 seconds.
   */

  if (!handle)
    {
      return ERROR;
    }

  if (dhcpc_request(handle, &ds) != OK)
    {
      return ERROR;
    }

  netlib_set_ipv4addr("eth0", &ds.ipaddr);

  if (ds.netmask.s_addr != 0)
    {
      netlib_set_ipv4netmask("eth0", &ds.netmask);
    }

  if (ds.default_router.s_addr != 0)
    {
      netlib_set_dripv4addr("eth0", &ds.default_router);
    }

  if (ds.dnsaddr.s_addr != 0)
    {
      netlib_set_ipv4dnsaddr(&ds.dnsaddr);
    }

  dhcpc_close(handle);
  printf("IP: %s\n", inet_ntoa(ds.ipaddr));

#endif /* CONFIG_EXAMPLES_TCPECHO_DHCPC */
#endif /* CONFIG_NSH_BUILTIN_APPS */
    printf("Ok\n");
  return OK;
}

static int tcpecho_server(void)
{
  int i, maxi, listenfd, connfd, sockfd;
  int nready;
  int ret;
  ssize_t n;
  char buf[TCPECHO_MAXLINE];
  socklen_t clilen;
  bool stop = false;
  struct pollfd client[CONFIG_EXAMPLES_TCPECHO_NCONN];
  struct sockaddr_in cliaddr, servaddr;
  int led_fd;
  static char nn = 0;
   // printf("b4 sock:AF_INET:%d\n", AF_INET);
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  //  printf("sock ok\n");
  if (listenfd < 0)
    {
      perror("ERROR: failed to create socket.\n");
      return ERROR;
    }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(CONFIG_EXAMPLES_TCPECHO_PORT);

  printf("ECHO PORT: %d\n",servaddr.sin_port);
  printf("ECHO PORT: %d\n",INADDR_ANY);

  /* Open Led device  */
  led_fd = open_led_dev();

  ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  printf("ret: %d\n",ret);
  if (ret < 0)
    {
      perror("ERROR: failed to bind socket.\n");
      return ERROR;
    }

  ninfo("start listening on port: %d\n", CONFIG_EXAMPLES_TCPECHO_PORT);
  printf("start listening on port: %d\n", CONFIG_EXAMPLES_TCPECHO_PORT);

  ret = listen(listenfd, CONFIG_EXAMPLES_TCPECHO_BACKLOG);
  if (ret < 0)
    {
      perror("ERROR: failed to start listening\n");
      return ERROR;
    }

  client[0].fd = listenfd;
  client[0].events = POLLRDNORM;
  for (i = 1; i < CONFIG_EXAMPLES_TCPECHO_NCONN; i++)
    {
      client[i].fd = -1;        /* -1 indicates available entry */
    }

  maxi = 0;                     /* max index into client[] array */

  while (!stop)
    {
      nready = poll(client, maxi+1, TCPECHO_POLLTIMEOUT);

      if (client[0].revents & POLLRDNORM)
        {
          /* new client connection */

          clilen = sizeof(cliaddr);
          connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
          printf("SEND\n");
//          write(connfd, TEST, sizeof(TEST));
//          write(connfd, TEST, sizeof(TEST));
          ninfo("new client: %s\n", inet_ntoa(cliaddr.sin_addr));
          printf("new client: %s\n", inet_ntoa(cliaddr.sin_addr));

          for (i = 1; i < CONFIG_EXAMPLES_TCPECHO_NCONN; i++)
            {
              if (client[i].fd < 0)
                {
                  client[i].fd = connfd;  /* save descriptor */
                  break;
                }
            }

          if (i == CONFIG_EXAMPLES_TCPECHO_NCONN)
            {
              perror("ERROR: too many clients");
              return ERROR;
            }

          client[i].events = POLLRDNORM;
          if (i > maxi)
            {
              maxi = i;    /* max index in client[] array */
            }

          if (--nready <= 0)
            {
              continue;    /* no more readable descriptors */
            }
        }

      for (i = 1; i <= maxi; i++)
        {
          /* check all clients for data */

          if ((sockfd = client[i].fd) < 0)
            {
              continue;
            }

          if (client[i].revents & (POLLRDNORM | POLLERR))
            {
              if ( (n = read(sockfd, buf, TCPECHO_MAXLINE)) < 0)
                {
                  if (errno == ECONNRESET)
                    {
                      /* connection reset by client */

                      nwarn("WARNING: client[%d] aborted connection\n", i);

                      close(sockfd);
                      client[i].fd = -1;
                    }
                  else
                    {
                      perror("ERROR: readline error\n");
                      close(sockfd);
                      client[i].fd = -1;
                    }
                }
              else if (n == 0)
                {
                  /* connection closed by client */

                  nwarn("WARNING: client[%d] closed connection\n", i);

                  close(sockfd);
                  client[i].fd = -1;
                }
              else
                {
                  if (strcmp(buf, "exit\r\n") == 0)
                    {
                      nwarn("WARNING: client[%d] closed connection\n", i);
                      close(sockfd);
                      client[i].fd = -1;
                    }
                  else
                    {
                      printf("received char: %i\n", buf[0]);
                      write(ffd, buf, 1);
                      //toogle_led(led_fd);
                      //write(sockfd, buf, n);
                      if(nn & 0x01){
                        write(sockfd, TEST, sizeof(TEST));
                      }
                      else{
                        write(sockfd, TEST_0, sizeof(TEST_0));
                      }
                      ++nn;
                    }
                }

              if (--nready <= 0)
                {
                  break;  /* no more readable descriptors */
                }
            }
        }
    }

  for (i = 0; i <= maxi; i++)
    {
      if (client[i].fd < 0)
        {
          continue;
        }

      close(client[i].fd);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * tcpecho_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int tcpecho_main(int argc, char *argv[])
#endif
{
  int ret;

  ret = tcpecho_netsetup();
  if (ret != OK)
    {
      perror("ERROR: failed to setup network\n");
      exit(1);
    }
  mkfifo(fifo_name, 0666);
  ffd = open(fifo_name, O_WRONLY | O_NONBLOCK);
  if(ffd < 0 ){
      printf("No fifo\n\n");
    }
  printf("Start echo server: %d\n", ret);

  ret = tcpecho_server();

  printf("ret: %d\n",ret);
  return ret;
#if 0
   int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 

    char sendBuff[1025];
    time_t ticks; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

        ticks = time(NULL);
        snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        write(connfd, sendBuff, strlen(sendBuff)); 

        close(connfd);
        sleep(1);
     }
#endif
}

