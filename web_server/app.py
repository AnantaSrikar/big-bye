"""
	@file app.py
	@author Ananta Srikar
	@brief Python server that responds to ESP32's requests with messages for each user
	@version 0.1
	@date 2023-07-09
	
	@copyright Copyright (c) 2023
"""

from flask import Flask

# TODO: Get list of valid users from JSON file

app = Flask(__name__)

@app.route("/", methods=["GET"])
def root():
	return "Welcome to project BigBye!"

@app.route("/user/<username>", methods=["GET"])
def get_user_msgs(username):
	return f"I have to send a JSON here for {username}!"