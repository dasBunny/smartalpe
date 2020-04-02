#!/usr/bin/python
# -*- coding: utf-8 -*-

# This shows a simple example of an MQTT subscriber.


import paho.mqtt.client as mqtt
import json
import datetime
import atexit
from config import ttnc_topic, sfc_topic, sf_user, sf_pw, sf_client, ttn_pw, ttn_user, sf_broker

def exitmsg():
    print("Called exit function")

def ttn_connect(ttnc, obj, flags, rc):
    if rc==0:
        print("TTN connected OK Returned code=",rc)
    else:
        print("TTN Bad connection Returned code=",rc)
    ttnc.subscribe(ttnc_topic, 0)


def ttn_message(ttnc, obj, msg):
    print("Received topic: " + msg.topic + " with QOS " + str(msg.qos))
    print("Payload: " + str(msg.payload))
    ttnMsg =  json.loads(msg.payload)
    print(ttnMsg["app_id"])
    print(ttnMsg["payload_fields"]["distance"])
    UNIXtime = round(datetime.datetime.utcnow().timestamp()*1000)
    print("Timestamp: "+str(UNIXtime))
    sfMsg = {
        "events":[  
            {"type":"watersensor",
            "timestamp":UNIXtime,
            "data": ttnMsg["payload_fields"]
            }
        ]
    }
    print(sfMsg)
    sfMsg_string = json.dumps(sfMsg)
    sfc.publish(sfc_topic,sfMsg_string,qos=1)

def ttn_publish(ttnc, obj, mid):
    print("mid: " + str(mid))


def ttn_subscribe(ttnc, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def ttn_log(ttnc, obj, level, string):
    print("TTN: "+string)

def sf_connect(ttnc, obj, flags, rc):
    if rc==0:
        print("SF connected OK Returned code=",rc)
    else:
        print("SF Bad connection Returned code=",rc)


def sf_message(ttnc, obj, msg):
    print("Received topic: " + msg.topic + " with QOS " + str(msg.qos))
    print("Payload: " + str(msg.payload))


def sf_publish(ttnc, obj, mid):
    print("mid: " + str(mid))


def sf_subscribe(ttnc, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def sf_log(ttnc, obj, level, string):
    print("SF: "+string)

atexit.register(exitmsg)
ttnc = mqtt.Client()
ttnc.on_message = ttn_message
ttnc.on_connect = ttn_connect
ttnc.on_publish = ttn_publish
ttnc.on_subscribe = ttn_subscribe
ttnc.on_log = ttn_log
ttnc.username_pw_set(ttn_user, ttn_pw)
ttnc.connect("eu.thethings.network", 1883, 60)
ttnc.loop_start()
atexit.register(ttnc.loop_stop)

print("Done with TTN setup")
sfc = mqtt.Client(client_id=sf_client)
sfc.on_message = sf_message
sfc.on_connect = sf_connect
sfc.on_publish = sf_publish
sfc.on_subscribe = sf_subscribe
sfc.on_log = sf_log
sfc.username_pw_set(sf_user, sf_pw)
sfc.connect(sf_broker, 1883, 60)
sfc.loop_start()
atexit.register(sfc.loop_stop)
while 1:
    pass