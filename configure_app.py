#!env python

header  = 'main/app_configuration.h'
_help = f'''Usage: configure_app [-h]

Prompts the user for the necessary information to configure this project.

The configuration will be stored in the "{header}" file; every
invocation of this script will first delete the contents of such file.'''

def header_string():
    '''
    Prompts the user for the neccessary information required to configure
    the project. The returned string has the format for a header file
    '''
    default_cmd = "/camera/cmd"
    default_img = "/camera/img"

    ssid, passwd, uri = [
        input(f'Enter {s}: ') for s in [
        'WIFI SSID',
        'WIFI Password',
        'MQTT broker URI'
        ]
    ]

    uri = uri.lower().replace('mqtt://','').split(':')
    host, port = uri if len(uri) > 1 \
        else [uri[0], input('Enter the MQTT port: ')]

    port = port or 1883 # mosquitto's default port

    cmd_topic = input(f"MQTT topic to read commands from [{default_cmd}]: ") \
        or default_cmd

    img_topic = input(f"MQTT topic to send images to [{default_img}]: ") \
        or default_img

    return f'''
#ifndef APP_CONFIGURATION_H
#define APP_CONFIGURATION_H

#define SSID "{ssid}"
#define PASSWORD "{passwd}"
#define MQTT_URI "mqtt://{host}:{port}"
#define MQTT_CMD_TOPIC "{cmd_topic}"
#define MQTT_IMG_TOPIC "{img_topic}"

#endif
'''

def run():
    import sys
    if len(sys.argv) > 1:
        print(_help)
        return

    with open(header, 'w') as f:
        f.write(header_string())

if __name__ == '__main__':
    run()
else:
    # run test with:
    #
    # python -c 'import confiure_app'
    #

    # mock the "input" builtin
    import builtins
    builtins.input = lambda _ : answers.pop(0)

    def expect(port=1234):
        return f'''
#ifndef APP_CONFIGURATION_H
#define APP_CONFIGURATION_H

#define SSID "my_ssid"
#define PASSWORD "my_passwd"
#define MQTT_URI "mqtt://127.0.0.1:{port}"
#define MQTT_CMD_TOPIC "/camera/cmd"
#define MQTT_IMG_TOPIC "/camera/img"

#endif
'''

    answers = ['my_ssid', 'my_passwd', 'mqtt://127.0.0.1:1234', '', '']
    assert header_string() == expect()

    answers = ['my_ssid', 'my_passwd', '127.0.0.1', '1234', '', '']
    assert header_string() == expect()

    answers = ['my_ssid', 'my_passwd', '127.0.0.1', '', '', '']
    assert header_string() == expect(1883)

    answers = ['my_ssid', 'my_passwd', '127.0.0.1:1234', '', '']
    assert header_string() == expect()

    print("ok")
