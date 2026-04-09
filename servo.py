import time
import board
import pwmio
from adafruit_motor import servo
import analogio

# Initialize PWM for servo 
pwm = pwmio.PWMOut(board.GP15, frequency=50)  # 50 Hz for servo
my_servo = servo.Servo(pwm, min_pulse=1000, max_pulse=2000)


# Function to set servo angle
def set_servo(angle):
    if angle < 0:
        angle = 0
    if angle > 180:
        angle = 180
    my_servo.angle = angle

# Main loop (sweep back and forth) 
while True:
    # Sweep 10 to 170
    for i in range(10, 170):
        set_servo(i)
        time.sleep(0.01)

    # Sweep 170 to 10
    for i in range(170, 10, -1):
        set_servo(i)
        time.sleep(0.01)