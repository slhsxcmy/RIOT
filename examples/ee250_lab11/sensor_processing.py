"""EE 250L Lab 11 Final Project

sensor_processing.py: Sensor data processing.

TODO: List team members here.
	  Mingyu Cui
	  Jui Po Hung
TODO: Insert Github repository link here.
	  https://github.com/usc-ee250-fall2018/finalproj-riot-cmyyyyyyy
"""

import paho.mqtt.client as mqtt
import time
import requests
import json
from datetime import datetime


# MQTT variables
broker_hostname = "eclipse.usc.edu"
broker_port = 11000

adc_topic = "mingyucu/light"


# Lists holding the ADC samples. Change the value of MAX_LIST_LENGTH depending on how 
# many ADC samples you would like to keep at any point in time.

THRESHOLD = 500
result = "LIGHT_OFF"
sample = 0

def sensor_callback(client, userdata, msg):
	global sample
	global result
	s = str(msg.payload)
	s = s.split("'")
	s = s[1]
	# print(s)
	sample = int(s)
	print(sample)


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
	print("Connected with result code " + str(rc))
	client.subscribe(adc_topic)
	client.message_callback_add(adc_topic, sensor_callback) 

# The callback for when a PUBLISH message is received from the server.
# This should not be called.
def on_message(client, userdata, msg): 
	print(msg.topic + " " + str(msg.payload))

def detectBrightness():
	global result
	global sample
	publish_flag = False
	if sample < THRESHOLD and result == "LIGHT_OFF":
		result = "LIGHT_ON"
		publish_flag = True
		print(result)
	elif sample >= THRESHOLD and result == "LIGHT_ON":
		result = "LIGHT_OFF"
		publish_flag = True
		print(result)
	return publish_flag
	
	
if __name__ == '__main__':
	
	# This header sets the HTTP request's mimetype to `application/json`. This
	# means the payload of the HTTP message will be formatted as a json ojbect
	hdr = {
		'Content-Type': 'application/json',
		'Authorization': None #not using HTTP secure
	}
	
	# Connect to broker and start loop    
	client = mqtt.Client()
	client.on_connect = on_connect
	client.on_message = on_message
	client.connect(broker_hostname, broker_port, 60)
	client.loop_start()

	while True:
		""" 
		Change this!!!!
		
		You have two lists, ranger1_dist and ranger2_dist, which hold a window
		of the past MAX_LIST_LENGTH samples published by ultrasonic ranger 1
		and 2, respectively. The signals are published roughly at intervals of
		200ms, or 5 samples/second (5 Hz). The values published are the 
		distances in centimeters to the closest object. The measurements can
		technically take values up to 1024, but you will mainly see values 
		between 0-700. Jumps in values will most likely be from 
		inter-sensor-interference, so be sure to filter the signal accordingly
		to remove these jumps. 
		"""
		
		
		# TODO: detect movement and/or position
		if detectBrightness() == True:

			# if publish_flag == 1:
			# The payload of our message starts as a simple dictionary. Before sending
			# the HTTP message, we will format this into a json object
			payload = {
				'time': str(datetime.now()),
				'event': str(result),
				'room' : "vhe_205"

			}
		
			# Send an HTTP POST message and block until a response is given.
			# Note: requests() is NOT the same thing as request() under the flask 
			# library.
			response = requests.post("http://0.0.0.0:5000/post-event", headers = hdr,
									 data = json.dumps(payload))
		
			# Print the json object from the HTTP response
			print(response.json())
			
		time.sleep(2)