import os
import keyboard

import airsim
import airsim.airsim_types as at
import rm_bot_client as rm_bot_client

def get_control_signals():
    w_pressed = keyboard.is_pressed('w')
    s_pressed = keyboard.is_pressed('s')
    a_pressed = keyboard.is_pressed('a')
    d_pressed = keyboard.is_pressed('d')

    throttle = 0.7
    n_throttle = -1.0 * throttle

    if w_pressed:
        if a_pressed:
            return (0, throttle)
        elif d_pressed:
            return (throttle, 0)
        elif s_pressed:
            return (0, 0)
        else:
            return (throttle, throttle)

    elif s_pressed:
        if (a_pressed):
            return (n_throttle, 0)
        elif (d_pressed):
            return (0, n_throttle)
        else:
            return (n_throttle, n_throttle)

    elif a_pressed:
        if d_pressed:
            return (0, 0)
        else:
            return (n_throttle, throttle)

    elif d_pressed:
        return (throttle, n_throttle)

    return (0, 0)

def main():
    client = rm_bot_client.RmBotClient()

    os.system('cls')
    print('Competition is now running.')

    while True:
        left_throttle, right_throttle = get_control_signals()
        client.drive(left_throttle, right_throttle)
        break


    client.drive(0, 0)

    os.system('cls')
    print('Graceful termination.')
        
if __name__ == '__main__':
    main()
