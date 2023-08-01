# Web Server

The web server functions as an API that responds with user messages when queried.

## Architecture
The web server adopts a straightforward architecture. Messages are stored in a `JSON` file. When queried for a message pertaining to a user, it checks the `JSON` file for the required entry and responds in the necessary format. It features two crucial endpoints:
- `/user/<username>` - returns the number of messages a user has.
- `/user/<username>/<msg_num>` - returns the message along with the heading.

These endpoints are utilized in the [firmware](../firmware/) to display the message for the user on their screen.

## Features
1. Simple and minimalistic design.
2. Dynamic nature due to the use of Python.

## Web Server Setup

### Reverse Proxy Web Server
I used [Caddy](https://caddyserver.com/) to reverse proxy the Flask server as it automatically handles both the TLS certification and HTTPS routing. It also allows you to add a layer of BasicAuth to ensure increased user privacy. My Caddy configuration is as follows (`/etc/caddy/Caddyfile`):

```
# Farewell project backend
yourwebdomain.here.com {
	reverse_proxy localhost:6969
	basicauth * {
		basicAuthUsername $2a$14$8dVYJ3oBhXJr/xNuRGWb3.jBB.aK45EeNrQTjVzjgGv6T7aO29yIy
	}
}
```

The hash displayed above is the password for your BasicAuth. You can generate it by running the following:

```
root@caddy:~# caddy hash-password 
Enter password: 
Confirm password: 
$2a$14$8dVYJ3oBhXJr/xNuRGWb3.jBB.aK45EeNrQTjVzjgGv6T7aO29yIy
```

Finally, restart `caddy` with `systemctl restart caddy` to activate your updated configurations.

# Flask Web Server
I use `gunicorn`, a production WSGI web server, to run the Flask application.
1. Create a Python virtual environment named `env` by running `python3 -m venv env`.
2. Activate it by running `source env/bin/activate`.
3. Update the base packages by running `pip3 install --upgrade pip setuptools`.
4. Install all the requirements by running `pip3 install -r requirements.txt`.
5. Start the Flask application with `gunicorn --bind localhost:6969 app:app`.

## User Privacy
Using BasicAuth on Caddy ensures access only to intended users who have permission to access the web server. The password for the same will be securely stored with the firmware on the ESP32, thus enhancing the system's privacy.