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

app = Flask(__name__)

@app.route("/", methods=["GET"])
def root():
	return "Welcome to project BigBye!"

@app.route("/user/<username>", methods=["GET"])
def get_user_msgs(username):
	
	# Open JSON db
	with open("userDB.json", "r") as inFPtr:
		all_user_data = load(inFPtr)

	if username in all_user_data:
		return jsonify(all_user_data[username])

	else:
		return "Not sure what you're looking for :(", 404