#ifndef _MQTTPlugin_h
#define _MQTTPlugin_h

#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Base.h"
#include "Misc.h"

extern struct mosquitto *mosq;
extern char const *mqtt_host;
extern char const *mqtt_topic;
extern char const *mqtt_username;
extern char const *mqtt_pw;

void mqtt_setup(const char *mqtt_host, const char *mqtt_topic, const char *mqtt_username, const char *mqtt_pw);
int mqtt_send(const char* buf, const int size);

#endif
