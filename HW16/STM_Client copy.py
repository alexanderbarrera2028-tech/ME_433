# Motor Control Client in python
import matplotlib.pyplot as plt
from statistics import mean
def read_plot_matrix():
    n_str = ser.read_until(b'\n'); # get the number of data points to receive
    n_int = float(n_str) # turn it into an int
    print('Data length = ' + str(n_int))
    ref = []
    data = []
    t = []
    data_received = 0
    while data_received < n_int:
        dat_str = ser.read_until(b'\n'); # get the data as a string, ints separated by spaces
        dat_f = list(map(float,dat_str.split())) # now the data is a list
        t.append(dat_f[0]) #index
        ref.append(dat_f[1]) #reference
        data.append(dat_f[2]) #measured
        
        data_received = data_received + 1
    meanzip = zip(ref,data)
    meanlist = []
    for i,j in meanzip:
        meanlist.append(abs(i-j))
    score = mean(meanlist)
    plt.plot(t,ref,'r*-',t,data,'b*-')
    plt.title('Score = ' + str(score))
    plt.ylabel('value')
    plt.xlabel('index')
    plt.show()
      

import serial

ser = serial.Serial('COM7', 115200, timeout=5)  # open the serial port that your PIC32 is connected to, and set the baud rate
print('Opening port: ')
print(ser.name)
has_quit = False
# menu loop
while not has_quit:
    print('STM32 MOTOR DRIVER INTERFACE')
    # display the menu options; this list will grow
    print('a: current test \tq: quit') # '\t' is a tab
    # read the user's choice
    selection = input('\nENTER COMMAND: ')
    selection_endline = selection+'\n'
    # send the command to the PIC32
    ser.write(selection_endline.encode()); # .encode() turns the string into a char array
    # take the appropriate action
    # there is no switch() in python, using if elif instead
    if (selection == 'a'):
        read_plot_matrix()
    elif (selection == 'q'):
        print('Exiting client')
        has_quit = True; # exit client
        # be sure to close the port
        ser.close()
    else:
        print('Invalid Selection ' + selection_endline)

