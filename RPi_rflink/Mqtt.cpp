#include "Mqtt.h"


void mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
  /* Pring all log messages regardless of level. */
  char buffer[1024];

  switch(level){
    case MOSQ_LOG_DEBUG:
    case MOSQ_LOG_INFO:
    case MOSQ_LOG_NOTICE:
    case MOSQ_LOG_WARNING:
      sprintf(buffer, "%i:%s\n", level, str);
      log(LOG_DEBUG, buffer);
      break;
    case MOSQ_LOG_ERR: {
      sprintf(buffer, "%i:%s\n", level, str);
      log(LOG_ERROR, buffer);
    }
  }
}

struct mosquitto *mosq = NULL;
char const *mqtt_host = "localhost";
char const *mqtt_topic = "/devices/rflink";
char const *mqtt_username = "";
char const *mqtt_pw = "";

void mqtt_setup(const char *mqtt_host, const char *mqtt_topic, const char *mqtt_username, const char *mqtt_pw){
  int port = 1883;
  int keepalive = 10;
  bool clean_session = false;

  mosquitto_lib_init();
  mosq = mosquitto_new("rflink", clean_session, NULL);
  if (!mosq) {
    log(LOG_ERROR, "Error: Out of memory.\n");
    mosq = NULL;
  }

  mosquitto_log_callback_set(mosq, mosq_log_callback);

  mosquitto_username_pw_set(mosq, mqtt_username, mqtt_pw);
  if(mosquitto_connect(mosq, mqtt_host, port, keepalive)){
    log(LOG_ERROR, "Unable to connect.\n");
    }
  int loop = mosquitto_loop_start(mosq);
  if(loop != MOSQ_ERR_SUCCESS){
    log(LOG_ERROR, "Unable to start loop\n");
  }
}

int mqtt_send(const char* buf, const int size) {
  //log(LOG_STATUS, "MQTT server send: ",false);
  //log(LOG_STATUS, buf);
  int c;
  int i;
  int ret = 0;
  char **arr = NULL;
  char buffer[1024];
  char prefix_full[1024];
  c = split(buf, ';', &arr);
  //sprintf(buffer, "found %d tokens.\n", c);
  //log(LOG_ERROR, buffer);

  if (c>4)
   if (strchr(arr[4], '=')!=NULL/* && strstr("X10", arr[2])!=NULL*/) {
    buffer[0] = '\0';
    prefix_full[0] = '\0';
    sprintf(buffer, "%s/%s/%s", mqtt_topic, arr[2], strchr(arr[3], '=')+1);
        /*int ret = mosquitto_loop_start(mosq);
        if (ret == MOSQ_ERR_CONN_LOST) {
          log(LOG_ERROR, "MQTT server: reconnect...");
          mosquitto_reconnect(mosq);
        }
        if (ret != MOSQ_ERR_SUCCESS){
          log(LOG_ERROR, "Unable to start loop (2)\n");
        }*/
    for (i = 4; i < c; i++) 
      if (strchr(arr[i], '=')!=NULL) {
        char *param, *value;
        param=arr[i];
        value=strchr(arr[i], '=')+1;
        strchr(arr[i], '=')[0]='\0';
        sprintf(prefix_full, "%s/%s", buffer, param);
        ret=mosquitto_publish(mosq, NULL, prefix_full, strlen(value), value, 0, false);
        if ( ret == -1 ) {
          log(LOG_ERROR,"MQTT server: Problem send data.");
        }
        //sprintf(buffer, "%s %s", prefix_full, value);
        log(LOG_ERROR, prefix_full);
        log(LOG_ERROR, value);
        usleep(100000);
        //mosquitto_disconnect(mosq);
      }
        mosquitto_loop_start(mosq);
        if (ret == MOSQ_ERR_CONN_LOST) {
          log(LOG_ERROR, "MQTT server: reconnect...");
          mosquitto_reconnect(mosq);
        }
        if (ret != MOSQ_ERR_SUCCESS){
          log(LOG_ERROR, "Unable to start loop (3)\n");
        }

        //mosquitto_loop_stop(mosq, true);
    }

  return ret;
}

