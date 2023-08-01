"""
	@file app.py
	@author Ananta Srikar
	@brief Python server that responds to ESP32's requests with messages for each user
	@version 0.1
	@date 2023-07-09
	
	@copyright Copyright (c) 2023
"""

from flask import Flask, jsonify
from json import load

# Relative path of json file
JSON_FILE_PATH = "userDB.json"

app = Flask(__name__)

# Function to get all data from json file
def getJSONdata():
	with open(JSON_FILE_PATH, "r") as inFPtr:
		all_user_data = load(inFPtr)

	return all_user_data

# Default route used to check if the API is online
@app.route("/", methods=["GET"])
def root():
	return "Welcome to project BigBye!"

# Get the number of messages for a user
@app.route("/user/<username>", methods=["GET"])
def get_user_num_msgs(username):

	# Very bad way of handling missing case but it works ¯\_(ツ)_/¯
	try:
		return jsonify(getJSONdata()[username]["num_msgs"])

	except Exception as e:
		print(f"Something bad happened: {e}")
		return "Not sure what you're looking for :(", 404

# Get the message with number for the user
@app.route("/user/<username>/<msg_num>", methods=["GET"])
def get_user_msgs(username, msg_num):
	
	# The bad practice again :p
	try:
		return jsonify(getJSONdata()[username][msg_num])

	except Exception as e:
		print(f"Something bad happened: {e}")
		return "Not sure what you're looking for :(", 404