//
// Created by pb00170 on 9/16/25.
//

#ifndef WPFTPD_OS_H
#define WPFTPD_OS_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

// Networking / filesystem related (kept here for convenience across sources)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>
#include <time.h>

// Add system user / auth related headers
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>

#endif //WPFTPD_OS_H